#include "engine.h"
#include "node.h"
#include "math.h"
#include "../lib/util/util.h"

#include <string.h>
#include <stdio.h>

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
		AddNodeEx(name, len, activation, NODE_INIT_WGHT + weight / 10 , 1, context, 0, now);

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
	else
		BiLink(A, B);

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
	if (!ValidateContext(document, &context)){
		fprintf(stderr, "Error: Context not found or doesn't exist\n");
		return 0;
	} 

	Nodes.needsRefresh = 1;

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
					read_node_activation(n),
					read_node_weight(n),
					n->parent,
					n->globalIndex
				    );
		else
			p += sprintf(
					p,
					"{\"name\":\"%s\",\"activation\":%.2f,\"weight\":%.2f,\"id\":%zu}\n",
					n->label,
					read_node_activation(n),
					read_node_weight(n),
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
				read_connection_weight(&c),
				read_connection_activation(&c)
			);
		}
	}

	*p++ = ']';
	*p++ = '}';
	*p = '\0';

	return WriteGraphAndFreeData(directory, buf);
}

_Bool ValidateContext(json_value *document, size_t *context){
	// Iterate once to find context
	for (size_t i = 0; i < document->u.object.length; i++){
		json_object_entry entry = document->u.object.values[i];
		if (strcmp(entry.name, "context") == 0 && entry.value->type == json_string){
			// linear search (Small context sample)
			lowerAll(&entry.value->u.string.ptr, entry.value->u.string.length);
			for (uint_fast8_t i = 0; i < CONTEXT_COUNT; i++){
				if (strcmp(entry.value->u.string.ptr, NodeAt(Contexts[i])->label) == 0){
					*context = Contexts[i];
					return 1;
				}
			}
			return 0;
		}
	}
	return 0;
}

// AI GENERATED CODE
// TODO
// 100 means every 100s the nodes half in activation
// It's actually denoted as lambda in a weird formula so you might want to be more accurate
// Don't forget to implement this in JS
static double decay_from_to(double value, time_t from, time_t to)
{
    double dt = difftime(to, from);
    if (dt <= 0.0) return value;

    // half-life = 100 seconds
    return value * pow(2.0, -dt / ACT_HALFTIME);
}


static void refresh_connection(Connection *c, time_t now, double boost_per_touch)
{
    if (!c) return;

    c->_activation = decay_from_to(c->_activation, c->lastTouched, now);

    // Then add pending touches
    //n->activation += boost_per_touch * (double)n->pendingActivationTouches;
    c->_activation += boost_per_touch * log1p((double)c->pendingTouches);// TODO Try to switch here to see if the formula is better, this is smoother

    c->pendingTouches = 0;
    c->lastTouched = now;
}

static void set_activation(Node *n, time_t now, double boost_per_touch)
{
    if (!n) return;

    n->_activation = decay_from_to(n->_activation, n->lastTouched, now);

    // Then add pending touches
    //n->activation += boost_per_touch * (double)n->pendingActivationTouches;
    n->_activation += boost_per_touch * log1p((double)n->pendingTouches);// TODO Try to switch here to see if the formula is better, this is smoother

    n->pendingTouches = 0;
    n->lastTouched = now;
}

void RefreshGraph(){
	//if (!Nodes.needsRefresh) return;
	if (!Nodes.init || Nodes.count == 0) return;
	
	// decrease connection and activation on every instance
	time_t currTime = time(NULL);

	double k = 0.2; // TODO make this a constant
	double c = 0.2; // also this, the penalty of ncount
		
	double mx_seen = -1, mx_used = -1, mx_support = -1;

	double *support_buffer = malloc(sizeof(double) * Nodes.count);
	cassert(support_buffer, "Couldn't allocate memory for support\n");

	// compute maxes for normalziation
	for (size_t i = 0; i < Nodes.count; i++){
		Node* n = NodeAt(i);

		if (n->times_seen > mx_seen) mx_seen = n->times_seen;
		if (n->times_used > mx_used) mx_used = n->times_used;

		// calculate support
		double support = 0;

		for (size_t j = 0; j < n->ncount; j++){
			refresh_connection(&n->neighbours[j], currTime, CONN_ACT_INCR);
			support += read_connection_weight(&n->neighbours[j]) + k * read_connection_activation(&n->neighbours[j]);
		}

		support /= 1 + 0.2 * n->ncount;
		support_buffer[i] = support;

		if (support > mx_support) mx_support = support;
	}


	for (size_t i = 0; i < Nodes.count; i++){
		Node* n = NodeAt(i);

		double seen_norm = 0;
		double used_norm = 0;
		double support_norm = 0;

		if (mx_used > 0)
			used_norm = log1p(n->times_used) / log1p(mx_used);
		if (mx_seen > 0)
			seen_norm = log1p(n->times_seen) / log1p(mx_seen);
		if (mx_support > 0)
			support_norm = log1p(support_buffer[i]) / log1p(mx_support);

		// TODO turn scalars into constants
		double merit = 0.6 * support_norm + 0.4 * used_norm;
		double confidence = seen_norm;
		double base = NODE_INIT_WGHT;
		double old_weight = n->_weight;
		double target_weight = confidence * merit + (1.0 - seen_norm) * base;

		// set weight
		n->_weight = 0.95 * old_weight + 0.05 * target_weight;
		

		set_activation(n, currTime, NODE_ACT_INCR);
	}

	free(support_buffer);
	Nodes.needsRefresh = 0;
}
