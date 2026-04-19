#include "http-server.h"
#include "node.h"
#include "graph/graph-export.h"

_Bool started = 0;

void start_server(){
	started = 1;
}

void stop_server(){
	started = 0;
}
