#ifndef NEUROENGINE_INNER
#define NEUROENGINE_INNER

#include <stddef.h>
#include <../lib/jsonp/json.h>
#include <../lib/jsonp/json.h>

#define DECOMPOSITION_INTO_GRAPH_PROMPT \
"You are analyzing a single user input and decomposing it into a semantic identity graph across five psychological contexts. " \
"The input text is: [%s]. " \
"The purpose is to transform the user input into a structured graph representation of the user's internal identity, motivations, emotions, passions, general tendencies, and subjective interpretations. " \ "This graph will later be traversed by an AI investigation engine. " \
"You must analyze the same input through exactly these five contexts: profesie, emotie, pasiuni, generalitati, subiectiv. " \
"Return one single valid JSON object and nothing else. " \
"Do not output markdown. Do not output explanations. Do not output commentary. " \
"The root JSON object must contain exactly one top-level key named contexts. " \
"contexts must be an array of exactly five objects, one for each of these contexts: profesie, emotie, pasiuni, generalitati, subiectiv. " \
"Each context object must contain exactly these keys: context, nodes, connections. " \
"For each context object: " \
"1. context must be exactly the context name. " \
"2. nodes must be an array of semantic concepts relevant to that context. " \
"3. connections must be an array of meaningful relations between nodes from the same context. " \
"Each node must contain a string field named name. " \
"Each node may also contain numeric fields named weight and activation. " \
"weight should usually stay close to " CONFIG_XSTR(NODE_INIT_WGHT) " with small variation. " \
"activation should usually stay close to " CONFIG_XSTR(NODE_INIT_ACT) " but may occasionally spike when a concept is especially salient in the input. " \
"Each connection must contain a field named nodes which is an array of exactly two node names that exist in the same context object. " \
"Each connection may also contain numeric fields named weight and activation. " \
"Generate around " CONFIG_XSTR(DEFAULT_MOCK_NODES_COUNT) " relevant nodes per context when enough information exists. " \
"Generate around " CONFIG_XSTR(DEFAULT_MOCK_ACTIONS_COUNT) " relevant connections per context when enough information exists. " \
"If the input provides little information for a context, return fewer but still relevant nodes and connections. " \
"Rules: " \
"1. Use short lowercase node names. " \
"2. Avoid duplicates within the same context. " \
"3. Do not invent unrelated concepts. " \
"4. Stay faithful to the user input. " \
"5. Prefer semantically meaningful abstractions over copying long phrases from the input. " \
"6. Keep each context distinct, but mild overlap of concepts is allowed when justified by the input. " \
"7. Connections must be meaningful, not random. " \
"8. Return only JSON."

_Bool AddContextNodesFromJSON(json_object_entry* context_data);
_Bool ExportGraphTo(char* path);

_Bool ValidateContext(json_value *document, size_t *context);

void RefreshGraph();

void DecomposeInputIntoGraph(char* input, size_t input_size);

#endif
