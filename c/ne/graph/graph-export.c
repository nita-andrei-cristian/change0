#include "graph-export.h"
#include "node.h"
#include "util.h"
#include <string.h>
#include <stdio.h>

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

