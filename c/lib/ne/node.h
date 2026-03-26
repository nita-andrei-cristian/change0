#include <stddef.h>
#include <stdint.h>

#define INIT_NODE_CAP 256
#define NODE_LABEL_CAP 32

typedef struct {
	char label[NODE_LABEL_CAP + 1];
	uint_fast8_t label_size;

} Node;

struct NodeContainer {
	_Bool init;
	size_t count;
	size_t capacity;
	Node* items;
} Nodes;

_Bool FreeNodes();

_Bool InitNodes();

Node* AddNode(const char* label);
