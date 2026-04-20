#include "http-server.h"
#include "graph-export.h"
#include "util.h"

#include "search/deep-search-session.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_BACKLOG      10
#define READ_CHUNK_SIZE     4096
#define INITIAL_BUFFER_CAP  8192
#define MAX_REQUEST_SIZE    (16 * 1024 * 1024)
#define MAX_HEADER_SIZE     (256 * 1024)

#define MAX_SSE_CLIENTS     64
#define MAX_RESEARCH_ID_LEN 63

static _Bool started = 0;
static int server_fd = -1;
static pthread_t server_thread;
static pthread_mutex_t server_lock = PTHREAD_MUTEX_INITIALIZER;

/* ========================= HTTP REQUEST ========================= */

typedef struct {
	char method[16];
	char path[256];
	char* body;
	size_t body_len;
} HttpRequest;

/* ========================= SSE CLIENTS ========================= */

typedef struct {
	int fd;
	_Bool alive;
	pthread_mutex_t write_lock;
	char research_id[MAX_RESEARCH_ID_LEN + 1];
} ClientConnection;

static ClientConnection sse_clients[MAX_SSE_CLIENTS];
static pthread_mutex_t sse_clients_lock = PTHREAD_MUTEX_INITIALIZER;

/* ========================= INTERNAL HELPERS ========================= */

static char* get_graph_data(void) {
	return SeriliazeGraph();
}

int server_is_running(void) {
	int running;

	pthread_mutex_lock(&server_lock);
	running = started ? 1 : 0;
	pthread_mutex_unlock(&server_lock);

	return running;
}

static int ascii_ncasecmp_n(const char* a, const char* b, size_t n) {
	size_t i;

	for (i = 0; i < n; i++) {
		unsigned char ca = (unsigned char)a[i];
		unsigned char cb = (unsigned char)b[i];

		ca = (unsigned char)tolower(ca);
		cb = (unsigned char)tolower(cb);

		if (ca != cb) return (int)ca - (int)cb;
		if (ca == '\0') return 0;
	}

	return 0;
}

static int send_all(int fd, const void* data, size_t len) {
	const char* p = (const char*)data;
	size_t sent_total = 0;

	while (sent_total < len) {
		ssize_t sent_now = send(fd, p + sent_total, len - sent_total, 0);

		if (sent_now < 0) {
			if (errno == EINTR) continue;
			return -1;
		}

		if (sent_now == 0) {
			return -1;
		}

		sent_total += (size_t)sent_now;
	}

	return 0;
}

static void send_response(
	int client_fd,
	int status_code,
	const char* status_text,
	const char* content_type,
	const char* body,
	size_t body_len
) {
	char header[1024];
	int header_len;

	header_len = snprintf(
		header,
		sizeof(header),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n"
		"Connection: close\r\n"
		"\r\n",
		status_code,
		status_text,
		content_type,
		body_len
	);

	if (header_len < 0 || (size_t)header_len >= sizeof(header)) return;

	send_all(client_fd, header, (size_t)header_len);

	if (body && body_len > 0) {
		send_all(client_fd, body, body_len);
	}
}

static void send_json_response(
	int client_fd,
	int status_code,
	const char* status_text,
	const char* json_body
) {
	send_response(
		client_fd,
		status_code,
		status_text,
		"application/json",
		json_body,
		strlen(json_body)
	);
}

static void handle_options(int client_fd) {
	static const char* response =
		"HTTP/1.1 204 No Content\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"\r\n";

	send_all(client_fd, response, strlen(response));
}

static const char* find_header_value(const char* headers, const char* name) {
	size_t name_len = strlen(name);
	const char* p = headers;

	while (*p) {
		const char* line_end = strstr(p, "\r\n");
		if (!line_end) return NULL;

		if ((size_t)(line_end - p) > name_len &&
		    ascii_ncasecmp_n(p, name, name_len) == 0 &&
		    p[name_len] == ':') {

			const char* value = p + name_len + 1;
			while (*value == ' ' || *value == '\t') value++;
			return value;
		}

		p = line_end + 2;
		if (*p == '\r' && *(p + 1) == '\n') break;
	}

	return NULL;
}

