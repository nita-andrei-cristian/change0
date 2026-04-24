#include "goal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"
#include "deep-search-session.h"

static Goal *GOAL_CONTAINER[1024];
static size_t GOAL_CONTAINER_COUNT = 0;

Goal *create_goal(String *input_goal, String *input_reasoning, size_t estimated_time, Goal *parent)
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

// those are mapped to input1 -> title input2 -> reason
void pre_process_goal(String *input1, String *input2)
{
	String out;
	InitString(&out, 2048);
	
	Task task;
	task.name_len = sprintf(task.name, "Customize the goal for the [%s]. With reasoning [%s], Come with an estimated_time in settings be pragmatic and gen JSON.", input1->p, input2->p);
	task.minDepth = 10;

	cassert(task.name_len < TASK_NAME_MAX_SIZE, "Task name is too big");
	
	// customize goal
	//start_ds_session()

	// extract process goals
	String title, reason;
	time_t estimated_time = 0;
	InitString(&title, 256); InitString(&reason, 1024);

	// obtain title and reason

	cassert(0, "TODO : Extract title, reason and estimated time");
	
	create_goal(&title, &reason, estimated_time, NULL);
}
