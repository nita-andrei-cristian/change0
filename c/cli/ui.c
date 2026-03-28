#include "ui.h"
#include <stdio.h>
#include "../lib/ne/node.h"
#include "../lib/ne/search.h"

static inline void clear(){
	printf("\033[H\033[J");
}

void UIStart(){
	InitNodes();

	Node* a = AddNode("Viorel");
	Node* b = AddNode("Claudiu");
	Node* c = AddNode("Cicadrel");
	Node* d = AddNode("Busuioc");
	Node* e = AddNode("Aurelian");
	Node* f = AddNode("Maximus");

	if (a)
		printf("Found : %s\n", a->label);

	UniLink(a, b);
	BiLink(a, c);
	UniLink(a, d);
	UniLink(a, e);
	UniLink(a, f);
	
	struct Connection* neighbours = FindNeighbours(a);
	for (int i = 0; i < a->ncount; i++){
		printf("Found neighbour [%s] : [%f]\n", neighbours[i].target->label, neighbours[i].activation);
	}

	if (b)
		printf("Found : %s\n", b->label);


}

static inline void WaitForInput(){
	char x;
	scanf(" %c", &x);
}



static void Run(int i){
	clear();
	printf("------------- [ %c ] -------------\n", INPUT_CHAR[i]);
	WaitForInput();
}

void UILoop(){
	int selection = -1;
	while(1){
		clear();
		printf("CHANGE APP CLI\n\n");

		if (selection >= 0 && selection < OPTIONS_COUNT){
			printf("Ran %c\n\n", INPUT_CHAR[selection]);
		}

		for (int i = 0; i < OPTIONS_COUNT; ++i){
			printf("[%c] - %s\n", INPUT_CHAR[i], INPUT_MSG[i]);
		}
		printf("\n\nSelect any option above: ");

		char option;
		scanf(" %c", &option);
		for (selection = 0; selection < OPTIONS_COUNT; ++selection){
			if (option == QUIT_BUTTON) return;
			if (option == INPUT_CHAR[selection]) Run(INPUT_TYPE[selection]);
		}
		if (selection == OPTIONS_COUNT) continue;


	}
}

void UIKill(){
	FreeNodes();
}
