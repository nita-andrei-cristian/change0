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

	char id[32];
} Goal;

enum GOAL_STATUS {
	GOAL_VALID=0,
	GOAL_INVALID
};

#define TIME_MARIGN 0.5


void free_goal(Goal *g);
void free_goals();
Goal* CreateUserGoal(String *input1, String *input2, char goalId[32]);

/*
 * Placeholder Mapping
 * -------------------
 * %s  (1) : goal_title
 * %s  (2) : goal_extrainfo
 * %s  (3) : user_action_history
 * %zu (4) : initial_timeframe_seconds
 * %zu (5) : remaining_time_seconds
 * %s  (6) : parallel_goals_same_layer
 * %s  (7) : parent_goal_chain_with_extrainfo
 * %s  (8) : current_layer_goal_chain_with_extrainfo
 */
#define SHORTEN_GOAL_AI_PROMPT \
"You are a goal scope-calibration agent. Your job is to rewrite one active goal so it becomes more achievable without changing its direction, hierarchy role, or time context. "\
"The user is currently attempting this goal: title [%s], extrainfo [%s]. "\
"This rewrite was triggered because the current goal appears too ambitious, inefficient, overdue, or unrealistic for the available time. "\
"Important: do not treat this as creating a new goal. Treat it as resizing the current goal so it fits better into the existing goal tree. "\
"User action history: [%s]. These are previous goals with timing and efficiency information. Use them to infer realistic scope, but do not copy them unless directly relevant. "\
"Initial timeframe for the current goal: [%zu] seconds. Remaining total time: [%zu] seconds. "\
"The new goal should keep the same time context. Do not solve the problem by merely making a tiny task or by changing the goal into a different activity. "\
"Instead, lower the ambition inside the same remaining time by reducing one or more of: amount of work, depth of detail, quality threshold, number of substeps, or completion criteria. "\
"Parallel goals at the same hierarchy level: [%s]. The new goal must not overlap with, duplicate, conflict with, or take responsibility from these goals. "\
"Parent goal chain with extrainfo: [%s]. The new goal must remain clearly aligned with this parent chain and continue serving the same higher-level objective. "\
"Current-layer goal chain with extrainfo: [%s]. These are nearby goals at the same depth, such as previous, current, next, or sibling goals. "\
"Use the current-layer chain to make the rewritten goal fit like a missing puzzle piece: coherent with the surrounding goals, non-repetitive, and not drifting into another direction. "\
"Preserve the original goal's domain, intent, and role in the sequence. Only reduce the scope or success condition. "\
"If the original goal overlaps with nearby goals, narrow it to the smallest useful non-overlapping contribution toward the same intent. "\
"If the remaining time is very low, create a minimal but still meaningful salvage version of the same goal. "\
"If the remaining time is reasonable, do not create a trivial goal; create a slightly less ambitious version that can productively use the remaining time. "\
"The title must be concise, action-oriented, and clearly derived from the original goal. "\
"The extrainfo must explain: the reduced scope, what counts as success, what is intentionally excluded, and how it avoids overlap with nearby or parallel goals. "\
"The estimated_time field is required only for schema compatibility. It must be a positive integer. Use the remaining total time if it is positive; otherwise use 1. "\
"Return JSON only, with this exact structure and no extra text: "\
"{\"title\":\"string\",\"extrainfo\":\"string\",\"estimated_time\":1}"

#define OPENAI_GOAL_EXTRACT_SCHEMA_JSON \
"{" \
  "\"type\":\"object\"," \
  "\"additionalProperties\":false," \
  "\"required\":[\"title\",\"reason\",\"extrainfo\",\"estimated_time\"]," \
  "\"properties\":{" \
    "\"title\":{" \
      "\"type\":\"string\"" \
    "}," \
    "\"reason\":{" \
      "\"type\":\"string\"" \
    "}," \
    "\"extrainfo\":{" \
      "\"type\":[\"string\",\"null\"]" \
    "}," \
    "\"estimated_time\":{" \
      "\"type\":[\"integer\",\"null\"]," \
      "\"minimum\":0" \
    "}" \
  "}" \
"}"

typedef _Bool (*goal_emit_like_func)(const char* id, const char *type, const char *buffer, size_t buffer_len);

_Bool DecomposeGoal(Goal *g);

#endif
