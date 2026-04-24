#ifndef GOAL_CHANGE_HEADER
#define GOAL_CHANGE_HEADER

#include <stddef.h>
#include <time.h>
#include "util.h"

typedef struct GoalType {
	String title;
	String reasoning;

	time_t start_date;
	time_t end_date;
	size_t required_time;

	struct GoalType **subgoals;
	size_t subgoals_len;

	size_t priority;
} Goal;

enum GOAL_STATUS {
	GOAL_VALID=0,
	GOAL_COLLISION,
	GOAL_OVERDUE,
};

#define TIME_MARIGN 0.5


Goal* create_goal(String *input_goal, String *input_reasoning, size_t estimated_time);
void mock_develop_subgoals(Goal *g);
enum GOAL_STATUS validate_goal(Goal *g);
void repair_goal(Goal* g);
void update_goal(Goal *g);
void free_goal(Goal *g);
void free_goals();

#endif
