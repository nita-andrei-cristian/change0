#include "goal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"
#include "json.h"
#include "config.h"
#include "openai.h"
#include "deep-search-session.h"

static Goal *GOAL_CONTAINER[1024];
static size_t GOAL_CONTAINER_COUNT = 0;

static Goal *create_goal(String *input_goal, String *input_reasoning, size_t estimated_time, Goal *parent)
{
	Goal *g = malloc(sizeof(Goal));

	InitString(&g->title, 256);
	CopyString(&g->title, input_goal);

	InitString(&g->reasoning, 1024);
	CopyString(&g->reasoning, input_reasoning);

	g->required_time = estimated_time;

	g->subgoals = NULL;
	g->subgoals_len = 0;
	g->priority = 0;
	g->globalIndex = GOAL_CONTAINER_COUNT;

	g->parent = parent;

	GOAL_CONTAINER[GOAL_CONTAINER_COUNT++] = g;

	return g;
}

void mock_develop_subgoals(Goal *g)
{
	time_t now = g->start_date;
	time_t end = g->end_date;

	Goal **subgoals = malloc(2 * sizeof(Goal *));

	String *input0 = CreateStringFrom(FSTRING_SIZE_PARAMS("Frontend development"));
	String *reasoning0 = CreateStringFrom(FSTRING_SIZE_PARAMS("Advance the skill for generating a website."));

	String *input1 = CreateStringFrom(FSTRING_SIZE_PARAMS("Backend development"));
	String *reasoning1 = CreateStringFrom(FSTRING_SIZE_PARAMS("Learn how to debug and stay sane."));

	size_t hours60 = 60 * 60 * 60;

	subgoals[0] = create_goal(input0, reasoning0, hours60, g);
	subgoals[1] = create_goal(input1, reasoning1, hours60, g);

	g->subgoals = subgoals;
	g->subgoals_len += 2;

	FreeString(input0);
	FreeString(reasoning0);
	FreeString(input1);
	FreeString(reasoning1);
}

enum GOAL_STATUS validate_goal(Goal *g)
{
	if (!g->subgoals || g->subgoals_len == 0)
		return GOAL_VALID;

	size_t required_sum = 0;
	for (size_t i = 1; i < g->subgoals_len; i++)
	{
		Goal *old = g->subgoals[i - 1];
		Goal *new = g->subgoals[i];

		cassert(new, "This should never be null. subgoal1");
		cassert(old, "This should never be null. subgoal2");

		required_sum += old->required_time;
	}

	required_sum += g->subgoals[g->subgoals_len - 1]->required_time;

	if (required_sum > g->required_time * (1 + TIME_MARIGN))
		return GOAL_OVERDUE;

	return GOAL_VALID;
}

void repair_goal(Goal *g)
{
	// TODO REPAIR GOAL
	cassert(0, "Implement Goal repairing");
}

void update_goal(Goal *g)
{
	for (size_t i = 0; i < g->subgoals_len; i++)
	{
		Goal *s = g->subgoals[i];

		enum GOAL_STATUS status = validate_goal(s);

		if (status != GOAL_VALID)
			repair_goal(g);

		update_goal(s);
	}
}

void free_goal(Goal *g)
{
	if (!g)
		return;
	if (!g->title.p)
		return;
	for (size_t i = 0; i < g->subgoals_len; i++)
		free_goal(g->subgoals[i]);

	FreeString(&g->title);
	FreeString(&g->reasoning);
	free(g->subgoals);
	free(g);
}

void free_goals()
{
	for (size_t i = 0; i < GOAL_CONTAINER_COUNT; i++)
	{
		free_goal(GOAL_CONTAINER[i]);
	}
}

static void create_goal_deep_search_id(char* name, char* deep_search_id){
	size_t index_end = MAX(strlen(name), 200);
	memcpy(deep_search_id, name, index_end);

	memcpy(deep_search_id + index_end, FSTRING_SIZE_PARAMS("-deep-search-id"));
	*(deep_search_id + index_end + FSIZE("-deep-search-id")) = '\0';
}

static void create_goal_task(String* input1, String* input2, Task *task){
	ResizeString(&task->name, sizeof(GOAL_ADAPTATION_PROMPT) + input1->len + input2->len + 10);
	size_t new_len = sprintf(c_str(&task->name), GOAL_ADAPTATION_PROMPT, c_str(input1), c_str(input2));
	cassert(new_len < task->name.cap, "This should be impossible...\n");

	task->name.len = new_len;
		
	task->minDepth = 6;

} 

static void call_title_reason(String *input, String *out)
{
    String prompt;
    InitString(&prompt, input->len + 2048);

    size_t len = sprintf(
        c_str(&prompt),
        GOAL_JSON_EXTRACT_PROMPT,
        c_str(input)
    );

    cassert(len < prompt.cap, "Goal extraction prompt is too big.\n");
    prompt.len = len;

    ai_gpt_request req = {0};
    req.prompt = prompt;
    InitString(&req.schema, sizeof(OPENAI_GOAL_EXTRACT_SCHEMA_JSON) + 1);
    CatString(&req.schema, FSTRING_SIZE_PARAMS(OPENAI_GOAL_EXTRACT_SCHEMA_JSON));

    req.model = AI_OPENAI_MODEL_GPT_5_4_NANO;
    req.schema_name = "goal_extraction";
    req.use_temperature = 0;

    String *result = ai_openai_call_gpt_request(&req);
    cassert(result, "OpenAI goal extraction call failed.\n");

    CatString(out, result->p, result->len);

    FreeString(&prompt);
    FreeString(&req.schema);
    FreeString(result);
}

// those are mapped to input1 -> title input2 -> reason
Goal* CreateUserGoal(String *input1, String *input2)
{
	// Init params
	String deep_search_result, json_extract_result;
	InitString(&deep_search_result, 2048);
	InitString(&json_extract_result, 2048);

	char search_id[256];
	create_goal_deep_search_id(c_str(input1), search_id);

	Task task = {0};
	create_goal_task(input1, input2, &task);
	
	// customize goal
	start_ds_session(&task, search_id, &deep_search_result);

	// extract process goals
	String title, reason;
	time_t estimated_time = 0;
	InitString(&title, 256); InitString(&reason, 1024);

	call_title_reason(&deep_search_result, &json_extract_result);

	json_value* doc = json_parse(c_str(&json_extract_result), json_extract_result.cap);
	cassert(doc, "AI produced an invalid JSON. (for goals)\n");
	cassert(doc->type == json_object, "JSON is not an object. (for goal)");

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry candidate = doc->u.object.values[i];

		if (strcmp(candidate.name, "reason") == 0){
			cassert(candidate.value->type == json_string, "JSON \"reason\" is not a string.\n");
			CatString(&reason, candidate.value->u.string.ptr, candidate.value->u.string.length);
		}else if (strcmp(candidate.name, "title") == 0){
			cassert(candidate.value->type == json_string, "JSON \"title\" is not a string.\n");
			CatString(&title, candidate.value->u.string.ptr, candidate.value->u.string.length);
		}else if (strcmp(candidate.name, "estimated_time") == 0){
			cassert(candidate.value->type == json_integer, "JSON \"estimated_time\" is not an integer.\n");
			estimated_time = candidate.value->u.integer;
		}
	}
	json_value_free(doc);

	printf("Goal estimated time [%ld], reason [%s], title [%s].\n", estimated_time, reason.p, title.p);
	
	return create_goal(&title, &reason, estimated_time, NULL);
}
