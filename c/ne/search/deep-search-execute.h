#ifndef DEEP_SEARCH_EXECUTE_C
#define DEEP_SEARCH_EXECUTE_C

#include "deep-search-session.h"
#include "json.h"
#include "util.h"

#define DS_JUDGE_PROMPT \
"You are an automatic validation agent for a deep-search investigation system. " \
"Your task is to evaluate whether the following raw deep-search conclusion is good enough for the following user task. " \
"User task: [%s]. " \
"Raw deep-search conclusion: [%s]. " \
"You are not judging literary style. You are judging whether the conclusion is relevant, grounded, specific enough, and useful enough for a downstream AI agent. " \
"Pass the result if it clearly addresses the task, contains meaningful insight, and gives usable direction for downstream action. " \
"Fail the result if it is too vague, too generic, poorly aligned with the task, repetitive, empty, or not useful enough. " \
"Fail the result if it sounds conclusive but does not actually say anything specific. " \
"Fail the result if it does not explain the main relevant patterns, constraints, or lack of evidence in a useful way. " \
"Important: do not require the conclusion to mention specific node categories, contexts, or domains unless they are clearly necessary and already supported by the investigation. " \
"Important: if the graph evidence appears sparse or insufficient, a conclusion may still pass if it clearly explains what was investigated, what was found, what was missing, and why the evidence is insufficient for stronger downstream action. " \
"Do not fail a conclusion merely because it did not discover a desired branch. " \
"Do not demand new social, relational, professional, or emotional evidence unless the current conclusion is clearly uninformative without it. " \
"Do not be overly harsh. Minor imperfections are acceptable if the result is still meaningfully useful. " \
"Do not be overly permissive. If the next AI agent would receive weak, shallow, or unclear guidance, fail it. " \
"If you pass the result, return a JSON object with pass=true and reason=null. " \
"If you fail the result, return a JSON object with pass=false and a short non-empty reason string. " \
"The reason must be a concrete retry hint for the deep-search agent. " \
"The reason must improve explanation quality, grounding, or investigative focus, but must not force unsupported branches or invent missing evidence. " \
"The reason must be short, direct, operational, and focused on what is missing or what should be improved in the next round. " \
"Return exactly one valid JSON object and nothing else."

#define OPENAI_DEEP_SEARCH_JUDGE_SCHEMA_JSON \
"{" \
  "\"type\":\"object\"," \
  "\"additionalProperties\":false," \
  "\"required\":[\"pass\",\"reason\"]," \
  "\"properties\":{" \
    "\"pass\":{\"type\":\"boolean\"}," \
    "\"reason\":{\"type\":[\"string\",\"null\"]}" \
  "}" \
"}"

void exec_response(json_value* doc, String *dynamic_mem, size_t depth, String* conclusion);

char *call_gpt_deepsearch(DS_memory *mem, size_t *respsize);

json_value *call_gpt_judge(String *out, Task *task);

#endif
