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

Node* AddNode(const char* label){
	if (!label || !Nodes.init || !Nodes.items) return NULL;

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
	node->length = mystrnlen(label, NODE_LABEL_CAP);

	node->nsize = NODE_NBRS_CAP;
	node->ncount = 0;
	node->id = Nodes.count;
	node->neighbours = malloc(node->nsize * sizeof(struct Connection));
	
	memcpy(node->label, label, node->length);
	node->label[node->length] = '\0';

	dic_add(NodeHash, node->label, node->length);
	*NodeHash->value = Nodes.count;

	Nodes.count ++;

	return node;
}

// Unidirectional Linkage
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
	A->neighbours[A->ncount].target = B;

	A->ncount ++;

	return 1;
}

// Bidirectional Linkage
_Bool BiLink(Node* A, Node* B){
	return UniLink(A, B) && UniLink(B, A);
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

inline Node* NodeAt(long index){return Nodes.items + index;}
