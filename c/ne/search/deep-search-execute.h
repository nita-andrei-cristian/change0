#ifndef DEEP_SEARCH_EXECUTE_C
#define DEEP_SEARCH_EXECUTE_C

#include "deep-search-session.h"
#include "json.h"
#include "util.h"

void exec_response(json_value* doc, String *dynamic_mem, size_t depth, String* conclusion);

char* process_ai_call(DS_memory *mem, size_t *respsize);

#endif