static int parse_content_length(const char* headers, size_t* out_len) {
	const char* value = find_header_value(headers, "Content-Length");
	char* endptr = NULL;
	unsigned long long n;

	if (!value) {
		*out_len = 0;
		return 0;
	}

	n = strtoull(value, &endptr, 10);
	if (endptr == value) return -1;

	*out_len = (size_t)n;
	return 0;
}

static int read_into_string(int fd, String* s, size_t max_total) {
	char chunk[READ_CHUNK_SIZE];

	for (;;) {
		ssize_t n = recv(fd, chunk, sizeof(chunk), 0);

		if (n < 0) {
			if (errno == EINTR) continue;
			return -1;
		}

		if (n == 0) {
			return 0;
		}

		if (s->len + (size_t)n > max_total) {
			return -1;
		}

		CatString(s, chunk, (size_t)n);
		return 1;
	}
}

static int read_http_request(int client_fd, HttpRequest* req) {
	String raw;
	size_t header_end_offset = 0;
	size_t content_length = 0;
	char* header_end = NULL;
	char* line_end = NULL;

	memset(req, 0, sizeof(*req));
	InitString(&raw, INITIAL_BUFFER_CAP);

	while (!header_end) {
		int rc;

		if (raw.len > MAX_HEADER_SIZE) {
			FreeString(&raw);
			return -1;
		}

		rc = read_into_string(client_fd, &raw, MAX_REQUEST_SIZE);
		if (rc <= 0) {
			FreeString(&raw);
			return -1;
		}

		header_end = strstr(c_str(&raw), "\r\n\r\n");
	}

	header_end_offset = (size_t)(header_end - c_str(&raw)) + 4;

	line_end = strstr(c_str(&raw), "\r\n");
	if (!line_end) {
		FreeString(&raw);
		return -1;
	}

	*line_end = '\0';

	if (sscanf(c_str(&raw), "%15s %255s", req->method, req->path) != 2) {
		FreeString(&raw);
		return -1;
	}

	*line_end = '\r';

	if (parse_content_length(c_str(&raw), &content_length) != 0) {
		FreeString(&raw);
		return -1;
	}

	if (content_length > MAX_REQUEST_SIZE) {
		FreeString(&raw);
		return -1;
	}

	while (raw.len < header_end_offset + content_length) {
		int rc = read_into_string(client_fd, &raw, MAX_REQUEST_SIZE);
		if (rc <= 0) {
			FreeString(&raw);
			return -1;
		}
	}

	req->body_len = content_length;
	req->body = malloc(content_length + 1);
	cassert(req->body != NULL, "Failed to allocate request body\n");

	if (content_length > 0) {
		memcpy(req->body, c_str(&raw) + header_end_offset, content_length);
	}

	req->body[content_length] = '\0';

	FreeString(&raw);
	return 0;
}

static void free_http_request(HttpRequest* req) {
	if (req->body) {
		free(req->body);
		req->body = NULL;
	}
	req->body_len = 0;
}

/* ========================= PATH / QUERY HELPERS ========================= */

static void split_path_and_query(const char* full_path, char* path_only, size_t path_cap, const char** query_out) {
	const char* q = strchr(full_path, '?');
	size_t path_len;

	if (!q) {
		snprintf(path_only, path_cap, "%s", full_path);
		*query_out = NULL;
		return;
	}

	path_len = (size_t)(q - full_path);
	if (path_len >= path_cap) path_len = path_cap - 1;

	memcpy(path_only, full_path, path_len);
	path_only[path_len] = '\0';
	*query_out = q + 1;
}

static int query_get_param(const char* query, const char* key, char* out, size_t out_cap) {
	size_t key_len;
	const char* p;

	if (!query || !key || !out || out_cap == 0) return 0;

	key_len = strlen(key);
	p = query;

	while (*p) {
		const char* amp = strchr(p, '&');
		const char* end = amp ? amp : p + strlen(p);
		const char* eq = memchr(p, '=', (size_t)(end - p));

		if (eq) {
			size_t cur_key_len = (size_t)(eq - p);
			size_t val_len = (size_t)(end - eq - 1);

			if (cur_key_len == key_len && memcmp(p, key, key_len) == 0) {
				if (val_len >= out_cap) val_len = out_cap - 1;
				memcpy(out, eq + 1, val_len);
				out[val_len] = '\0';
				return 1;
			}
		}

		if (!amp) break;
		p = amp + 1;
	}

	return 0;
}

