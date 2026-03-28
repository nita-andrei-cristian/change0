#include "search.h"
#include "node.h"

struct Connection* FindNeighbours(Node* node){
	return node->neighbours;
}
