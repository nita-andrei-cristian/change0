#include "goal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"
#include "json.h"
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
	task->name_len = sprintf(task->name, "Adapth the goal [%s] for the user. With reasoning [%s], Come with an estimated_time in settings be pragmatic and reason why. Esimated time is total time, not work time. Sturcture clearly your arguments as 1. title and 2. reasoning and 3. estumated_time (in seconds)", input1->p, input2->p);
	task->minDepth = 10;

	cassert(task->name_len < TASK_NAME_MAX_SIZE, "Task name is too big");

} 

static void mock_call_title_reason(String *input, String* out){
	String prompt;
	InitString(&prompt, input->cap + 2048);

	size_t len = sprintf(c_str(&prompt), "You are an agent supposed to extract a json file, nothing more, nothign less, you will extract a title, reason and estimated_time (in seconds, integer) json from the following message. It should be deomposed, if not, extract apropiatelly without inventing amnything. Message : [%s]", c_str(input));
	cassert(len < input->cap, "Length is too big, or cap is too small.\n");
	prompt.len = len;

	// here goes work ...

	CatString(out, FSTRING_SIZE_PARAMS("{\"reason\" : \"become rich and cool\", \"title\" : \"get one million dollars\", \"estimated_time\" : 999999}"));
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

	Task task;
	create_goal_task(input1, input2, &task);
	
	// customize goal
	//start_ds_session(&task, search_id, &deep_search_result);

	//printf("Goal advice to user: [%s]\n\n", deep_search_result.p);

	// extract process goals
	String title, reason;
	time_t estimated_time = 0;
	InitString(&title, 256); InitString(&reason, 1024);

	mock_call_title_reason(&deep_search_result, &json_extract_result);

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
