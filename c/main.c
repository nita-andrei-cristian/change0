#include <stdio.h>

#include "lib/ne/node.h"
#include "lib/util.h"

int main(){
	InitNodes();

	AddNode("Viorel");
	AddNode("Claudiu");

	Node* a = FindNode("Viorel", 6);
	Node* b = FindNode("Claudiu", 7);

	if (a)
		printf("Found : %s\n", a->label);

	UniLink(a, b);

	if (b)
		printf("Found : %s\n", b->label);

	FreeNodes();

	return 0;
}
