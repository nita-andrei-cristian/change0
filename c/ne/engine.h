#ifndef NEUROENGINE_INNER
#define NEUROENGINE_INNER

#include <stddef.h>
#include <../lib/jsonp/json.h>
#include <../lib/jsonp/json.h>

#define DECOMPOSITION_INTO_CONTEXT_PROMPT "You process [%s] from a [%s] perspective and give me a JSON."
_Bool AddContextNodesFromJSON(char* JSON, size_t len);
_Bool ExportGraphTo(char* path);

_Bool ValidateContext(json_value *document, size_t *context);

void RefreshGraph();

void DecomposeInputIntoGraph(char* input, size_t input_size);

#endif