/* ========================= JSON ESCAPE FOR SSE ========================= */

static char* json_escape_dup_n(const char* src, size_t len) {
	size_t i;
	size_t cap = len * 6 + 1;
	char* out = malloc(cap);
	size_t w = 0;

	cassert(out != NULL, "Failed to allocate escaped json buffer\n");

	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char)src[i];

		switch (c) {
			case '\"':
				out[w++] = '\\';
				out[w++] = '\"';
				break;
			case '\\':
				out[w++] = '\\';
				out[w++] = '\\';
				break;
			case '\n':
				out[w++] = '\\';
				out[w++] = 'n';
				break;
			case '\r':
				out[w++] = '\\';
				out[w++] = 'r';
				break;
			case '\t':
				out[w++] = '\\';
				out[w++] = 't';
				break;
			default:
				if (c < 0x20) {
					w += (size_t)sprintf(out + w, "\\u%04x", (unsigned)c);
				} else {
					out[w++] = (char)c;
				}
				break;
		}
	}

	out[w] = '\0';
	return out;
}

/* ========================= SSE REGISTRY ========================= */

static void init_sse_clients(void) {
	int i;

	pthread_mutex_lock(&sse_clients_lock);
	for (i = 0; i < MAX_SSE_CLIENTS; i++) {
		sse_clients[i].fd = -1;
		sse_clients[i].alive = 0;
		sse_clients[i].research_id[0] = '\0';
	}
	pthread_mutex_unlock(&sse_clients_lock);
}

static void remove_sse_client_locked(int idx) {
	if (idx < 0 || idx >= MAX_SSE_CLIENTS) return;

	if (sse_clients[idx].alive) {
		shutdown(sse_clients[idx].fd, SHUT_RDWR);
		close(sse_clients[idx].fd);
		sse_clients[idx].fd = -1;
		sse_clients[idx].alive = 0;
		sse_clients[idx].research_id[0] = '\0';
		pthread_mutex_destroy(&sse_clients[idx].write_lock);
	}
}

static int add_sse_client(int fd, const char* research_id) {
	int i;

	pthread_mutex_lock(&sse_clients_lock);

	for (i = 0; i < MAX_SSE_CLIENTS; i++) {
		if (!sse_clients[i].alive) {
			sse_clients[i].fd = fd;
			sse_clients[i].alive = 1;
			sse_clients[i].research_id[0] = '\0';
			if (research_id) {
				strncpy(sse_clients[i].research_id, research_id, MAX_RESEARCH_ID_LEN);
				sse_clients[i].research_id[MAX_RESEARCH_ID_LEN] = '\0';
			}
			pthread_mutex_init(&sse_clients[i].write_lock, NULL);
			pthread_mutex_unlock(&sse_clients_lock);
			return 1;
		}
	}

	pthread_mutex_unlock(&sse_clients_lock);
	return 0;
}

static void prune_dead_sse_clients(void) {
	int i;

	pthread_mutex_lock(&sse_clients_lock);
	for (i = 0; i < MAX_SSE_CLIENTS; i++) {
		if (sse_clients[i].alive) {
			char ping[] = ": ping\n\n";
			pthread_mutex_lock(&sse_clients[i].write_lock);
			if (send_all(sse_clients[i].fd, ping, strlen(ping)) != 0) {
				pthread_mutex_unlock(&sse_clients[i].write_lock);
				remove_sse_client_locked(i);
				continue;
			}
			pthread_mutex_unlock(&sse_clients[i].write_lock);
		}
	}
	pthread_mutex_unlock(&sse_clients_lock);
}

/* ========================= SSE EMIT ========================= */

