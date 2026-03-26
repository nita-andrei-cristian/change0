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

	Node* node = Nodes.items + Nodes.count;
	node->label_size = mystrnlen(label, NODE_LABEL_CAP);
	
	memcpy(node->label, label, node->label_size);
	node->label[node->label_size] = '\0';

	Nodes.count ++;

	return node;
}

