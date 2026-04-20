#include "globals.h"

static struct GlobalPointerMap globalPointerMap = {0};

_Bool SetGlobalPointer(char* name, size_t name_len, void* value){
	if (!globalPointerMap.dic) return 0;
	
	int status = dic_add(globalPointerMap.dic, name, name_len);
	if (status > 0) return 0; // status 0 happy path
	
	size_t index = globalPointerMap.count;
	globalPointerMap.container[index] = value;
	*globalPointerMap.dic->value = index;

	globalPointerMap.count++;
		
	return 1;
}

void* ReadGlobalPointer(char* name, size_t name_len){
	int status = dic_find(globalPointerMap.dic, name, name_len);

	if (status == 0) return NULL; // status 1 happy path

	long index = *globalPointerMap.dic->value;

	return globalPointerMap.container[index];
}


_Bool InitGlobalPointerMap(){
	globalPointerMap.dic = dic_new(MAX_POINTERS);
	globalPointerMap.count = 0;

	if (!globalPointerMap.dic) return 0;

	return 1;
}

_Bool FreeGlobalPointerMap(){
	if (!globalPointerMap.dic) return 0;

	free(globalPointerMap.dic);
	return 1;
}