void ds_emit_event(const char* id, const char* type, const char* buffer, size_t buffer_len) {
	char* esc_id = NULL;
	char* esc_type = NULL;
	char* esc_data = NULL;
	size_t payload_cap;
	char* payload;
	int payload_len;
	int i;

	if (!server_is_running()) return;

	if (!id) id = "";
	if (!type) type = "";
	if (!buffer) {
		buffer = "";
		buffer_len = 0;
	}

	esc_id = json_escape_dup_n(id, strlen(id));
	esc_type = json_escape_dup_n(type, strlen(type));
	esc_data = json_escape_dup_n(buffer, buffer_len);

	payload_cap =
		strlen(esc_id) +
		strlen(esc_type) +
		strlen(esc_data) +
		128;

	payload = malloc(payload_cap);
	cassert(payload != NULL, "Failed to allocate SSE payload\n");

	payload_len = snprintf(
		payload,
		payload_cap,
		"{\"id\":\"%s\",\"type\":\"%s\",\"data\":\"%s\"}",
		esc_id,
		esc_type,
		esc_data
	);

	if (payload_len > 0) {
		pthread_mutex_lock(&sse_clients_lock);

		for (i = 0; i < MAX_SSE_CLIENTS; i++) {
			if (sse_clients[i].alive) {
				int send_failed = 0;

				if (sse_clients[i].research_id[0] != '\0' && strcmp(sse_clients[i].research_id, id) != 0) {
					continue;
				}

				pthread_mutex_lock(&sse_clients[i].write_lock);

				if (send_all(sse_clients[i].fd, "data: ", 6) != 0) send_failed = 1;
				if (!send_failed && send_all(sse_clients[i].fd, payload, (size_t)payload_len) != 0) send_failed = 1;
				if (!send_failed && send_all(sse_clients[i].fd, "\n\n", 2) != 0) send_failed = 1;

				pthread_mutex_unlock(&sse_clients[i].write_lock);

				if (send_failed) {
					remove_sse_client_locked(i);
				}
			}
		}

		pthread_mutex_unlock(&sse_clients_lock);
	}

	free(payload);
	free(esc_data);
	free(esc_type);
	free(esc_id);
}

/* ========================= ROUTE HANDLERS ========================= */

static void handle_get_graph(int client_fd) {
	char* graph_json = get_graph_data();

	if (!graph_json) {
		send_json_response(
			client_fd,
			500,
			"Internal Server Error",
			"{\"ok\":false,\"error\":\"serialize_graph_failed\"}"
		);
		return;
	}

	send_response(
		client_fd,
		200,
		"OK",
		"application/json",
		graph_json,
		strlen(graph_json)
	);

	free(graph_json);
}

static const char* json_skip_ws(const char* p) {
	while (*p && isspace((unsigned char)*p)) p++;
	return p;
}

static int json_get_string_field(const char* json, const char* key, char* out, size_t out_cap) {
	char pattern[128];
	const char* p;
	const char* start;
	size_t w = 0;

	if (!json || !key || !out || out_cap == 0) return 0;

	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	p = strstr(json, pattern);
	if (!p) return 0;

	p += strlen(pattern);
	p = json_skip_ws(p);
	if (*p != ':') return 0;

	p++;
	p = json_skip_ws(p);
	if (*p != '"') return 0;

	p++;
	start = p;

	while (*p) {
		if (*p == '\\') {
			p++;
			if (*p) p++;
			continue;
		}
		if (*p == '"') break;
		p++;
	}

	if (*p != '"') return 0;

	while (start < p && w + 1 < out_cap) {
		if (*start == '\\') {
			start++;
			if (!*start) break;

			switch (*start) {
				case '"':  out[w++] = '"'; break;
				case '\\': out[w++] = '\\'; break;
				case '/':  out[w++] = '/'; break;
				case 'b':  out[w++] = '\b'; break;
				case 'f':  out[w++] = '\f'; break;
				case 'n':  out[w++] = '\n'; break;
				case 'r':  out[w++] = '\r'; break;
				case 't':  out[w++] = '\t'; break;
				default:   out[w++] = *start; break;
			}
			start++;
		} else {
			out[w++] = *start++;
		}
	}

	out[w] = '\0';
	return 1;
}

