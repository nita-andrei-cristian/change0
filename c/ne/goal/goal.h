#ifndef GOAL_CHANGE_HEADER
#define GOAL_CHANGE_HEADER

#include <stddef.h>
#include <time.h>
#include "util.h"

typedef struct GoalType {
	String title;
	String extra_info;

	time_t start_date;
	time_t end_date;
	time_t required_time;

	size_t *subgoals;
	size_t subgoals_len;

	size_t parent;
	size_t prev;
	size_t next;
	
	size_t globalIndex;
	size_t depth;
	size_t retry_depth;

	size_t priority;
} Goal;

enum GOAL_STATUS {
	GOAL_VALID=0,
	GOAL_COLLISION,
	GOAL_OVERDUE,
};

#define TIME_MARIGN 0.5


Goal* GetGoalFromID(size_t ID);
void free_goal(Goal *g);
void free_goals();
Goal* CreateUserGoal(String *input1, String *input2);

/*
 * Placeholder Mapping
 * -------------------
 * %s  (1) : goal_title
 * %s  (2) : goal_extrainfo
 * %s  (3) : user_action_history_with_frequency
 * %zu (4) : initial_timeframe_seconds
 * %zu (5) : remaining_time_seconds
 * %s  (6) : parallel_goals_same_layer
 * %s  (7) : parent_goal_chain_with_extrainfo
 */
#define SHORTEN_GOAL_AI_PROMPT \
"You are a goal-refinement agent. Adapt your output to the user's behavior, constraints, available time, and goal hierarchy. "\
"The user is attempting a goal titled [%s], with extrainfo [%s]. "\
"This prompt was triggered because the goal appears unrealistic, inefficient, or overdue. "\
"User action history and frequency: [%s]. "\
"Initial timeframe: [%zu] seconds. Remaining total time: [%zu] seconds. "\
"Parallel goals at the same hierarchy level: [%s]. Do not create conflicts with them. "\
"Parent goal chain with extrainfo: [%s]. "\
"Create a shorter, more achievable version of the goal while preserving the original intent. "\
"Account for realistic human cognitive, physical, and time limitations. "\
"The new goal must fit within the remaining total time. "\
"Return JSON only, with this exact structure: "\
"{\"title\":\"string\",\"extrainfo\":\"string\"} "

#define OPENAI_GOAL_EXTRACT_SCHEMA_JSON \
"{" \
  "\"type\":\"object\"," \
  "\"additionalProperties\":false," \
  "\"required\":[\"title\",\"reason\"]," \
  "\"properties\":{" \
    "\"title\":{\"type\":\"string\"}," \
    "\"extrainfo\":{\"type\":\"string\"}," \
    "\"estimated_time\":{\"type\":\"integer\",\"minimum\":0}" \
  "}" \
"}"

#endif
