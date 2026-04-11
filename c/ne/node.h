#ifndef NE_NODES
#define NE_NODES

#include <stddef.h>
#include <stdint.h>
#include "time.h"
#include "../lib/hd/hashdict.h"

#define CONTEXT_COUNT 5

#define INIT_NODE_CAP 16
#define NODE_LABEL_CAP 32
#define NODE_NBRS_CAP 4 // NEIGHBOURS

#define NODE_ACT_DECAY 0.95
#define NODE_WGHT_DECAY 0.99
#define CONN_ACT_DECAY 0.95
#define CONN_WGT_DECAY 0.99

#define NODE_ACT_INCR 0.5
#define NODE_WGHT_INCR 0.01
#define CONN_ACT_INCR 0.5
#define CONN_WGHT_INCR 0.01

#define ACT_HALFTIME 100.0

#define NODE_INIT_ACT 1
#define NODE_INIT_WGHT 1

#define CONN_INIT_ACT 1
#define CONN_INIT_WGHT 1

#define INIT_CHILDREN_COUNT 256

#define NodeAt(i) (Nodes.items + (i))
#define NodeExists(i) ((i) < Nodes.count)

#define INFERTILE 0
#define FERTILE 1
#define HASPARENT 1
#define PARENTLESS 0

typedef struct NodeType {
	char label[NODE_LABEL_CAP];
	_Bool hasParent;
	uint_fast8_t labelLength;

	double weight;
	double activation;

	struct ConnectionType *neighbours;
	size_t nsize, ncount, globalIndex;

	time_t lastAccessedActivation;
	time_t lastAccessedWidth;
	uint_fast32_t pendingActivationTouches;
	uint_fast32_t pendingWeightTouches;

	size_t parent;
	struct dictionary *childrenIndex;
} Node;

typedef struct ConnectionType {
	double activation;
	double weight;
	time_t lastAccessedActivation;
	time_t lastAccessedWidth;
	uint_fast32_t pendingActivationTouches;
	uint_fast32_t pendingWeightTouches;
	size_t target;
	size_t source;
} Connection;

struct {
	Node* items;
	size_t capacity;
	size_t count;

	_Bool init;
} Nodes;

_Bool InitNodes();
_Bool FreeNodes();

Node* FindNode(char* target, uint_fast8_t length, Node* parent);

Node* AddNodeEx(
		char* label,
		size_t label_len,
		double activation,
		double weight,
		_Bool hasParent,
		size_t parent,
		_Bool fertile,
		time_t now
	       );

Connection* LinkExists(Node* A, Node* B);

_Bool UniLinkEx(Node* A, Node* B, double activation, double weight);
_Bool UniLink(Node* A, Node* B);
_Bool BiLink(Node* A, Node* B);
_Bool BiLinkEx(Node* A, Node* B, double activation, double weight);

size_t ConnectionCount = 0;

char context_labels[CONTEXT_COUNT][NODE_LABEL_CAP] = {
	"profesie",
	"emotie",
	"pasiuni",
	"generalitati",
	"subiectiv",
};
size_t Contexts[CONTEXT_COUNT];

double readNodeActivation(Node* n);
double readNodeWeight(Node* n);
double readConnectionActivation(Connection* c);
double readConnectionWeight(Connection* c);

void touch_node(Node *n, uint_fast8_t, time_t now);
void touch_connection(Connection *c, uint_fast8_t, time_t now);

#endif
