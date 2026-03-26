#ifndef NE_NODES
#define NE_NODES

#include <stddef.h>
#include <stdint.h>
#include "../hd/hashdict.h"

#define INIT_NODE_CAP 256
#define NODE_LABEL_CAP 32
#define NODE_NBRS_CAP 4 // NEIGHBOURS

typedef struct {
	char label[NODE_LABEL_CAP + 1];
	uint_fast8_t length;
	long *neighbours;
	long nsize;
	long ncount;
	long id;
} Node;
struct dictionary* NodeHash;

struct NodeContainer {
	_Bool init;
	size_t count;
	size_t capacity;
	Node* items;
} Nodes;

_Bool FreeNodes();

_Bool InitNodes();

Node* AddNode(const char* label);

Node* FindNode(const char* label, uint8_t size);

Node* NodeAt(long index);

// Link A -> B
_Bool UniLink(Node* A, Node* B);
// Link A -> B and B -> A
_Bool BiLink(Node* A, Node* B);

#endif
