#include "deep-search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/jsonp/json.h"
#include "../lib/util/util.h"
#include "../config.h"
#include "search.h"
#include "engine.h"

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
	sprintf(path, DEFAULT_MOCK_DIRECTORY "action-data/action_%03u.json", rand() % DEFAULT_MOCK_ACTIONS_COUNT);
	return readFile(path, size);
}

static inline void cat_perc_to_buffer(String *buffer, int_fast64_t percentage){
	cassert(buffer->len + 2 < buffer->cap, "Error : Buffer Doesn't have enough memory for 2 more characters");

	if (percentage >= 100){
		CatFixed(buffer, "100");
	}else if (percentage > 9){ // 2 digits
				   // We assume preallocated memory is enough
		*(buffer->p + buffer->len) = ('0' + percentage / 10);
		*(buffer->p + buffer->len + 1) = ('0' + percentage % 10);
		buffer->len += 2;
	}else if (percentage < 10 && percentage > -1){
		*(buffer->p + buffer->len) = ('0' + percentage); 
		buffer->len++;
	}
}

static _Bool decompose_command_1_params(
		json_value *doc, String *ErrorBuff,
		int_fast64_t *percentage,
		char (*criteria)[16], size_t *criteria_length,
		String *intent
		){
	*criteria_length = 0;
	*percentage = -1;

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry entry = doc->u.object.values[i];
		if (!strcmp(entry.name, "percentage") && entry.value->type == json_integer)
			*percentage = CLAMP(0, 100, entry.value->u.integer);
		if (!strcmp(entry.name, "criteria") && entry.value->type == json_string){
			*criteria_length = MIN(entry.value->u.string.length, 15);
			if (
					strcmp(entry.value->u.string.ptr, "weight") != 0 &&
					strcmp(entry.value->u.string.ptr, "activation") != 0
			   ){
				CatFixed(ErrorBuff, "Error : \"criteria\" parameter must be \"activation\" or \"weight\".\n");
				return 0;
			};

			memcpy(*criteria, entry.value->u.string.ptr, *criteria_length);
			(*criteria)[*criteria_length] = '\0';
		}
		if (!strcmp(entry.name, "intent") && entry.value->type == json_string){
			CatString(intent, entry.value->u.string.ptr, entry.value->u.string.length);
		}
	}

	if (*percentage == -1){
		CatFixed(ErrorBuff, "\nError executing: No \"percentage\" argument passed (1-100)\n");
		return 0;
	};

	if (*criteria_length == 0){
		CatFixed(ErrorBuff, "\nError executing: You must pass a \"criteria\" parameter (\"activation\" or \"weight\").\n");
		return 0;
	};

	return 1;
}

static void run1(json_value* doc, String *dynamic_mem){
	if (!dynamic_mem || !doc) return;

	int_fast64_t percentage = -1;
	char criteria[16];
	size_t criteria_length;
	String intent; InitString(&intent, 256);
	
	if (decompose_command_1_params(doc, dynamic_mem, &percentage, &criteria, &criteria_length, &intent) == 0) return;

	size_t count = 0;

	Node** result;

	_Bool isActivation;

	if (!strcmp(criteria, "activation")){
		result = FilterNodeByActivationGlobal(percentage, &count);
		isActivation = 1;
	}else if(!strcmp(criteria, "weight")){
		result = FilterNodeByWeightGlobal(percentage, &count);
		isActivation = 0;
	}else {
		cassert(0, "You shouldn t arrive here unless the compiler is brain damaged :( \n");
	}

	if (!result){
		return;
	}

	// Allocate 35 characters per node, 32 bonus
	String data;
	InitString(&data, count * (NODE_LABEL_CAP * 2 + 32) + 32);
	cassert(data.p, "Failed to allocate memory for data.\n");

	if (*intent.p != '\0'){
		CatFixed(&data, "Model Intention : \"");
		CatString(&data, intent.p, intent.len);
		CatFixed(&data, "\"\n\nTop ");
	}else{
		CatFixed(&data, "Top ");
	}

	// convert 0-100 to string
	cat_perc_to_buffer(&data, percentage);

	char s[256];
	CatString(&data, s, sprintf(s, "%c nodes by [%s]:\nCommand Result: ", '%', criteria));

	for (size_t i = 0; i < count; i++){
		char buffer[NODE_LABEL_CAP * 2 + 32];
		size_t len = sprintf(	
				buffer, 
				"{\"name\" : \"%s\", \"%s\" : %.2f, \"parent\" : \"%s\"}\n",
				result[i]->label,
				criteria,
				isActivation ? read_node_activation(result[i]) : read_node_weight(result[i]),
				result[i]->hasParent ? NodeAt(result[i]->parent)->label : "NONE"
		       );
		CatString(&data, buffer, len);

		// count node as seen
		result[i]->times_seen ++;
	}

	CatString(dynamic_mem, c_str(&data), data.len);

	FreeString(&data);
	free(result);
}

