#ifndef MOCKS_IMPLEMENTATION
#define MOCKS_IMPLEMENTATION

#include <stddef.h>

#include "node.h"
#include "util.h"

char* mock_ai_action(char* prompt, size_t *size);

void make_mock_task(Task* task);

char* make_mock_judge(String *out, size_t *len);

#endif
