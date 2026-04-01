#ifndef NE_NODES
#define NE_NODES

#include <stddef.h>
#include <stdint.h>
#include "../hd/hashdict.h"

#define INIT_NODE_CAP 256
#define NODE_LABEL_CAP 32
#define NODE_NBRS_CAP 4 // NEIGHBOURS

#define NODE_ACT_DECAY 0.95
#define CONN_ACT_DECAY 0.95
#define CONN_WGT_DECAY 0.99

#define NODE_ACT_INCR 0.05
#define CONN_ACT_INCR 0.05
#define CONN_WGT_INCR 0.01

#define NodeAt(i) (Nodes.items + (i))

typedef enum {
	REL_LOGICAL,
	REL_EMOTIONAL,
	REL_SOCIAL,
	REL_TEMPORAL,
	REL_CAUSAL
} RelationType;

typedef struct {
	char label[NODE_LABEL_CAP + 1];
	uint_fast8_t length;
	struct Connection *neighbours;
	double activation;
	size_t nsize;
	size_t ncount;
	size_t id;
} Node;
struct dictionary* NodeHash;

struct Connection {
	RelationType dimension;
	double activation;
	double weight;
	Node* target;
};

struct NodeContainer {
	_Bool init;
	size_t count;
	size_t capacity;
	Node* items;
} Nodes;

size_t ConnectionCount = 0;

_Bool FreeNodes();

_Bool InitNodes();

Node* AddNode(const char* label);

Node* AddNode_ex(const char* label, double activation);

Node* FindNode(const char* label, uint8_t size);

// Link A -> B
_Bool UniLink(Node* A, Node* B);
// Link A -> B and B -> A
_Bool BiLink(Node* A, Node* B);

_Bool DecayNodes();

_Bool ActivateAllConnections();

_Bool DecayAllConnections();

struct Connection* LinkExists(Node* A, Node* B);

double ReadNodeActivation(Node* n);

double ReadConnectionActivation(struct Connection* c);

double ReadConnectionWeight(struct Connection* c);

#endif
