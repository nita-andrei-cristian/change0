#ifndef NEUROENGINE_EXPORT
#define NEUROENGINE_EXPORT

#include <stddef.h>
#include "json.h"

_Bool AddContextNodesFromJSON(json_object_entry* context_data);
_Bool ExportGraphTo(char* path);


#endif
