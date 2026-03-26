#include <stdio.h>
#include <stdlib.h>
#include "connection.h"
#include "../util.h"

_Bool FreeConnections(){
	if (Connections.items != NULL){
		free(Connections.items);
		Connections.items = NULL;
	}
	Connections.capacity = 0;
	Connections.count = 0;
	Connections.init = 0;
	return 1;
}

_Bool InitConnections(){
	if (Connections.init) FreeConnections();

	Connections.capacity = INIT_CONNECTION_CAP;
	Connections.count = 0;
	Connections.items = (Connection*)malloc(sizeof(Connection) * INIT_CONNECTION_CAP);

	if (Connections.items == NULL) {
		Connections.capacity = 0;
		Connections.init = 0;
		return 0;
	}

	Connections.init = 1;
	return 1;
}

Connection* AddConnections(){
	if (!Connections.init || !Connections.items) return NULL;

	if (Connections.count >= Connections.capacity) {
		size_t new_capacity = MAX(INIT_CONNECTION_CAP, Connections.capacity) * 2;
		Connection* tmp = realloc( Connections.items, sizeof(Connection) * new_capacity);
		if (tmp == NULL){
			fprintf(stderr, "Error: Couldn't allocate more memory to add nodes");
			return NULL;
		};
		Connections.items = tmp;
		Connections.capacity = new_capacity;
	}

	Connection* connection = Connections.items + Connections.count;

	return connection;
}

