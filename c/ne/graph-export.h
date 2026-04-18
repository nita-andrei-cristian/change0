#ifndef NEUROENGINE_INNER
#define NEUROENGINE_INNER

#include <stddef.h>
#include "../lib/jsonp/json.h"
#include "../lib/util/util.h"

_Bool AddContextNodesFromJSON(json_object_entry* context_data);
_Bool ExportGraphTo(char* path);


#endif
