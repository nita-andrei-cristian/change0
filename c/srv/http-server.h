#ifndef HTTP_SERVER_HEADER
#define HTTP_SERVER_HEADER

#include <stddef.h>

void start_server(int port);
void stop_server();
int server_is_running(void);
void ds_emit_event(const char* id, const char* type, const char* buffer, size_t buffer_len);

#endif
