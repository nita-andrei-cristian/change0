#ifndef NEUROENGINE_INNER
#define NEUROENGINE_INNER

#include <stddef.h>
#include <../lib/jsonp/json.h>
#include <../lib/jsonp/json.h>

_Bool AddContextNodesFromJSON(char* JSON, size_t len);
_Bool ExportGraphTo(char* path);

_Bool ValidateContext(json_value *document, size_t *context);

void RefreshNodes();

#endif
