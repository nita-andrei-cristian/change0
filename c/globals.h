#ifndef CHANGE_GLOBALS_CONFIG_HEADER
#define CHANGE_GLOBALS_CONFIG_HEADER

#include "hashdict.h"

#define MAX_POINTERS 10
#ifndef FSIZE
#define FSIZE(fixed) (sizeof((fixed)) - 1)
#endif

_Bool SetGlobalPointer(char* name, size_t name_len, void* value);
void* ReadGlobalPointer(char* name, size_t name_len);

#define SetGlobalPointerF(name, value) (SetGlobalPointer((name), FSIZE((name)), (value)))
#define ReadGlobalPointerF(name) (ReadGlobalPointer((name), FSIZE((name))))

typedef _Bool (*ds_emit_like_func)(const char* id, const char *type, const char *buffer, size_t buffer_len);

struct GlobalPointerMap{
	struct dictionary* dic;
	void* container[MAX_POINTERS];
	long count;
};

_Bool InitGlobalPointerMap();

_Bool FreeGlobalPointerMap();

#endif
