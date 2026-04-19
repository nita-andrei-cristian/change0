#include "http-server.h"
#include "graph/graph-export.h"
#include <sys/socket.h>

static _Bool started = 0;
static int server_fd = -1;

char* get_graph_data() {
    return SeriliazeGraph();
}

void start_server(int port) {
    if (started) return;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // bind
    // listen
    // loop accept
    
    started = 1;
}

void stop_server() {
    if (!started) return;

    // close(server_fd);
    server_fd = -1;
    started = 0;
}
