#include "engine.h"
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <node.h>
#include <../jsonp/json.h>
#include <../util.h>

// TODO : make sure strcmp is safe and strings null terminate, altough it's probably fine if i use the parser

static void AddNodeFromEntry(json_value *val){
	if (val->type == json_string){
		printf("Adding [%s]\n", val->u.string.ptr);

		Node* n = FindNode(val->u.string.ptr, val->u.string.length);
		if (n){
			n->activation += NODE_ACT_INCR;
		}else{
			AddNode(val->u.string.ptr); // TODO you may optimize by not calculating strlen twice (it's already calcualted in the json_string)
		}

		return;
	}
	if (val->type != json_object) return;
	if (val->u.object.length == 0) return;

	char name[32] = "\0";
	_Bool constructable = 0;
	double activation = 1.0;

	for (size_t i = 0; i < val->u.object.length; i++){
		json_object_entry entry = val->u.object.values[i];

		// Node {name, activation}

		if (!strcmp(entry.name, "name") && entry.value->type == json_string){
			// Constructing a name for the Node
			short len = MIN(31, entry.value->u.string.length);
			memcpy(name, entry.value->u.string.ptr, len);

			name[len] = '\0';
			constructable = 1;
		} else if (!strcmp(entry.name, "activation")){
			if (entry.value->type == json_double){
				activation = entry.value->u.dbl;
			}else if (entry.value->type == json_integer){
				activation = (double)entry.value->u.integer;
			}
		}
	}

	printf("Adding [%s] with activation [%f]\n", name, activation);
	if (constructable)
		AddNode_ex(name, activation);
	
}

static void AddNodesFromEntry(json_object_entry* entry){
	if (entry->value == NULL) return;
	if (entry->value->type != json_array) return;
	if (entry->value->u.array.length == 0) return;

	for (size_t i = 0; i < entry->value->u.array.length; i++){
		AddNodeFromEntry(entry->value->u.array.values[i]);
	}
	
	// Decay Nodes
	DecayNodes();
}

static void ProcessArrayLinkage(json_value *entry, double weight, double activation){
	if (!entry) return;
	if (entry->type != json_array) return;
	if (entry->u.array.length < 2) return;
	for (size_t i = 0; i < entry->u.array.length; i++){
		size_t j = (i + 1) % entry->u.array.length;

		json_value* a = entry->u.array.values[i];
		json_value* b = entry->u.array.values[j];

		if (a->type != json_string) continue;
		if (b->type != json_string) continue;

		Node* A = FindNode(a->u.string.ptr, a->u.string.length);
		if (!A) continue;
		Node* B = FindNode(b->u.string.ptr, b->u.string.length);
		if (!B) continue;

		printf("Linking %s, %s\n", A->label, B->label);

		struct Connection *link = LinkExists(A, B);
		if (link){
			link->activation += CONN_ACT_INCR * activation;
			link->weight += CONN_WGT_INCR * weight;
		}else{
			UniLink(A, B);
			A->neighbours[A->ncount - 1].activation = activation;
			A->neighbours[A->ncount - 1].weight = weight;
		}

	}
}

// link a->b b->c c->d d->a
static void AddConnectionFromEntry(json_value* val){
	if (!val) return;
	ProcessArrayLinkage(val, 1.0, 1.0); // just in case we're working with a direct array
	if (val->type != json_object) return;
	
	double activation = 1.0;
	double weight = 1.0;
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

	ProcessArrayLinkage(arr, weight, activation);
	
}

static void AddConnectionsFromEntry(json_object_entry* entry){
	if (entry->value == NULL) return;
	if (entry->value->type != json_array) return;
	if (entry->value->u.array.length == 0) return;

	for (size_t i = 0; i < entry->value->u.array.length; i++)
		// Adding a connection
		AddConnectionFromEntry(entry->value->u.array.values[i]);

	// Decay Connecitons
	DecayAllConnections();
}

_Bool AddToGraph(char *JSON, size_t len){
	if (!JSON) return 0;

	json_value* document = json_parse(JSON, len);
	size_t i;

	if (!document) return 0;
	if (document->type != json_object){
		json_value_free(document);
		return 0;
	};

	for ( i = 0; i < document->u.object.length; i ++){
		json_object_entry entry = document->u.object.values[i];
		if (strcmp(entry.name, "nodes") == 0) AddNodesFromEntry(&entry);
		else if(strcmp(entry.name, "connections") == 0) AddConnectionsFromEntry(&entry);
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
		if (i) *p++ = ',';

		Node *n = NodeAt(i);

		if (n->activation == 1.0) {
			*p++ = '"';
			memcpy(p, n->label, n->length);
			p += n->length;
			*p++ = '"';
		} else {
			p += sprintf(
				p,
				"{\"name\":\"%s\",\"activation\":%.2f}",
				n->label,
				n->activation
			);
		}
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
			struct Connection c = n->neighbours[j];

			if (!firstConnection)
				*p++ = ',';
			firstConnection = 0;

			p += sprintf(
				p,
				"{\"nodes\":[\"%s\",\"%s\"],\"weight\":%.2f,\"activation\":%.2f}",
				n->label,
				c.target->label,
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
