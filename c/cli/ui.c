#include "ui.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "util.h"
#include "mockopenai.h"
#include "mocks/mocks.h"
#include "graph-export.h"
#include "input-processor.h"
#include "search/deep-search-session.h"
#include "config.h"
#include <time.h>
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
		char *input_raw = NULL;
		size_t input_size = 0;

		while (input_size < 5){
			printf("Please write a message: \n");
			input_raw = NULL; input_size = 0;

			int read = mygetline(&input_raw, &input_size, stdin);
			if (read < 0) input_size = 0;

			if (read > 0){
				String input; InitString(&input, input_size + 1);
				CatString(&input, input_raw, input_size);

				DecomposeInputIntoGraph(&input);

				FreeString(&input);
			}

			if (input_raw) free(input_raw);
		}
	}

	if (INPUT_TYPE[i] == MESSAGEX10){
		printf("Work in progress");
	}

	if (INPUT_TYPE[i] == EXPORT){
		ExportGraphTo(DEFAULT_GRAPH_EXPORT);
	}

	if (INPUT_TYPE[i] == DEEPRESEARCH){
		Task task;

		char *input_raw = NULL;
		size_t input_size = 0;

		printf("Please write a message: \n");
		input_raw = NULL; input_size = 0;

		int read = mygetline(&input_raw, &input_size, stdin);
		if (read < 0) input_size = 0;

		if (input_size < 5){
			memcpy(task.name, input_raw, input_size);
			task.name_len = input_size;
			task.minDepth = 10;
		}

		if (input_raw) free(input_raw);

		make_mock_task(&task);

		char *out = start_ds_session(&task);
		if (out){
			printf("Deep research result : \n\n%s\n", out);
			dump_to_file(PROJECT_ROOT "deep-search-result.txt", out, strlen(out));
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
