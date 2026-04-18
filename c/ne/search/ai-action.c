#include "ai-action.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"
#include "command-parsing.h"

void run1(json_value* doc, String *dynamic_mem){
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
		FreeString(&intent);
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

void run2(json_value* doc, String *dynamic_mem){
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
		FreeString(&intent);
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
		FreeString(&intent);
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

void run3(json_value* doc, String *dynamic_mem){
	if (!doc || !dynamic_mem) return;

	char target[NODE_LABEL_CAP];
	size_t target_length = 0;

	int_fast64_t percA, percW, depth, context;
	String intent; InitString(&intent, 256);

	if (!decompose_command_3_params(doc, dynamic_mem, &target, &target_length, &context, &percA, &percW, &depth, &intent)) return;
					  
	Node* node = FindNode(target, target_length, NodeAt(context));

	if (!node){
		CatFixed(dynamic_mem, "Error: Target Node not found in context. (Only context was found).\n");
		FreeString(&intent);
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

