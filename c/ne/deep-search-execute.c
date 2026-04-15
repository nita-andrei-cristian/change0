#include "deep-search-session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai-action.h"
#include "command-parsing.h"

void exec_response(json_value* doc, String *dynamic_mem, size_t depth, char** conclusion, size_t *conclusionSize){

	_Bool finished = 0;
	json_value* original_conclusion = NULL;
	int_fast64_t command = -1;
	
	parse_exec_response(doc, &finished, &original_conclusion, &command);

	cassert(!(finished && !original_conclusion), "Error : Finished but no conclusion passed");

	if (finished && original_conclusion){
		*conclusion = malloc(original_conclusion->u.string.length + 1);
		cassert(*conclusion, "Can't get memory to generate conclusion\n.");
		memcpy(*conclusion, original_conclusion->u.string.ptr, original_conclusion->u.string.length);
		(*conclusion)[original_conclusion->u.string.length] = '\0';
		*conclusionSize = original_conclusion->u.string.length;
		return;
	}

	if (command == 1)
		run1(doc, dynamic_mem);
	if (command == 2)
		run2(doc, dynamic_mem);
	if (command == 3)
		run3(doc, dynamic_mem);
}

