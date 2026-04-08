#ifndef DEEP_SEARCH_C
#define DEEP_SEARCH_C

#include <stddef.h>
#include "../lib/util/util.h"

#define DS_PERSISTENT_MEMORY_SIZE 2048
#define DS_DYNAMIC_MEMORY_SIZE 2048
#define DS_OUT_MEMORY_SIZE 2048

typedef struct {
	String dynamic;
	String persistent;
} DS_memory;

_Bool init_ds_memory(DS_memory *d);
void free_ds_memory(DS_memory *d);

char* start_ds_session();

#endif
