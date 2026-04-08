#include "deep-search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/jsonp/json.h"
#include "../lib/util//util.h"

_Bool init_ds_memory(DS_memory *d){
	if (!d) return 0;

	// they auto assert
	InitString(&d->dynamic, DS_DYNAMIC_MEMORY_SIZE);
	InitString(&d->persistent, DS_PERSISTENT_MEMORY_SIZE);

	return 1;
}

// assuming d is stack allocated
void free_ds_memory(DS_memory *d){
	if (!d) return;
	FreeString(&d->persistent);
	FreeString(&d->dynamic);
}

static char* mock_ai_run(char* prompt, size_t *size){
	char path[128];
	sprintf(path, "/home/nita/dev/c/change2/mocks/action-data/%d.json", rand() % 4);
	return readFile(path, size);
}


static void exec_response(json_value* doc, String *dynamic_mem, size_t depth, char** conclusion, size_t *conclusionSize){

	_Bool finished = 0;
	json_value* original_conclusion = NULL;

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry *e = &doc->u.object.values[i];
		if (strcmp(e->name, "finished") == 0 && e->value->type == json_boolean)
			finished = e->value->u.boolean;
		if (strcmp(e->name, "conclusion") == 0 && e->value->type == json_string){
			original_conclusion = e->value;
		}
	}

	cassert(!(finished && !original_conclusion), "Error : Finished but no conclusion passed");

	if (finished && original_conclusion){
		*conclusion = malloc(original_conclusion->u.string.length + 1);
		cassert(conclusion, "Can't get memory to generate conclusion\n.");
		memcpy(*conclusion, original_conclusion->u.string.ptr, original_conclusion->u.string.length);
		(*conclusion)[original_conclusion->u.string.length] = '\0';
		*conclusionSize = original_conclusion->u.string.length;
		return;
	}

	printf("%s", dynamic_mem->p);
}

// TODO : handle cassert
static void think(DS_memory *mem, String *out, size_t depth){

	// Setup prompt
	String prompt;
	InitString(&prompt, mem->dynamic.len + mem->persistent.len + 1);
	CatString(&prompt, mem->dynamic.p, mem->dynamic.len);
	CatString(&prompt, mem->persistent.p, mem->persistent.len);

	// Get response
	size_t respsize;
	char* response = mock_ai_run(c_str(&prompt), &respsize);
	cassert(response, "Error : Coudln't read response");

	char header[32];
	CatString(&mem->dynamic, header, sprintf(header, "Round [%zu]:\n", depth));
	CatString(&mem->dynamic, response, respsize);

	// Parse JSON
	json_value *doc = json_parse(response, respsize);
	cassert(doc, "Error : Can't parse resp as json.\n");
	cassert(doc->type == json_object, "Error : json is not an object.\n");

	// check if finished
	char* conclusion = NULL;
	size_t conclusionSize = 0;
	exec_response(doc, &mem->dynamic, depth, &conclusion, &conclusionSize);

	if (conclusion){
		if (depth < 10){
			char *buff = malloc(conclusionSize + 256);
			if (!buff)
				CatStringF(&mem->dynamic, "Agent tried to finish early, but minimum round number is 10.\n");
			else{
				CatString(&mem->dynamic, buff, sprintf(buff, "Agent tried to finish early, but minimum round is 10, reason : [%s]\n", conclusion));
				free(buff);
			}
		}else{
			CatString(out, conclusion, conclusionSize);

			FreeString(&prompt);
			free(response);
			json_value_free(doc);

			return;
		}
		free(conclusion);
	}
	
	FreeString(&prompt);
	free(response);
	json_value_free(doc);

	think(mem, out, depth + 1);
}

char* start_ds_session(){
	DS_memory mem;
	if (massert(init_ds_memory(&mem), "Couldn't allocate memory for ds session"))
		return NULL;

	String out;
	InitString(&out, DS_OUT_MEMORY_SIZE);

	think(&mem, &out, 0);

	free_ds_memory(&mem);

	return c_str(&out);
}
