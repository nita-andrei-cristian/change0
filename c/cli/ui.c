#include "ui.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "../ne/node.h"
#include "../lib/util/util.h"
#include "../lib/openai/mockopenai.h"
#include "../ne/engine.h"
#include "../ne/deep-search-session.h"
#include "../config.h"
#include "../ne/mocks.h"
#include <unistd.h>
#include <termios.h>
#include <string.h>

// AI generated function
static int getch_nowait_enterless(void) {
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);   // save current terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // disable line buffering and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore settings
    return ch;
}

static inline void clear(){
	printf("\033[H\033[J");
}

static void SetUpContexts(){
	time_t now = time(NULL);
	for (int i = 0; i < CONTEXT_COUNT; i++){
		Node *n = AddNodeEx(context_labels[i], strlen(context_labels[i]), NODE_INIT_ACT, NODE_INIT_WGHT, PARENTLESS, -1, FERTILE, now);
		if(!n){
			fprintf(stderr, "Critical Error: Couldn't initialize contexts for neuro engine\n");
			exit(EXIT_FAILURE);
		}
		Contexts[i] = n->globalIndex;
	}
}

void UIStart(){
	InitNodes();
	SetUpContexts();
}

static void Run(int i){
	clear();
	printf("------------- [ %c ] -------------\n", INPUT_CHAR[i]);

	if (INPUT_TYPE[i] == MESSAGE){
		// mock something
		char path[64];
		sprintf(path, DEFAULT_MOCK_DIRECTORY "nodes/graph_%03u.json", rand() % DEFAULT_MOCK_NODES_COUNT);

		char *input = NULL;
		size_t input_size = 0;

		int read = mygetline(&input, &input_size, stdin);

		if (input){
			DecomposeInputIntoGraph(input, input_size);
			free(input);
		}
	}

	if (INPUT_TYPE[i] == MESSAGEX10){
		for (int i = 0; i < DEFAULT_MOCK_NODES_COUNT; i++){
			// mock something
			char path[64];
			sprintf(path, DEFAULT_MOCK_DIRECTORY "nodes/graph_%03u.json", i);

			size_t size = 0;
			char* content = readFile(path, &size);
			if (content){
				if (!AddContextNodesFromJSON(content,size)){
					fprintf(stderr, "Couldn't add to graph the following json \n%s\n", content);
				}
				free(content);
			}
		}
	}

	if (INPUT_TYPE[i] == EXPORT){
		ExportGraphTo(DEFAULT_GRAPH_EXPORT);
	}

	if (INPUT_TYPE[i] == DEEPRESEARCH){
		Task task;

		make_mock_task(&task);

		char *out = start_ds_session(&task);
		if (out){
			printf("Deep research result : \n\n%s\n", out);
			free(out);
		}
	}

	if (INPUT_TYPE[i] == REGEN_OPENAI)
		RegenMocksOpenAI();

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

		char option = (char)getch_nowait_enterless();
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
