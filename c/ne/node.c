#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "node.h"
#include "../lib/util/util.h"

_Bool FreeNodes(){
	if (Nodes.items != NULL){
		free(Nodes.items);
		Nodes.items = NULL;
	}
	Nodes.capacity = 0;
	Nodes.count = 0;
	Nodes.init = 0;

	return 1;
}

_Bool InitNodes(){
	if (Nodes.init) FreeNodes();

	Nodes.capacity = INIT_NODE_CAP;
	Nodes.count = 0;
	Nodes.items = (Node*)malloc(sizeof(Node) * INIT_NODE_CAP);

	if (Nodes.items == NULL) {
		Nodes.capacity = 0;
		Nodes.init = 0;
		return 0;
	}

	Nodes.init = 1;
	return 1;
}

// Parent can be Nullable
Node* AddNodeEx(char* label, size_t label_len, double activation, double weight, _Bool hasParent, size_t parent, _Bool fertile){
	if (!label || !Nodes.init || !Nodes.items) return NULL;
	if (label_len > NODE_LABEL_CAP - 2) label[NODE_LABEL_CAP-1] = '\0';

	if (Nodes.count >= Nodes.capacity) {
		size_t new_capacity = MAX(INIT_NODE_CAP,Nodes.capacity) * 2;
		Node* tmp = realloc( Nodes.items, sizeof(Node) * new_capacity);
		if (tmp == NULL){
			fprintf(stderr, "Error: Couldn't allocate more memory to add nodes");
			return NULL;
		};
		Nodes.items = tmp;
		Nodes.capacity = new_capacity;
	}

	Node* node = NodeAt(Nodes.count);
	node->labelLength = label_len;
	node->nsize = NODE_NBRS_CAP;
	node->ncount = 0;
	node->globalIndex = Nodes.count;
	node->neighbours = malloc(node->nsize * sizeof(Connection));
	node->activation = NODE_INIT_ACT;
	node->weight = NODE_INIT_WGHT;
	node->hasParent = 0;
	
	memcpy(node->label, label, node->labelLength);
	node->label[node->labelLength] = '\0';

	if (hasParent && NodeExists(parent)){
		dic_add(NodeAt(parent)->childrenIndex, node->label, node->labelLength);
		*(NodeAt(parent)->childrenIndex->value) = node->globalIndex;
		node->parent = parent;
		node->hasParent = 1;
	}

	if (fertile){
		node->childrenIndex = dic_new(INIT_CHILDREN_COUNT);
	}else{
		node->childrenIndex = NULL;
	}

	Nodes.count ++;

	return node;
}

Node* AddInfertileNodeInParent(char* label, size_t label_len, size_t parent){
	return AddNodeEx(label, label_len, NODE_INIT_ACT, NODE_INIT_WGHT, 1, parent, 0);
}

Connection* LinkExists(Node* A, Node* B){
	// scan if A already contains B
	// TODO make this faster than O(A neighbours)
	
	for (size_t i = 0; i < A->ncount; i++){
		if (A->neighbours[i].target == B->globalIndex){
			// HIT
			// TODO when adding a new connection, increase activation based on its initial activation
			
			/*
			A->neighbours[i].activation += CONN_ACT_INCR;
			A->neighbours[i].weight += CONN_WGT_INCR;
			*/
			return &(A->neighbours[i]);
		};
	}

	return NULL;
}

// Unidirectional Linkage
// Danger: Always check the Link doesn't exist to avoid overriding
_Bool UniLinkEx(Node* A, Node* B, double activation, double weight){
	if (!A || !B) return 0;

	long a = A->ncount;

	if (a >= A->nsize){
		size_t new_capacity = MAX(NODE_NBRS_CAP, A->nsize * 2);
		Connection* tmp = (Connection*)realloc(A->neighbours, new_capacity * sizeof(Connection));
		if (!tmp){
			fprintf(stderr, "Failed to allocate memory for node neighbour\n");
			return 0;
		}
		A->neighbours = tmp;
		A->nsize = new_capacity;
	}

	A->neighbours[A->ncount].activation = activation;
	A->neighbours[A->ncount].weight = weight;
	A->neighbours[A->ncount].target = B->globalIndex;

	A->ncount ++;

	ConnectionCount ++;

	return 1;
}

_Bool UniLink(Node* A, Node* B){
	return UniLinkEx(A, B, NODE_INIT_ACT, NODE_INIT_WGHT);
}

// Bidirectional Linkage
_Bool BiLink(Node* A, Node* B){
	return UniLink(A, B) && UniLink(B, A);
}

_Bool BiLinkEx(Node* A, Node* B, double activation, double weight){
	return UniLinkEx(A, B, activation, weight) && UniLinkEx(B, A, activation, weight);
}

Node* FindNode(char* target, uint_fast8_t length, Node* parent){
	if (parent == NULL || target == NULL || length == 0) return NULL;

	if (dic_find(parent->childrenIndex, (void*)target, length)){
		long index = *parent->childrenIndex->value;
		if (index < 0 || index >= Nodes.count) return NULL;
		return NodeAt(index);
	}
	return NULL;
}