static void print_global_context_nodes(String *buff, char *suffix){
	for (uint_fast8_t i = 0; i < CONTEXT_COUNT; i++){
		CatString(buff, NodeAt(Contexts[i])->label, NodeAt(Contexts[i])->labelLength);
		if (i != CONTEXT_COUNT - 1) CatFixed(buff, ","); else CatFixed(buff, suffix);
	}
}

static _Bool decompose_command_2_params(
		json_value *doc, String *ErrorBuff, 
		int_fast64_t *percentage, 
		char (*target)[NODE_LABEL_CAP], size_t *target_length, 
		char (*criteria)[16], size_t *criteria_length,
		int_fast64_t *context,
		String *intent){
	*percentage = -1;
	*target_length = 0;
	*criteria_length = 0;
	*context = -1;

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry entry = doc->u.object.values[i];
		if (!strcmp(entry.name, "percentage") && entry.value->type == json_integer)
			*percentage = CLAMP(0, 100, entry.value->u.integer);
		if (!strcmp(entry.name, "node") && entry.value->type == json_string){
			*target_length = MIN(entry.value->u.string.length, NODE_LABEL_CAP - 1);
			memcpy(*target, entry.value->u.string.ptr, *target_length);
			(*target)[*target_length] = '\0';
		}
		if (!strcmp(entry.name, "criteria") && entry.value->type == json_string){
			*criteria_length = MIN(entry.value->u.string.length, 15);

			if (
					strcmp(entry.value->u.string.ptr, "weight") != 0 &&
					strcmp(entry.value->u.string.ptr, "activation") != 0
			   ){
				CatFixed(ErrorBuff, "Error : \"criteria\" parameter must be \"activation\" or \"weight\".\n");
				return 0;
			};

			memcpy(*criteria, entry.value->u.string.ptr, *criteria_length);
			(*criteria)[*criteria_length] = '\0';
		}
		if (!strcmp(entry.name, "context") && entry.value->type == json_string){
			lowerAll(&entry.value->u.string.ptr, entry.value->u.string.length);
			for (uint_fast8_t i = 0; i < CONTEXT_COUNT; i++){
				if (strcmp(entry.value->u.string.ptr, NodeAt(Contexts[i])->label) == 0){
					*context = Contexts[i];
					break;
				}
			}
			if (*context == -1){
				CatFixed(ErrorBuff, "Error : You passed an invalid context. Available Contexts:\n");
				print_global_context_nodes(ErrorBuff, "\n\n");
				return 0;
			}
		}
		if (!strcmp(entry.name, "intent") && entry.value->type == json_string){
			CatString(intent, entry.value->u.string.ptr, entry.value->u.string.length);
		}
	}

	if (*percentage == -1 || *target_length == 0 || *context == -1 || *criteria_length == 0){
		CatFixed(ErrorBuff, "Error : You must pass a \"percentage\" argument (1-100), a filter \"criteria\" (activation / weight) and a \"context\" (Node parent) in command 2. Available Contexts:\n");
		print_global_context_nodes(ErrorBuff, "\n\n");
		return 0;
	};

	*percentage = CLAMP(0, 100, *percentage);

	return 1;
}

