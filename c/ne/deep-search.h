#ifndef DEEP_SEARCH_C
#define DEEP_SEARCH_C

#include <stddef.h>
#include "../lib/util/util.h"

#define DS_PERSISTENT_MEMORY_SIZE 2048
#define DS_DYNAMIC_MEMORY_SIZE 2048
#define DS_OUT_MEMORY_SIZE 2048

#define DS_PERSISTENT_PROMPT "|System : This is a workspace, You will help the user to : [%s]. You will produce a JSON that contains blah blah blah||User : Hi, my name is Mark||AI : "

typedef struct {
	String dynamic;
	String persistent;
} DS_memory;

typedef struct {
	char name[2048];
	size_t minDepth;
} Task;

_Bool init_ds_memory(DS_memory *d);
void free_ds_memory(DS_memory *d);

char* start_ds_session(Task* task);

#endif
