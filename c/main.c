#include <stdio.h>

#include "lib/ne/node.h"
#include "lib/ne/connection.h"
#include "lib/util.h"

int main(){
	InitNodes();

	AddNode("Viorel");
	AddNode("Claudiu");
	printf("Found : %s\n", Nodes.items[0].label);
	printf("Found : %s\n", Nodes.items[1].label);

	FreeNodes();

	return 0;
}