static void run2(json_value* doc, String *dynamic_mem){
	if (!doc || !dynamic_mem) return;

	int_fast64_t percentage;
	int_fast64_t context;

	char target[NODE_LABEL_CAP];
	size_t target_length;
	char criteria[16];
	size_t criteria_length;

	String intent; InitString(&intent, 256);

	if (!decompose_command_2_params(doc, dynamic_mem, &percentage, &target, &target_length, &criteria, &criteria_length, &context, &intent)) return;

	Node* node = FindNode(target, target_length, NodeAt(context));

	if (!node){
		CatFixed(dynamic_mem, "Error: Target Node not found in context. (Only context was found).\n");
		return;
	}

	node->times_used ++;

	size_t count = 0;
	Connection** result;
	_Bool isActivation;

	if (node->ncount == 0){
		CatFixed(dynamic_mem, "Node has no neighbours.\n");
		return;
	}

	if (!strcmp(criteria, "activation")){
		result = FilterNodeNeighboursByActivation(node, percentage, &count);
		isActivation = 1;
	}else{
		isActivation = 0;
		result = FilterNodeNeighboursByWeight(node, percentage, &count);
	}

	if (!result){
		CatFixed(dynamic_mem, "Internal Error : No result.\n");
		return;
	}

	String data;
	InitString(&data, count * (NODE_LABEL_CAP + 128) + 32);
	cassert(data.p, "Can't allocate memory for string here.\n");

	if (*intent.p != '\0'){
		CatFixed(&data, "Model Intention : \"");
		CatString(&data, intent.p, intent.len);
		CatFixed(&data, "\"\n\nTop ");
	}else{
		CatFixed(&data, "Top ");
	}

	cat_perc_to_buffer(&data, percentage);

	char s[256];
	size_t len = sprintf(s, "%c nodes related to [\"%s\"] by %s:\nCommand Result: ", '%', target, criteria);

	CatString(&data, s, len);

	for (size_t i = 0; i < count; i++){
		char buffer[NODE_LABEL_CAP + 128];
		Node* target = NodeAt(result[i]->target);
		double relative = isActivation ? read_connection_activation(result[i]) : read_connection_weight(result[i]);
		double local = isActivation ? read_node_activation(target) : read_node_weight(target);
		size_t len = sprintf(	
				buffer, 
				"{\"name\": \"%s\", \"connection_%s\": %.2f, \"node_%s\": %.2f}\n",
				target->label,
				criteria,
				relative,
				criteria,
				local
		       );
		cassert(len < sizeof(buffer), "Here buffer is too small\n");

		target->times_seen ++;

		CatString(&data, buffer, len);
	}
	CatFixed(&data, "\n");

	free(result);
	CatString(dynamic_mem, data.p, data.len);
	FreeString(&data);
}

static _Bool decompose_command_3_params(
	json_value* doc, String* ErrorBuff,
	char (*target)[NODE_LABEL_CAP], size_t *target_length,
	int_fast64_t *context,
	int_fast64_t *percA, int_fast64_t *percW,
	int_fast64_t *depth,
	String *intent
	){

	*target_length = 0;
	*context = -1;
	*percA = -1;
	*percW = -1;
	*depth = 3;

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry entry = doc->u.object.values[i];
		if (!strcmp(entry.name, "node") && entry.value->type == json_string){
			*target_length = MIN(entry.value->u.string.length, NODE_LABEL_CAP - 1);
			memcpy(*target, entry.value->u.string.ptr, *target_length);
			(*target)[*target_length] = '\0';
		}
		if (!strcmp(entry.name, "depth") && entry.value->type == json_integer)
			*depth = entry.value->u.integer;
		if (!strcmp(entry.name, "percA") && entry.value->type == json_integer)
			*percA = entry.value->u.integer;
		if (!strcmp(entry.name, "percW") && entry.value->type == json_integer)
			*percW = entry.value->u.integer;
		if (!strcmp(entry.name, "context") && entry.value->type == json_string){
			lowerAll(&entry.value->u.string.ptr, entry.value->u.string.length);
			for (uint_fast8_t i = 0; i < CONTEXT_COUNT; i++){
				if (strcmp(entry.value->u.string.ptr, NodeAt(Contexts[i])->label) == 0){
					*context = Contexts[i];
					break;
				}
			}
			if (*context == -1){
				CatFixed(ErrorBuff, "Error : You passed an invalid context. Available Contexts:\n");
				print_global_context_nodes(ErrorBuff, "\n\n");
				return 0;
			}
		}
		if (!strcmp(entry.name, "intent") && entry.value->type == json_string){
			CatString(intent, entry.value->u.string.ptr, entry.value->u.string.length);
		}
	}

	if ((*percW == -1 && *percA == -1) || *target_length == 0 || *context == -1){
		CatFixed(ErrorBuff, "Error : You must either pass a \"percA\" filter (1-100), either a \"percW\" filter (1-100), or both. You also need a \"context\" (Node parent) in command 3. Available Contexts:\n");
		print_global_context_nodes(ErrorBuff, "\nYou can also pass an optional \"depth\" parameter (1-5, defaults to 3)\n\n");
		return 0;
	};
	
	// defaults to full coverage
	if (*percA == -1) *percA = 100;
	if (*percW == -1) *percW = 100; 

	*percA = CLAMP(0, 100, *percA);
	*percW = CLAMP(0, 100, *percW);

	return 1;
}

