#include "mocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "node.h"

char* mock_ai_action(char* prompt, size_t *size){
	char path[128];
	sprintf(path, DEFAULT_MOCK_DIRECTORY "action-data/action_%03u.json", rand() % DEFAULT_MOCK_ACTIONS_COUNT);
	return readFile(path, size);
}

void make_mock_task(Task* task){
	char str[] = "Help Mark become rich.";

	CatString(&task->name, FSTRING_SIZE_PARAMS(str));
	task->minDepth = 10;
}

char* make_mock_judge(String *out, size_t *len){
	char* s = malloc(1024);
	cassert(s, "couldn t allocate memory for mocking a judge\n");

	*len = sprintf(s, "{\"pass\" : true}");

	return s;
}