static int json_get_int_field(const char* json, const char* key, int* out_value) {
	char pattern[128];
	const char* p;
	char* endptr = NULL;
	long value;

	if (!json || !key || !out_value) return 0;

	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	p = strstr(json, pattern);
	if (!p) return 0;

	p += strlen(pattern);
	p = json_skip_ws(p);
	if (*p != ':') return 0;

	p++;
	p = json_skip_ws(p);

	errno = 0;
	value = strtol(p, &endptr, 10);
	if (endptr == p || errno != 0) return 0;

	*out_value = (int)value;
	return 1;
}

static void handle_post_research_start(int client_fd, const HttpRequest* req) {
	char research_id[MAX_RESEARCH_ID_LEN + 1];
	char task_name[256];
	int min_rounds = 0;
	String out;
	Task task;

	research_id[0] = '\0';
	task_name[0] = '\0';

	if (!req->body) {
		send_json_response(
			client_fd,
			400,
			"Bad Request",
			"{\"ok\":false,\"error\":\"missing_body\"}"
		);
		return;
	}

	if (!json_get_string_field(req->body, "id", research_id, sizeof(research_id))) {
		send_json_response(
			client_fd,
			400,
			"Bad Request",
			"{\"ok\":false,\"error\":\"missing_id\"}"
		);
		return;
	}

	if (!json_get_string_field(req->body, "taskName", task_name, sizeof(task_name))) {
		send_json_response(
			client_fd,
			400,
			"Bad Request",
			"{\"ok\":false,\"error\":\"missing_task_name\"}"
		);
		return;
	}

	if (!json_get_int_field(req->body, "minRounds", &min_rounds)) {
		send_json_response(
			client_fd,
			400,
			"Bad Request",
			"{\"ok\":false,\"error\":\"missing_min_rounds\"}"
		);
		return;
	}

	if (min_rounds < 1) {
		send_json_response(
			client_fd,
			400,
			"Bad Request",
			"{\"ok\":false,\"error\":\"invalid_min_rounds\"}"
		);
		return;
	}

	printf("research/start id=%s taskName=%s minRounds=%d\n",
	       research_id,
	       task_name,
	       min_rounds);

	memset(&task, 0, sizeof(task));

	strncpy(task.name, task_name, sizeof(task.name) - 1);
	task.name[sizeof(task.name) - 1] = '\0';
	task.name_len = sizeof(task.name);
	task.minDepth = min_rounds;

	InitString(&out, 1024);

	ds_emit_event(research_id, "research_started", "deep search started", strlen("deep search started"));

	start_ds_session(&task, research_id, &out);

	send_response(
		client_fd,
		200,
		"OK",
		"application/json",
		c_str(&out),
		out.len
	);

	FreeString(&out);
}

static void handle_get_research_events(int client_fd, const char* full_path) {
	char path_only[256];
	const char* query = NULL;
	char research_id[MAX_RESEARCH_ID_LEN + 1];
	static const char* header =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/event-stream\r\n"
		"Cache-Control: no-cache\r\n"
		"Connection: keep-alive\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"\r\n";

	split_path_and_query(full_path, path_only, sizeof(path_only), &query);
	research_id[0] = '\0';

	query_get_param(query, "id", research_id, sizeof(research_id));

	if (send_all(client_fd, header, strlen(header)) != 0) {
		close(client_fd);
		return;
	}

	if (!add_sse_client(client_fd, research_id[0] ? research_id : NULL)) {
		send_json_response(
			client_fd,
			503,
			"Service Unavailable",
			"{\"ok\":false,\"error\":\"too_many_sse_clients\"}"
		);
		close(client_fd);
		return;
	}

	if (research_id[0]) {
		ds_emit_event(research_id, "sse_connected", "connected", strlen("connected"));
	}
}

static void handle_not_found(int client_fd) {
	send_json_response(
		client_fd,
		404,
		"Not Found",
		"{\"ok\":false,\"error\":\"route_not_found\"}"
	);
}

static void handle_bad_request(int client_fd) {
	send_json_response(
		client_fd,
		400,
		"Bad Request",
		"{\"ok\":false,\"error\":\"bad_request\"}"
	);
}

