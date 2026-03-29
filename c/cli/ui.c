#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include "../lib/ne/node.h"
#include "../lib/ne/engine.h"
#include "../lib/util.h"

static inline void clear(){
	printf("\033[H\033[J");
}

void UIStart(){
	InitNodes();
}

static inline void WaitForInput(){
	char x;
	scanf(" %c", &x);
}

static void Run(int i){
	clear();
	printf("------------- [ %c ] -------------\n", INPUT_CHAR[i]);

	if (INPUT_TYPE[i] == MESSAGE){
		// mock something
		char path[64];
		sprintf(path, "/home/nita/dev/c/change/mocks/nodes/%d.json", rand() % 10);

		size_t size = 0;
		char* content = readFile(path, &size);
		if (content){
			if (!AddToGraph(content,size)){
				fprintf(stderr, "Couldn't add to graph the following json \n%s\n", content);
			}
			free(content);
		}
	}

	if (INPUT_TYPE[i] == MESSAGEX10){
		for (int i = 0; i < 10; i++){
			// mock something
			char path[64];
			sprintf(path, "/home/nita/dev/c/change/mocks/nodes/%d.json", i);

			size_t size = 0;
			char* content = readFile(path, &size);
			if (content){
				if (!AddToGraph(content,size)){
					fprintf(stderr, "Couldn't add to graph the following json \n%s\n", content);
				}
				free(content);
			}
		}
	}

	if (INPUT_TYPE[i] == EXPORT){
		ExportGraphTo("/home/nita/dev/c/change/js/export.json");
	}

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
