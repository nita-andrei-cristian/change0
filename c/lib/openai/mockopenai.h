#ifndef OPENAI_MOCKS_H
#define OPENAI_MOCKS_H

#include "../../config.h"

#define OPENAI_IDENTITY_PROMPT_BODY \
"{" \
"\"model\":\"gpt-5.4-mini\"," \
"\"reasoning\":{\"effort\":\"low\"}," \
"\"max_output_tokens\":12000," \
"\"input\":\"Generate a single valid JSON object and nothing else. Do not output markdown. Do not output explanations. Do not wrap the answer in code fences. The output must be a large combined mock dataset for a user-identity graph engine. " \
"Return exactly one root JSON object with exactly these two top-level keys: \\\"graph_mocks\\\" and \\\"action_mocks\\\". " \
"Purpose: simulate identity graphs and deep-search commands over those graphs. The contexts used by graph mocks are: profesie, emotie, pasiuni, generalitati, subiectiv. " \
"GRAPH MOCK RULES: " \
"1. Each graph mock must be an object with keys: context, nodes, connections. " \
"2. context must be one of: profesie, emotie, pasiuni, generalitati, subiectiv. " \
"3. nodes is an array of objects. Each node must contain a string key \\\"name\\\". A node may also contain numeric keys \\\"weight\\\" and \\\"activation\\\". " \
"4. connections is an array of objects. Each connection must contain key \\\"nodes\\\" which is an array of exactly 2 node names. A connection may also contain numeric keys \\\"weight\\\" and \\\"activation\\\". " \
"5. Graph mocks must be semantically coherent. Nodes should be meaningfully related to the context. Connections should interleave nodes into a believable network. " \
"6. Most node weights should stay close to the configured default weight " CONFIG_XSTR(NODE_INIT_WGHT) ". Avoid large weight spikes. " \
"7. Activations should stay around default activation " CONFIG_XSTR(NODE_INIT_ACT) ", but occasional spikes are allowed. " \
"8. The number of nodes per graph should vary around " CONFIG_XSTR(DEFAULT_MOCK_NODES_COUNT) ". " \
"9. The number of connections per graph should vary around " CONFIG_XSTR(DEFAULT_MOCK_ACTIONS_COUNT) ". " \
"10. Omit weight or activation on some nodes and connections occasionally. " \
"11. Use short lowercase labels. Avoid duplicates in the same graph. " \
"12. Generate exactly " CONFIG_XSTR(GRAPH_MOCKS_COUNT) " graph mocks total. Distribute contexts reasonably. " \
"ACTION MOCK RULES: " \
"1. action_mocks is an array of raw command objects in native format. Do not wrap them. " \
"2. Allowed command shapes: " \
"Command 1: command, percentage, criteria, intent. " \
"Command 2: command, percentage, node, criteria, context, intent. " \
"Command 3: command, node, context, percA, percW, depth, intent. " \
"Terminal: finished, conclusion. " \
"3. Generate exactly " CONFIG_XSTR(ACTION_MOCKS_COUNT) " action mocks. " \
"4. Most actions must reference nodes from graph_mocks. " \
"5. About " CONFIG_XSTR(INVALID_ACTION_PERCENT) " percent must contain intentional mistakes. " \
"6. Mistakes may include: unknown node, invalid criteria, missing fields, invalid percentages, invalid depth, context mismatch. " \
"7. Do not label or explain mistakes. " \
"8. Keep intent short and natural. " \
"9. Criteria should usually be activation or weight, except mistakes. " \
"10. Depth should usually be small positive integer, except mistakes. " \
"11. Majority must be command 1, 2, or 3. Terminal objects are rare. " \
"QUALITY RULES: " \
"1. Output must be strictly valid JSON. " \
"2. Return only the JSON object. " \
"3. No duplicate top-level keys. " \
"4. Avoid duplicate node names inside a graph. " \
"5. Avoid meaningless connections. " \
"6. Graphs must be useful for traversal testing. " \
"7. Keep weights stable, activations can spike. " \
"8. Ensure most actions target valid nodes, with controlled invalid cases.\"," \
"\"text\":{" \
"\"format\":{" \
"\"type\":\"json_schema\"," \
"\"strict\":true," \
"\"name\":\"identity_mock_bundle\"," \
"\"schema\":{" \
"\"type\":\"object\"," \
"\"additionalProperties\":false," \
"\"required\":[\"graph_mocks\",\"action_mocks\"]," \
"\"properties\":{" \
"\"graph_mocks\":{" \
"\"type\":\"array\"," \
"\"items\":{" \
"\"type\":\"object\"," \
"\"additionalProperties\":false," \
"\"required\":[\"context\",\"nodes\",\"connections\"]," \
"\"properties\":{" \
"\"context\":{\"type\":\"string\",\"enum\":[\"profesie\",\"emotie\",\"pasiuni\",\"generalitati\",\"subiectiv\"]}," \
"\"nodes\":{" \
"\"type\":\"array\"," \
"\"items\":{" \
"\"type\":\"object\"," \
"\"additionalProperties\":false," \
"\"required\":[\"name\"]," \
"\"properties\":{" \
"\"name\":{\"type\":\"string\"}," \
"\"weight\":{\"type\":\"number\"}," \
"\"activation\":{\"type\":\"number\"}" \
"}" \
"}" \
"}," \
"\"connections\":{" \
"\"type\":\"array\"," \
"\"items\":{" \
"\"type\":\"object\"," \
"\"additionalProperties\":false," \
"\"required\":[\"nodes\"]," \
"\"properties\":{" \
"\"nodes\":{" \
"\"type\":\"array\",\"minItems\":2,\"maxItems\":2," \
"\"items\":{\"type\":\"string\"}" \
"}," \
"\"weight\":{\"type\":\"number\"}," \
"\"activation\":{\"type\":\"number\"}" \
"}" \
"}" \
"}" \
"}" \
"}" \
"}," \
"\"action_mocks\":{" \
"\"type\":\"array\"," \
"\"items\":{" \
"\"type\":\"object\"," \
"\"additionalProperties\":false," \
"\"properties\":{" \
"\"command\":{\"type\":\"integer\"}," \
"\"percentage\":{\"type\":\"integer\"}," \
"\"criteria\":{\"type\":\"string\"}," \
"\"intent\":{\"type\":\"string\"}," \
"\"node\":{\"type\":\"string\"}," \
"\"context\":{\"type\":\"string\"}," \
"\"percA\":{\"type\":\"integer\"}," \
"\"percW\":{\"type\":\"integer\"}," \
"\"depth\":{\"type\":\"integer\"}," \
"\"finished\":{\"type\":\"boolean\"}," \
"\"conclusion\":{\"type\":\"string\"}" \
"}," \
"\"anyOf\":[" \
"{\"required\":[\"command\"]}," \
"{\"required\":[\"finished\",\"conclusion\"]}" \
"]" \
"}" \
"}" \
"}" \
"}" \
"}" \
"}";

_Bool RegenMocksOpenAI();

#endif