/* returns 1 if caller should keep socket open */
static int handle_request(int client_fd, const HttpRequest* req) {
	char path_only[256];
	const char* query_unused = NULL;

	split_path_and_query(req->path, path_only, sizeof(path_only), &query_unused);

	if (strcmp(req->method, "OPTIONS") == 0) {
		handle_options(client_fd);
		return 0;
	}

	if (strcmp(req->method, "GET") == 0 && strcmp(path_only, "/graph") == 0) {
		handle_get_graph(client_fd);
		return 0;
	}

	if (strcmp(req->method, "POST") == 0 && strcmp(path_only, "/research/start") == 0) {
		handle_post_research_start(client_fd, req);
		return 0;
	}

	if (strcmp(req->method, "GET") == 0 && strcmp(path_only, "/research/events") == 0) {
		handle_get_research_events(client_fd, req->path);
		return 1;
	}

	handle_not_found(client_fd);
	return 0;
}

static void handle_client(int client_fd) {
	HttpRequest req;
	int keep_open = 0;

	if (read_http_request(client_fd, &req) != 0) {
		handle_bad_request(client_fd);
		close(client_fd);
		return;
	}

	keep_open = handle_request(client_fd, &req);
	free_http_request(&req);

	if (!keep_open) {
		close(client_fd);
	}
}

/* ========================= SERVER THREAD ========================= */

static void* server_thread_main(void* arg) {
	(void)arg;

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd;

		pthread_mutex_lock(&server_lock);
		if (!started || server_fd < 0) {
			pthread_mutex_unlock(&server_lock);
			break;
		}
		pthread_mutex_unlock(&server_lock);

		client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

		if (client_fd < 0) {
			pthread_mutex_lock(&server_lock);
			if (!started) {
				pthread_mutex_unlock(&server_lock);
				break;
			}
			pthread_mutex_unlock(&server_lock);

			if (errno == EINTR) continue;

			perror("accept");
			continue;
		}

		handle_client(client_fd);
		prune_dead_sse_clients();
	}

	return NULL;
}

/* ========================= PUBLIC API ========================= */

void start_server(int port) {
	struct sockaddr_in addr;
	int opt = 1;
	int rc;

	pthread_mutex_lock(&server_lock);

	if (started) {
		pthread_mutex_unlock(&server_lock);
		stop_server();
		pthread_mutex_lock(&server_lock);
	}

	init_sse_clients();

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		pthread_mutex_unlock(&server_lock);
		cassert(0, "Server can't generate socket.\n");
		return;
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(server_fd);
		server_fd = -1;
		pthread_mutex_unlock(&server_lock);
		cassert(0, "Can't set sockopt\n");
		return;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(server_fd);
		server_fd = -1;
		pthread_mutex_unlock(&server_lock);
		cassert(0, "Failed binding the server socket\n");
		return;
	}

	if (listen(server_fd, SERVER_BACKLOG) < 0) {
		perror("listen");
		close(server_fd);
		server_fd = -1;
		pthread_mutex_unlock(&server_lock);
		cassert(0, "Failed listening at the server socket\n");
		return;
	}

	started = 1;

	rc = pthread_create(&server_thread, NULL, server_thread_main, NULL);
	if (rc != 0) {
		started = 0;
		close(server_fd);
		server_fd = -1;
		pthread_mutex_unlock(&server_lock);
		cassert(0, "Failed creating server thread\n");
		return;
	}

	pthread_mutex_unlock(&server_lock);

	printf("Server listening on http://127.0.0.1:%d\n", port);
}

void stop_server(void) {
	int local_fd = -1;
	_Bool need_join = 0;
	int i;

	pthread_mutex_lock(&server_lock);

	if (!started) {
		pthread_mutex_unlock(&server_lock);
		return;
	}

	started = 0;
	local_fd = server_fd;
	server_fd = -1;
	need_join = 1;

	pthread_mutex_unlock(&server_lock);

	if (local_fd >= 0) {
		shutdown(local_fd, SHUT_RDWR);
		close(local_fd);
	}

	if (need_join) {
		pthread_join(server_thread, NULL);
	}

	pthread_mutex_lock(&sse_clients_lock);
	for (i = 0; i < MAX_SSE_CLIENTS; i++) {
		if (sse_clients[i].alive) {
			remove_sse_client_locked(i);
		}
	}
	pthread_mutex_unlock(&sse_clients_lock);
}
