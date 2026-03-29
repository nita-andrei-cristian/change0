#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "node.h"
#include "../util.h"

_Bool FreeNodes(){
	if (Nodes.items != NULL){
		free(Nodes.items);
		Nodes.items = NULL;
	}
	dic_delete(NodeHash);
	Nodes.capacity = 0;
	Nodes.count = 0;
	Nodes.init = 0;
	return 1;
}

_Bool InitNodes(){
	if (Nodes.init) FreeNodes();

	NodeHash = dic_new(INIT_NODE_CAP);
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

// Danger: Node must not exist! Always verify with FindNode
Node* AddNode(const char* label){
	if (!label || !Nodes.init || !Nodes.items) return NULL;
	size_t label_len = mystrnlen(label, NODE_LABEL_CAP);

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
	node->length = label_len;
	node->nsize = NODE_NBRS_CAP;
	node->ncount = 0;
	node->id = Nodes.count;
	node->neighbours = malloc(node->nsize * sizeof(struct Connection));
	node->activation = 1;
	
	memcpy(node->label, label, node->length);
	node->label[node->length] = '\0';

	dic_add(NodeHash, node->label, node->length);
	*NodeHash->value = Nodes.count;

	Nodes.count ++;

	return node;
}

Node* AddNode_ex(const char* label, double activation){
	size_t label_len = mystrnlen(label, NODE_LABEL_CAP); // recomputing length, yes
	Node* out = FindNode(label, label_len);
	if (out) {
		out->activation += NODE_ACT_INCR * activation; // relative to 1
		return out;
	}
	out = AddNode(label);
	out->activation = activation;

	return out;
}

struct Connection* LinkExists(Node* A, Node* B){
	// scan if A already contains B
	// TODO make this faster than O(A neighbours)
	
	for (size_t i = 0; i < A->ncount; i++){
		if (A->neighbours[i].target == B){
			// HIT
			// TODO when adding a new connection, increase activation based on its initial activation
			A->neighbours[i].activation += CONN_ACT_INCR;
			A->neighbours[i].weight += CONN_WGT_INCR;
			return &(A->neighbours[i]);
		};
	}

	return NULL;
}

// Unidirectional Linkage
// Danger: Always check the Link doesn't exist to avoid overriding
_Bool UniLink(Node* A, Node* B){
	if (!A || !B) return 0;
	

	long a = A->ncount, b = B->ncount;

	if (a >= A->nsize){
		size_t new_capacity = MAX(NODE_NBRS_CAP, A->nsize * 2);
		struct Connection* tmp = (struct Connection*)realloc(A->neighbours, new_capacity * sizeof(struct Connection));
		//struct Connection* tmp = (struct Connection*)realloc(A->neighbours, 50);
		if (!tmp){
			fprintf(stderr, "Failed to allocate memory for node neighbour\n");
			return 0;
		}
		A->neighbours = tmp;
		A->nsize = new_capacity;
	}

	A->neighbours[A->ncount].activation = 1.0;
	A->neighbours[A->ncount].weight = 1.0;
	A->neighbours[A->ncount].target = B;

	A->ncount ++;

	ConnectionCount ++;

	return 1;
}

// Bidirectional Linkage
_Bool BiLink(Node* A, Node* B){
	return UniLink(A, B) && UniLink(B, A);
}

_Bool DecayNodes(){
	for (size_t i = 0; i < Nodes.count; i++){
		NodeAt(i)->activation *= NODE_ACT_DECAY;
	}
	return 1;
}

_Bool ActivateAllConnections(){
	for (size_t i = 0; i < Nodes.count; i++){
		Node* n = NodeAt(i);
		n->neighbours[i].weight += CONN_WGT_INCR;
		n->neighbours[i].activation += CONN_ACT_INCR;
	}
	return 1;
}

_Bool DecayAllConnections(){
	for (size_t i = 0; i < Nodes.count; i++){
		Node* n = NodeAt(i);
	
		for (size_t j = 0; j < n->ncount; j++){
			n->neighbours[j].weight *= CONN_WGT_DECAY;
			n->neighbours[j].activation *= CONN_ACT_DECAY;
		}
	}
	return 1;
}


Node* FindNode(const char* label, uint8_t size){
	if (!Nodes.init || !Nodes.items) return NULL;
	if (dic_find(NodeHash, (void*)label, size)){
		long index = *NodeHash->value;
		if (index < 0 || index >= Nodes.count) return NULL;
		return NodeAt(index);
	}
	return NULL;
}
