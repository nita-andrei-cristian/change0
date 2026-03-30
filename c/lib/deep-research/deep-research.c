#include "deep-research.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../util.h"
#include "../jsonp//json.h"

static _Bool NewContext(Context *c){
	c->len = 0;
	c->capacity = CONTEXT_INIT_SIZE;

	c->payload = malloc(CONTEXT_INIT_SIZE);
	if (!c->payload){
		fprintf(stderr, "Error : Failed to malloc context payload\n");
		return 0;
	}
	*c->payload = '\0';

	c->round = 0;
	c->minDepth = 4;
	c->maxDepth = 8;

	return 1;
}

static _Bool AddContextEx(Context *c, char* end, size_t len){

	if (len + c->len >= c->capacity){
		// increase
		size_t newCapacity = c->capacity;
		while(len + c->len >= newCapacity){
			newCapacity *= 2;
		}

		char* tmp = realloc(c->payload, newCapacity);
		if (!tmp){
			fprintf(stderr, "Error : Failed to increase context size");
			return 0;
		}
		c->payload = tmp;
		c->capacity = newCapacity;
	}

	memcpy(c->payload + c->len, end, len);
	c->len += len;
	*(c->payload + c->len) = '\0';

	return 1;
}

static _Bool AddContext(Context *c, char* end){
	return AddContextEx(c, end, strlen(end));
}

static char* MockResponse(size_t *size){
	char path[64];
	sprintf(path, "/home/nita/dev/c/change/mocks/action-data/%d.json", rand() % 4);
	return readFile(path, size);
}

char* DeepResearchStart(){
	Context context;
	NewContext(&context);

	while (1){
		enum DEEP_RESEARCH_EXIT status = DeepResearchLoop(&context);
		
		if (status == FINISH || status == MAX_LIMIT) break;
		//if (status == COMMON_FAIL) break;
	}

	AddContextEx(&context, "\n------------------ JOB END\n\n\n", FSIZE("\n------------------ JOB END\n\n\n"));
	
	return context.payload;
}

// Commands 1-4

enum DEEP_RESEARCH_EXIT Run1(Context *context, json_value *document){
	AddContext(context, "[[Command 1 inner contents]]\n");
	return NEXT_STEP;
}

enum DEEP_RESEARCH_EXIT Run2(Context *context, json_value *document){
	AddContext(context, "[[Command 2 inner contents]]\n");
	return NEXT_STEP;
}

enum DEEP_RESEARCH_EXIT Run3(Context *context, json_value *document){
	AddContext(context, "[[Command 3 inner contents]]\n");
	return NEXT_STEP;
}

enum DEEP_RESEARCH_EXIT Run4(Context *context, json_value *document){
	AddContext(context, "[[Command 4 inner contents]]\n");
	return NEXT_STEP;
}

enum DEEP_RESEARCH_EXIT DeepResearchLoop(Context *context){
	if (!context) return COMMON_FAIL;

	char s[32];

	context->round ++;

	if (context->round > context->maxDepth){
		return MAX_LIMIT;
	}

	AddContextEx(context, s, sprintf(s, "Deep research round %zu:\n\n", context->round));

	char *response;
	size_t responseSize;

	response = MockResponse(&responseSize);

	if (!response) {
		AddContextEx(context, RESPONSE_NULL_ERROR, FSIZE(RESPONSE_NULL_ERROR));
		return COMMON_FAIL;
	};

	json_value* document = json_parse(response, responseSize);
	if (!document){
		AddContextEx(context, RESPONSE_FAIL_PARSE, FSIZE(RESPONSE_FAIL_PARSE));
		free(response);
		return COMMON_FAIL;
	}

	if (document->type != json_object || document->u.object.length == 0){
		AddContextEx(context, JSON_INVALID_CHECKS, FSIZE(JSON_INVALID_CHECKS));
		free(response);
		free(document);
		return COMMON_FAIL;
	}

	// Read entries
	for (size_t i = 0; i < document->u.object.length; i++){
		json_object_entry entry = document->u.object.values[i];

		// AI Wants to finish
		if (	
			!strcmp(entry.name, "finished") && 
			(
			 (entry.value->type == json_boolean && entry.value->u.boolean) ||
			 (entry.value->type == json_integer && entry.value->u.integer)
			)
		    ){
			char s[128];

			if (context->round >= context->minDepth){
				AddContextEx(context, s, sprintf(s, "AI ended successfully after %zu round(s)\n", context->round));
				return FINISH;
			}

			AddContextEx(context, s, sprintf(s, "AI tried to finish at round [%zu], but minimum round is [%zu]\n", context->round, context->minDepth));

			break;
		}

		if (!strcmp(entry.name, "command")){
			json_value *found = entry.value;
			
			if (found->type != json_integer) continue;

			uint_fast64_t value = found->u.integer;

			enum DEEP_RESEARCH_EXIT (*fn)(Context* a, json_value* b);

			fn = NULL;

			if (value == 1)
				fn = Run1;
			else if (value == 2)
				fn = Run2;
			else if (value == 3)
				fn = Run3;
			else if (value == 4)
				fn = Run4;

			if (!fn) continue;

			enum DEEP_RESEARCH_EXIT status = fn(context, document);

			if (status == COMMON_FAIL) {
				AddContextEx(context, "Error : Command failed", FSIZE("Error : Command failed"));
				return status;
			}
			
			AddContextEx(context, "Ran command : ", FSIZE("Ran command : "));
			AddContextEx(context, response, responseSize);
		}

	}

	json_value_free(document);
	free(response);
	return NEXT_STEP;
}
