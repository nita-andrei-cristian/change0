#include "deep-search-session.h"
#include "command-parsing.h"
#include "deep-search-execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/openai/openai.h"
#include "graph-engine.h"

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

static void write_round_header(DS_memory* mem, size_t depth){
	char header[32];
	CatString(&mem->dynamic, header, sprintf(header, "\n\n------------- Round [%zu]:\n\n", depth));
}

static void fail_finish(DS_memory *mem, String *conclusion, size_t depth){
	char *buff = malloc(conclusion->len + 256);
	cassert(buff, "Can't allocate quick memory to fill a buffer");

	size_t size = sprintf(buff, "Agent tried to finish early, but minimum round is 10, reason : [%s]\n", c_str(conclusion));

	CatString(&mem->dynamic, buff, size);
	free(buff);
}

_Bool try_terminate(DS_memory *mem, String *out, size_t depth, String *conclusion){

	if (conclusion->len > 0){
		if (depth < 10){
			fail_finish(mem, conclusion, depth);
			return 0; // did not finish
		}else{
			CopyString(out, conclusion);
			return 1; // did finish
		}
	}

	return 0; // did not finish
}

static void write_feedback(String* mem, String* reason){
	char* buffer = malloc(reason->len + 1024);
	cassert(buffer, "Can't allocate memory for writing feedback");

	size_t len = sprintf(buffer, "{ Server Intervention :  You had previously tried to execute this task, but failed the automatic validation, here is feedback: [%s]. You may repeat the round with this hint. }", c_str(reason));

	EmptyString(mem);

	mem->len = 0;
	CatString(mem, buffer, len);

	free(buffer);
}

static _Bool judge_result(String *out, String* reason, Task *task){
	size_t len = 0; 

	_Bool pass = 0;
	_Bool received_pass = 0;

	json_value *root = call_gpt_judge(out, task);
	cassert(root, "Judge root is NULL.\n");

	char *json_raw = openai_extract_output_text_dup(root, &len);
	cassert(json_raw, "Failed to extract judge JSON text.\n");
	cassert(len > 0, "Judge returned empty JSON text.\n");

	json_value *json_parsed = json_parse(json_raw, len);
	cassert(json_parsed, "Judge result isn't JSON.\n");
	cassert(json_parsed->type == json_object, "Judge JSON is not an object.\n");

	parse_judge_result(json_parsed, reason, &received_pass, &pass);
	cassert(received_pass, "Judge JSON didn't provide a pass field.\n");

	json_value_free(json_parsed);
	json_value_free(root);
	free(json_raw);

	if (pass) return 1;

	printf("Judge reason : %s", c_str(reason));

	cassert(reason->len, "No provided reason.\n");
	return 0;
}

// TODO Implement a score mechanism as a third way of searching andd the default one.
// a node value is (weight * 0.8) + (activation * 0.3)
// Or even make it in more "human" formats like variants of a * weight + b * activation
// where a and b depend on the context

static _Bool think(DS_memory *mem, String *out, size_t depth){

	size_t respsize;
	char* response = call_gpt_deepsearch(mem, &respsize);

	printf("Response : %s\n\n", response);

	write_round_header(mem, depth);
	CatString(&mem->dynamic, response, respsize);

	// Parse JSON
	json_value *doc = json_parse(response, respsize);
	cassert(doc, "Error : Can't parse resp as json.\n");
	cassert(doc->type == json_object, "Error : json is not an object.\n");


	// check if finished

	String conclusion; InitString(&conclusion, 1024);
	exec_response(doc, &mem->dynamic, depth, &conclusion);

	_Bool terminated = try_terminate(mem, out, depth, &conclusion);
	
	free(response);
	json_value_free(doc);
	FreeString(&conclusion);

	return terminated;
}

char* start_ds_session(Task *task){
	DS_memory mem;
	if (massert(init_ds_memory(&mem), "Couldn't allocate memory for ds session"))
		return NULL;

	String out;
	InitString(&out, 1024);
	
	ResizeString(&mem.persistent, sizeof(DS_PERSISTENT_PROMPT) + task->name_len + 1);

	size_t req_space = sprintf(mem.persistent.p, DS_PERSISTENT_PROMPT, task->name);

	if (req_space >= mem.persistent.cap){
		fprintf(stderr, "Req size : %zu, Mem Size : %zu", req_space, mem.persistent.cap);
		cassert(0, "Error : Increase Persistent memory size (Macro) to solve this.\n");
	}
	mem.persistent.len = req_space;

	RefreshGraph();
	
	// internal external depth
	size_t idepth = 1, edepth = 1;
	String reason; InitString(&reason, 1024);

	while (edepth++){
		cassert(edepth < 10, "Error : External Depth went way too hight\n");

		idepth = 1;
		while(idepth++) {
			_Bool status = think(&mem, &out, idepth);
			if (status == 1) break;
			cassert(idepth < 100, "Error : Internal Depth went way too high\n");
		}

		_Bool success = judge_result(&out, &reason, task);
		if (success) break;


		// failed task 
		write_feedback(&mem.dynamic, &reason);
	}

	printf("[debug] Persistent Memory : %s", mem.persistent.p);
	printf("[debug] Dynamic Memory : %s", mem.dynamic.p);

	FreeString(&reason);
	free_ds_memory(&mem);

	return c_str(&out); // might look sus but only the pointer matters
}
