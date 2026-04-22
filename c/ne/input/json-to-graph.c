#include "json-to-graph.h"
#include "json.h"
#include "util.h"
#include "time.h"
#include "node.h"
#include "string.h"
#include "config.h"

static void AddNodeFromEntry(json_value *val, size_t context, time_t now){
	if (val->type != json_object) return;
	if (val->u.object.length == 0) return;

	char name[32] = "\0";
	_Bool constructable = 0;
	double activation = NODE_INIT_ACT;
	double weight = NODE_INIT_WGHT;
	size_t len;

	for (size_t i = 0; i < val->u.object.length; i++){
		json_object_entry entry = val->u.object.values[i];

		// Node {name, activation}

		if (!strcmp(entry.name, "name") && entry.value->type == json_string){
			// Constructing a name for the Node
			len = MIN(31, entry.value->u.string.length);
			memcpy(name, entry.value->u.string.ptr, len);
			name[len] = '\0';
			constructable = 1;
		} else if (!strcmp(entry.name, "activation")){
			if (entry.value->type == json_double){
				activation = entry.value->u.dbl;
			}else if (entry.value->type == json_integer){
				activation = (double)entry.value->u.integer;
			}
		}else if (!strcmp(entry.name, "weight")){
			if (entry.value->type == json_double){
				weight = entry.value->u.dbl;
			}else if (entry.value->type == json_integer){
				weight = (double)entry.value->u.integer;
			}
		}

	}

	if (!constructable) return;

	// TODO Quick reminder this only makes sense if NODE_INIT_ACT is 1.0 (same for the connections below)
	Node* existing = FindNode(name, len, NodeAt(context));
	if (existing)
		touch_node(existing, activation, now);
	else
		AddNodeEx(name, len, activation, NODE_INIT_WGHT + ( weight - NODE_INIT_WGHT ) * NODE_GUESS_WEIGHT_RELEVANCE , 1, context, 0, now);

}

static void AddNodesFromEntry(json_object_entry* entry, size_t context){
	if (!entry->value) return;
	if (entry->value->type != json_array || entry->value->u.array.length == 0) return;

	time_t now = time(NULL);
	for (size_t i = 0; i < entry->value->u.array.length; i++){
		AddNodeFromEntry(entry->value->u.array.values[i], context, now);
	}
	
}

static _Bool ProcessArrayLinkage(json_value *entry, double weight, double activation, size_t context, time_t now){
	if (!entry) return 0;

	if (entry->type != json_array || entry->u.array.length != 2) return 0;

	json_value* a = entry->u.array.values[0];
	json_value* b = entry->u.array.values[1];


	cassert(a->type == json_string && b->type == json_string, "Error: Nodes are not strings");

	Node* A = FindNode(a->u.string.ptr, a->u.string.length, NodeAt(context));
	if (!A) return 1;
	A->lastTouched = time(NULL);
	Node* B = FindNode(b->u.string.ptr, b->u.string.length, NodeAt(context));
	if (!B) return 1;
	B->lastTouched = time(NULL);

	Connection* existing = LinkExists(A, B);

	if(existing)
		touch_connection(existing, activation, now);
	else{
	 	BiLink(A, B);
		Connection* ab = A->neighbours + A->ncount - 1;
		Connection* ba = B->neighbours + B->ncount - 1;
		
		ab->_activation += activation * 0.1;
		ab->_weight += ( weight - CONN_INIT_WGHT ) * CONNECTION_GUESS_WEIGHT_RELEVANCE;
	}

	return 1;
}

// link a->b b->c c->d d->a
static void AddConnectionFromEntry(json_value* val, size_t context, time_t now){
	if (!val) return;
	//cassert(ProcessArrayLinkage(val, NODE_INIT_ACT, NODE_INIT_WGHT, context), "Problem when direct adding nodes"); // just in case we're working with a direct array
	if (val->type != json_object) return;
	
	double activation = NODE_INIT_ACT;
	double weight = NODE_INIT_WGHT;
	json_value* arr = NULL;

	for (size_t i = 0; i < val->u.object.length; i++){
		json_object_entry entry = val->u.object.values[i];
		if (!strcmp(entry.name, "nodes")){
			arr = entry.value;
		} else if (!strcmp(entry.name, "activation")){
			if (entry.value->type == json_double){
				activation = entry.value->u.dbl;
			}else if (entry.value->type == json_integer){
				activation = (double)entry.value->u.integer;
			}
		} else if (!strcmp(entry.name, "weight")){
			if (entry.value->type == json_double){
				weight = entry.value->u.dbl;
			}else if (entry.value->type == json_integer){
				weight = (double)entry.value->u.integer;
			}
		}
	}

	cassert(ProcessArrayLinkage(arr, weight, activation, context, now), "Problem when creaing connections list, btw size must be two.");
	
}

static void AddConnectionsFromEntry(json_object_entry* entry, size_t context){
	if (entry->value == NULL) return;
	if (entry->value->type != json_array) return;

	time_t now = time(NULL);
	for (size_t i = 0; i < entry->value->u.array.length; i++)
		// Adding a connection
		AddConnectionFromEntry(entry->value->u.array.values[i], context, now);

}

_Bool AddContextNodesFromJSON(json_object_entry* context_data){
	cassert(context_data, "No context data provided. Json entry.\n");

	cassert(context_data->value->type == json_object, "Context provided, but is not an object \n");

	json_value *root = context_data->value;

	Node* context = FindNodeGlobal(context_data->name, context_data->name_length, CONTEXT_COUNT);
	cassert(context, "Context not found. Be carefull, there is also an edge case where the context exists but is not withing the first CONTEXT_COUNT nodes.\n");

	size_t context_id = context->globalIndex;

	for (size_t i = 0; i < root->u.object.length; i++){
		json_object_entry entry = root->u.object.values[i];
		if (strcmp(entry.name, "nodes") == 0) AddNodesFromEntry(&entry, context_id);
		else if(strcmp(entry.name, "connections") == 0) AddConnectionsFromEntry(&entry, context_id);
	}

	printf("JSON parsed successfully\n\n");

	return 1;
}


