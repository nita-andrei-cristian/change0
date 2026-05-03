#ifndef GOAL_UTIL_DECLARATIONS
#define GOAL_UTIL_DECLARATIONS

#include <stddef.h>
#include <time.h>
#include "config.h"
#include "node.h"
#include "util.h"

#define GOAL_REQUIRED_TIME_ERROR_MARGIN 0.5
#define INITIAL_GOAL_INDEX 1 // must be > 0
#define GOAL_ID_SIZE 32
#define GOAL_MIN_SECONDS 60 * 16

typedef _Bool (*goal_emit_like_func)(const char* id, const char *type, const char *buffer, size_t buffer_len);

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

	char id[GOAL_ID_SIZE + 1];
} Goal;

enum GOAL_STATUS {
	GOAL_VALID=0,
	GOAL_INVALID
};

extern Goal *GOAL_CONTAINER[1024];
extern size_t GOAL_CONTAINER_COUNT;

static inline Goal *FindGoal(size_t id)
{
    if (id == 0 || id >= GOAL_CONTAINER_COUNT)
        return NULL;

    return GOAL_CONTAINER[id];
}

static inline void link_goals(Goal* a, Goal* b){
	a->next = b->globalIndex;
	b->prev = a->globalIndex;
}

static void create_goal_task(String* input1, String* input2, Task *task){
	ResizeString(&task->name, sizeof(GOAL_ADAPTATION_PROMPT) + input1->len + input2->len + 10);
	size_t new_len = sprintf(c_str(&task->name), GOAL_ADAPTATION_PROMPT, c_str(input1), c_str(input2));
	cassert(new_len < task->name.cap, "This should be impossible...\n");

	task->name.len = new_len;
		
	task->minDepth = 1;
} 

void GoalSystemLazyLoad(goal_emit_like_func *goal_emit);
void CreateSubgoalId(Goal *parent, size_t child_index, char out[33]);
Goal *CreateGoal(char goalId[], String *input_goal, String *input_extrainfo, size_t estimated_time, size_t parent_index, size_t depth);
Goal *ExternalFindGoal(size_t id);

time_t CalcGoalRequiredTime(Goal *g);
enum GOAL_STATUS ValidateGoal(Goal *g, time_t now);
void CreateGoalDSId(char* name, char* deep_search_id);
void PersonalizeGoal(String* input1, String *input2, String* out, char* goalId);

Goal *CalcGoalRoot(Goal *g);

#endif