static void run3(json_value* doc, String *dynamic_mem){
	if (!doc || !dynamic_mem) return;

	char target[NODE_LABEL_CAP];
	size_t target_length = 0;

	int_fast64_t percA, percW, depth, context;
	String intent; InitString(&intent, 256);

	if (!decompose_command_3_params(doc, dynamic_mem, &target, &target_length, &context, &percA, &percW, &depth, &intent)) return;
					  
	Node* node = FindNode(target, target_length, NodeAt(context));

	if (!node){
		CatFixed(dynamic_mem, "Error: Target Node not found in context. (Only context was found).\n");
		return;
	}

	size_t count = 0;

	
	size_t res_length = 0;
	char* result = ComputeNodeFamily(node, percA, percW, depth, &res_length);

	if (*intent.p != '\0'){
		CatFixed(dynamic_mem, "Model Intention : \"");
		CatString(dynamic_mem, intent.p, intent.len);
		CatFixed(dynamic_mem, "\nCommand Result: ");
	}else{
		CatFixed(dynamic_mem, "Command Result: ");
	}

	CatString(dynamic_mem, result, res_length);

	free(result);
}

static void exec_response(json_value* doc, String *dynamic_mem, size_t depth, char** conclusion, size_t *conclusionSize){

	_Bool finished = 0;
	json_value* original_conclusion = NULL;
	int_fast64_t command = -1;

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry *e = &doc->u.object.values[i];
		if (strcmp(e->name, "finished") == 0 && e->value->type == json_boolean)
			finished = e->value->u.boolean;
		if (strcmp(e->name, "conclusion") == 0 && e->value->type == json_string){
			original_conclusion = e->value;
		}
		if (strcmp(e->name, "command") == 0 && e->value->type == json_integer){
			command = e->value->u.integer;
		}
	}

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

// TODO : handle cassert
static _Bool think(DS_memory *mem, String *out, size_t depth){
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
	CatString(&mem->dynamic, header, sprintf(header, "\n\n------------- Round [%zu]:\n\n", depth));
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
				CatFixed(&mem->dynamic, "Agent tried to finish early, but minimum round number is 10.\n");
			else{
				CatString(&mem->dynamic, buff, sprintf(buff, "Agent tried to finish early, but minimum round is 10, reason : [%s]\n", conclusion));
				free(buff);
			}
		}else{
			CatString(out, conclusion, conclusionSize);

			FreeString(&prompt);
			free(response);
			json_value_free(doc);

			return 0;
		}
		free(conclusion);
	}
	
	FreeString(&prompt);
	free(response);
	json_value_free(doc);

	return 1;
}

static _Bool judge_result(){

	return 1;
}

// TODO Implement a score mechanism as a third way of searching andd the default one.
// a node value is (weight * 0.8) + (activation * 0.3)
// Or even make it in more "human" formats like variants of a * weight + b * activation
// where a and b depend on the context

char* start_ds_session(Task *task){
	DS_memory mem;
	if (massert(init_ds_memory(&mem), "Couldn't allocate memory for ds session"))
		return NULL;

	String out;
	InitString(&out, DS_OUT_MEMORY_SIZE);
	
	size_t req_space = sprintf(mem.persistent.p, DS_PERSISTENT_PROMPT, task->name);
	cassert(req_space < mem.persistent.cap - 1, "Error : Increase Persistent memory size (Macro) to solve this.\n");
	mem.persistent.len = req_space;

	RefreshGraph();

	size_t depth = 0;
#if 0   
	do {
		_Bool status = think(&mem, &out, depth++);
		if (status == 0) break;
		cassert(depth < 100, "Depth went way too high");
	} while (0);
#else
	do {
		_Bool status = think(&mem, &out, depth++);
		if (status == 0) break;
		cassert(depth < 100, "Depth went way too high");
	} while (1);
#endif

	printf("Memory : %s", mem.dynamic.p);
	free_ds_memory(&mem);

	return c_str(&out);
}
