#include "engine.h"
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "node.h"
#include <../lib/jsonp/json.h>
#include <../lib/util/util.h>

static void AddNodeFromEntry(json_value *val, size_t context){
	if (val->type == json_string){
		printf("Adding [%s]\n", val->u.string.ptr);

		AddInfertileNodeInParent(val->u.string.ptr, val->u.string.length, context); 

		return;
	}
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

	if (constructable)
		AddNodeEx(name, len, activation, weight, 1, context, 0);
}

static void AddNodesFromEntry(json_object_entry* entry, size_t context){
	if (!entry->value) return;
	if (entry->value->type != json_array || entry->value->u.array.length == 0) return;

	for (size_t i = 0; i < entry->value->u.array.length; i++){
		AddNodeFromEntry(entry->value->u.array.values[i], context);
	}
	
}

static void ProcessArrayLinkage(json_value *entry, double weight, double activation, size_t context){
	if (!entry) return;
	if (entry->type != json_array || entry->u.array.length < 2) return;
	for (size_t i = 0; i < entry->u.array.length; i++){
		size_t j = (i + 1) % entry->u.array.length;

		json_value* a = entry->u.array.values[i];
		json_value* b = entry->u.array.values[j];

		if (a->type != json_string) continue;
		if (b->type != json_string) continue;

		Node* A = FindNode(a->u.string.ptr, a->u.string.length, NodeAt(context));
		if (!A) continue;
		Node* B = FindNode(b->u.string.ptr, b->u.string.length, NodeAt(context));
		if (!B) continue;

		printf("Linking %s, %s\n", A->label, B->label);

		UniLink(A, B);
		A->neighbours[A->ncount - 1].activation = activation;
		A->neighbours[A->ncount - 1].weight = weight;

	}
}

// link a->b b->c c->d d->a
static void AddConnectionFromEntry(json_value* val, size_t context){
	if (!val) return;
	ProcessArrayLinkage(val, NODE_INIT_ACT, NODE_INIT_WGHT, context); // just in case we're working with a direct array
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

	ProcessArrayLinkage(arr, weight, activation, context);
	
}

static void AddConnectionsFromEntry(json_object_entry* entry, size_t context){
	if (entry->value == NULL) return;
	if (entry->value->type != json_array) return;

	for (size_t i = 0; i < entry->value->u.array.length; i++)
		// Adding a connection
		AddConnectionFromEntry(entry->value->u.array.values[i], context);

}

_Bool AddContextNodesFromJSON(char *JSON, size_t len){
	if (!JSON) return 0;

	json_value* document = json_parse(JSON, len);
	size_t i;

	if (!document) return 0;
	if (document->type != json_object){
		json_value_free(document);
		return 0;
	};


	size_t context;
	_Bool found = 0;

	// Iterate once to find context
	for (i = 0; i < document->u.object.length; i++){
		json_object_entry entry = document->u.object.values[i];
		if (strcmp(entry.name, "context") == 0 && entry.value->type == json_string){
			// linear search (Small context sample)
			for (uint_fast8_t i = 0; i < CONTEXT_COUNT; i++){
				if (NodeExists(i) && strcmp(entry.value->u.string.ptr, NodeAt(Contexts[i])->label) == 0){
					context = Contexts[i];
					found = 1;
					break;
				}
			}
			break;
		}
	}

	if (!found){
		fprintf(stderr, "Error: Context not found or doesn't exist\n");
		return 0;
	} 

	for (i = 0; i < document->u.object.length; i++){
		json_object_entry entry = document->u.object.values[i];
		if (strcmp(entry.name, "nodes") == 0) AddNodesFromEntry(&entry, context);
		else if(strcmp(entry.name, "connections") == 0) AddConnectionsFromEntry(&entry, context);
	}

	printf("JSON parsed successfully\n\n");

	json_value_free(document);

	return 1;
}

static _Bool WriteGraphAndFreeData(char* directory, char *buf){
	FILE *fptr = NULL;

	// Open a file in writing mode
	fptr = fopen(directory, "w");

	if (!fptr) return 0;

	// Write some text to the file
	fprintf(fptr, "%s", buf);

	// Close the file
	fclose(fptr); 
	
	printf("\n\nSuccessfully exported graph to %s\n", directory);

	if (buf) free(buf);

	return 1;
}

_Bool ExportGraphTo(char* directory){
	size_t estimated =
		Nodes.count * 64 +
		ConnectionCount * 96 +
		512;

	char *buf = malloc(estimated);
	if (!buf){
		fprintf(stderr, "Couldn't allocate enough memory to export graph [%zu]\n", estimated);
		return 0;
	}

	char *p = buf;

	*p++ = '{';

	// Node section
	memcpy(p, "\"nodes\":[", FSIZE("\"nodes\":["));
	p += FSIZE("\"nodes\":[");

	for (size_t i = 0; i < Nodes.count; i++) {
		Node *n = NodeAt(i);
		//printf("Node : %s | %s\n", n->label, n->parent ? n->parent->label : "NONE");
	}

	for (size_t i = 0; i < Nodes.count; i++) {
		if (i) *p++ = ',';

		Node *n = NodeAt(i);

		if (n->hasParent)
			p += sprintf(
					p,
					"{\"name\":\"%s\",\"activation\":%.2f,\"weight\":%.2f,\"parent\":%zu,\"id\":%zu}\n",
					n->label,
					n->activation,
					n->weight,
					n->parent,
					n->globalIndex
				    );
		else
			p += sprintf(
					p,
					"{\"name\":\"%s\",\"activation\":%.2f,\"weight\":%.2f,\"id\":%zu}\n",
					n->label,
					n->activation,
					n->weight,
					n->globalIndex
				    );
	}

	*p++ = ']';
	*p++ = ',';

	// Connection section
	memcpy(p, "\"connections\":[", FSIZE("\"connections\":["));
	p += FSIZE("\"connections\":[");

	_Bool firstConnection = 1;

	for (size_t i = 0; i < Nodes.count; i++) {
		Node *n = NodeAt(i);

		for (size_t j = 0; j < n->ncount; j++) {
			Connection c = n->neighbours[j];

			if (!firstConnection)
				*p++ = ',';
			firstConnection = 0;

			p += sprintf(
				p,
				"{\"nodes\":[%zu,%zu],\"weight\":%.2f,\"activation\":%.2f}\n",
				n->globalIndex,
				c.target,
				c.weight,
				c.activation
			);
		}
	}

	*p++ = ']';
	*p++ = '}';
	*p = '\0';

	return WriteGraphAndFreeData(directory, buf);
}
