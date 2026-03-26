#include <stddef.h>

#define INIT_CONNECTION_CAP 256

typedef struct {

} Connection;
struct ConnectionContainer {
	_Bool init;
	size_t count;
	size_t capacity;
 	Connection* items;
} Connections;

