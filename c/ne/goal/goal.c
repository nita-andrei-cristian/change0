#include "goal.h"
#include <assert.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "node.h"
#include "json.h"
#include "config.h"
#include "openai.h"
#include "deep-search-session.h"

static goal_emit_like_func goal_emit = NULL;

#define INITIAL_GOAL_INDEX 1 // must be > 0
#define FindGoal(x) (GOAL_CONTAINER[(x) - INITIAL_GOAL_INDEX])

static Goal *GOAL_CONTAINER[1024];
static size_t GOAL_CONTAINER_COUNT = INITIAL_GOAL_INDEX;

Goal* GetGoalFromID(size_t ID){
	return FindGoal(ID);
}

static Goal *create_goal(String *input_goal, String *input_extrainfo, size_t estimated_time, size_t parent_index, size_t depth)
{
	Goal *g = malloc(sizeof(Goal));

	InitString(&g->title, 256);
	CopyString(&g->title, input_goal);

	InitString(&g->extra_info, 1024);
	CopyString(&g->extra_info, input_extrainfo);

	g->required_time = estimated_time;

	g->subgoals = NULL;
	g->subgoals_len = 0;
	g->priority = 0;
	g->prev = 0;
	g->next = 0;
	g->retry_depth = 0;
	g->globalIndex = GOAL_CONTAINER_COUNT;
	g->depth = depth;

	g->parent = parent_index;

	GOAL_CONTAINER[GOAL_CONTAINER_COUNT] = g;

	GOAL_CONTAINER_COUNT++;

	return g;
}

static inline void link_goals(Goal* a, Goal* b){
	a->next = b->globalIndex;
	b->prev = a->globalIndex;
}

void mock_develop_subgoals(Goal *g)
{
	time_t now = g->start_date;
	time_t end = g->end_date;

	Goal **subgoals = malloc(2 * sizeof(Goal *));
	size_t *goals_index  = malloc(2 * sizeof(size_t));

	String *input0 = CreateStringFrom(FSTRING_SIZE_PARAMS("Frontend development"));
	String *info0 = CreateStringFrom(FSTRING_SIZE_PARAMS("Advance the skill for generating a website."));

	String *input1 = CreateStringFrom(FSTRING_SIZE_PARAMS("Backend development"));
	String *info1 = CreateStringFrom(FSTRING_SIZE_PARAMS("Learn how to debug and stay sane."));

	size_t hours60 = 60 * 60 * 60;

	subgoals[0] = create_goal(input0, info0, hours60, g->globalIndex, g->depth + 1);
	subgoals[1] = create_goal(input1, info1, hours60, g->globalIndex, g->depth + 1);

	goals_index[0] = subgoals[0]->globalIndex;
	goals_index[1] = subgoals[1]->globalIndex;

	link_goals(subgoals[0], subgoals[1]);

	g->subgoals = goals_index;
	g->subgoals_len += 2;

	FreeString(input0);
	FreeString(info0);
	FreeString(input1);
	FreeString(info1);

	free(subgoals);
}

enum GOAL_STATUS validate_goal(Goal *g)
{
	if (!g->subgoals || g->subgoals_len == 0)
		return GOAL_VALID;

	size_t required_sum = 0;
	if(g->subgoals_len == 1)
		required_sum = FindGoal(g->subgoals[0])->required_time;
	
	for (size_t i = 1; i < g->subgoals_len; i++)
	{
		size_t old_index = g->subgoals[i - 1];
		Goal* old = FindGoal(old_index);

		size_t new_index = g->subgoals[i];
		Goal* new = FindGoal(new_index);

		cassert(new, "This should never be null. subgoal1");
		cassert(old, "This should never be null. subgoal2");

		required_sum += old->required_time;
		if (i == g->subgoals_len - 1)
			required_sum += new->required_time;
	}

	if (required_sum > g->required_time * (1 + TIME_MARIGN))
		return GOAL_OVERDUE;

	return GOAL_VALID;
}

static void call_title_extra(String *input, String *out)
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

    printf("Calling extracter... \n\n");

    String *result = ai_openai_call_gpt_request(&req);
    cassert(result, "OpenAI goal extraction call failed.\n");

    CatString(out, result->p, result->len);

    FreeString(&prompt);
    FreeString(&req.schema);
    FreeString(result);
}

static void extract_goal(String* text, String* title, String* extrainfo, time_t *estimated_time){
	String json_extract_result;

	// extract process goals
	InitString(title, 256); InitString(extrainfo, 1024);
	InitString(&json_extract_result, 2048);

	printf("Extracing goal... \n\n");

	call_title_extra(text, &json_extract_result);

	json_value* doc = json_parse(c_str(&json_extract_result), json_extract_result.cap);
	change_assert(doc && doc->type == json_object, "Goal is not an object or is not a json\n\n\n%s", c_str(text));

	for (size_t i = 0; i < doc->u.object.length; i++){
		json_object_entry candidate = doc->u.object.values[i];

		if (strcmp(candidate.name, "extrainfo") == 0){
			cassert(candidate.value->type == json_string, "JSON \"extrainfo\" is not a string.\n");
			CatString(extrainfo, candidate.value->u.string.ptr, candidate.value->u.string.length);
		}else if (strcmp(candidate.name, "title") == 0){
			cassert(candidate.value->type == json_string, "JSON \"title\" is not a string.\n");
			CatString(title, candidate.value->u.string.ptr, candidate.value->u.string.length);
		}else if (strcmp(candidate.name, "estimated_time") == 0){
			cassert(candidate.value->type == json_integer, "JSON \"estimated_time\" is not an integer.\n");
			*estimated_time = candidate.value->u.integer;
		}
	}
	json_value_free(doc);

}


static Goal *get_root(Goal *g){
	while (g->parent != 0){
		g = FindGoal(g->parent);
	}
	return g;
}

static time_t calc_required_time(Goal *g){

	/*
		If a goal has children, return the children sum
		else return the default required time 
	*/

	if (g->subgoals_len == 0){
		return g->required_time;
	}

	time_t sum = 0;

	for (size_t i = 0; i < g->subgoals_len; i++){
		Goal *subgoal = FindGoal(g->subgoals[i]);
		
		sum += calc_required_time(subgoal);		
	}

	// also cache the calculated required just in case children are regenerated
	g->required_time = sum;

	return sum;
}

#define REGISTERED_ACITON_TEMPLATE "{\"goal_title\" : %s, \"depth\" : %zu, \"finished on date\": \"%s\", \"efficiency_score\" : \"%s\"}\n"
 
inline static char* register_goal(Goal* g, size_t *min_len){
	*min_len = sizeof(REGISTERED_ACITON_TEMPLATE) + g->title.len + 512;

	char* main_buffer = malloc(*min_len);
	cassert(main_buffer, "Coudln't malloc mem for main buffer\n");

	time_t copytime = g->end_date;
	char time_buffer[128];
	size_t l0 = sprintf(time_buffer, "%s", copytime == 0 ? "goal is not finished" : ctime(&copytime));
	cassert(l0 < 128, "Buffer too small 0\n");

	char efficiency_buffer[128];
	if (g->end_date && g->start_date && g->required_time && g->end_date != g->start_date){
		size_t l1 = snprintf(efficiency_buffer, 128, "%.2f%%",
				(double)g->required_time /
				(double)(g->end_date - g->start_date) * 100.0
				);
		cassert(l1 < 128, "Buffer too small 1\n");
	}else{
		sprintf(efficiency_buffer, "goal not finished.");
	}

	sprintf(main_buffer, REGISTERED_ACITON_TEMPLATE, g->title.p, g->depth, time_buffer, efficiency_buffer);

	return main_buffer;
}

static void compute_user_action_up_to(Goal* g, String *buffer, int max){
	InitString(buffer, 4048);

	size_t currentIndex = g->globalIndex - 1;
	int i = 0;

	while (currentIndex >= INITIAL_GOAL_INDEX && i < max){
		Goal *g = FindGoal(currentIndex);

		if (g->start_date != 0){
			size_t register_len_size = 0;
			char* info = register_goal(g, &register_len_size);
			cassert(info, "Something failed when provdiing info.\n");
			CatString(buffer, info, register_len_size);
			free(info);
			i++;
		}

		currentIndex --;
	}
}

static void compute_same_layer(Goal *g, String *buffer){
	InitString(buffer, 4048);

	Goal* parent = FindGoal(g->parent);

	for (size_t i = 0; i < parent->subgoals_len; i++){
		Goal *friend = FindGoal(g->subgoals[i]);
		size_t len = 0;

		char* info = register_goal(friend, &len);

		CatString(buffer, info, len);

		free(info);
	}
}

static void compute_goal_parent_chain(Goal *g, String *buffer){
	InitString(buffer, 2048);
	
	while (g->parent != 0){

		size_t len;
		char* info = register_goal(g, &len);

		CatString(buffer, info, len);

		g = FindGoal(g->parent);
		free(info);
	}
}

static void set_shorten_goal_prompt(Goal* g, String* prompt, time_t now){
	time_t old_required_time = g->required_time;
	cassert(g->start_date, "You can't shorten a goal without start date.");

	time_t estimated_end_date = g->start_date + old_required_time;

	time_t remaining_time = estimated_end_date - now;
	time_t initial_timeframe = g->required_time;

	char* title = c_str(&g->title);
	char* extra_info = c_str(&g->extra_info);

	// user action will be last max 20 goals
	String user_action;
	compute_user_action_up_to(g, &user_action, 20);

	String same_layer;
	compute_same_layer(g, &same_layer);

	String goal_parent_chain;
	compute_goal_parent_chain(g, &goal_parent_chain);

	char* user_action_raw = user_action.p;
	char* same_layer_raw = same_layer.p;
	char* goal_parent_chain_raw = goal_parent_chain.p;

	size_t estimated_size = user_action.len + same_layer.len + goal_parent_chain.len + g->title.len + g->extra_info.len + sizeof(SHORTEN_GOAL_AI_PROMPT) + 1024;

	CatTemplateString(prompt, SHORTEN_GOAL_AI_PROMPT, 
			title, extra_info,
			user_action_raw, 
			initial_timeframe,
			remaining_time,
			same_layer_raw,
			goal_parent_chain_raw
		);

	FreeString(&user_action);
	FreeString(&same_layer);
	FreeString(&goal_parent_chain);
}

void shorten_goal(Goal *g, time_t now){
	String prompt; InitString(&prompt, 256);

	set_shorten_goal_prompt(g, &prompt, now);

	ai_gpt_request req = {0};
	req.prompt = prompt;
	InitString(&req.schema, sizeof(OPENAI_GOAL_EXTRACT_SCHEMA_JSON) + 1);
	CatString(&req.schema, FSTRING_SIZE_PARAMS(OPENAI_GOAL_EXTRACT_SCHEMA_JSON));

	req.model = AI_OPENAI_MODEL_GPT_5_4_MINI;
	req.schema_name = "shorten_goal";

	String *out = ai_openai_call_gpt_request(&req);

	String new_title, new_extra_info;
	time_t new_estimated_time = 0; // this should not matter
	extract_goal(out, &new_title, &new_extra_info, &new_estimated_time);
	
	change_assert(new_title.len > 1 && new_extra_info.len > 1, "Goal title or extra info is broken. title : [%s], info : [%s]\n", new_title.p, new_extra_info.p);

	EmptyString(&g->title);
	CatTemplateString(&g->extra_info, "%s\n user failed to finish goal [%s] so was shortened to [%s] in date [%s]\n", g->title, new_title, ctime(&now));

	CopyString(&g->title, &new_title);
	CopyString(&g->extra_info, &new_extra_info);

	FreeString(&prompt);
	FreeString(out);
}

void repair_goal(Goal *g)
{
	time_t max_end_date = g->start_date + g->required_time * (1 + TIME_MARIGN);
	time_t now = time(NULL);

	time_t delta = (max_end_date - now) * (max_end_date - g->start_date);
	
	_Bool shouldExtend = g->retry_depth < 3;
	g->retry_depth ++;

	if (shouldExtend){
		// increase goal by 25%
		time_t old_time = calc_required_time(g);
		time_t new_time = (old_time * 125) / 100;

		g->required_time = new_time;
		CatTemplateString(&g->extra_info, "\n[Goal was extended on date [%s] from [%zu] to [%zu]]\n", ctime(&now), old_time, new_time);
	}else{
		shorten_goal(g, now);
		g->retry_depth = 0;
	}
}

void UpdateGoal(Goal *g)
{
	for (size_t i = 0; i < g->subgoals_len; i++)
	{
		Goal *s = FindGoal(g->subgoals[i]);

		enum GOAL_STATUS status = validate_goal(s);

		if (status != GOAL_VALID)
			repair_goal(g);

		UpdateGoal(s);
	}
}

void free_goal(Goal *g)
{
	if (!g)
		return;
	if (!g->title.p)
		return;
	for (size_t i = 0; i < g->subgoals_len; i++)
		free_goal(FindGoal(g->subgoals[i]));

	if (g->title.p)
		FreeString(&g->title);
	if (g->extra_info.p){
		FreeString(&g->extra_info);
	}
	if (g->subgoals){
		free(g->subgoals);
		g->subgoals = NULL;
	}
	if (g){
		free(g);
	}
}

void free_goals()
{
	for (size_t i = 0; i < GOAL_CONTAINER_COUNT; i++)
	{
		free_goal(GOAL_CONTAINER[i]);
		GOAL_CONTAINER[i] = NULL;
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
		
	task->minDepth = 1;

} 

static void personalize_goal(String* input1, String *input2, String* out, char* goalId){
	InitString(out, 2048);

	// Init params
	char search_id[256];
	create_goal_deep_search_id(c_str(input1), search_id);

	Task task = {0};
	create_goal_task(input1, input2, &task);
	
	// customize goal
	start_ds_session(&task, search_id, out);

	goal_emit(goalId, "deep-search-final-recomandation", c_str(out), out->len);
}

static void lazy_load(){
    if (goal_emit == NULL){
        goal_emit = (goal_emit_like_func)ReadGlobalPointer(FSTRING_SIZE_PARAMS("goal_emit"));
	change_assert(goal_emit, "Can't load goal_emit");
    }
}

// those are mapped to input1 -> title input2 -> extrainfo
Goal* CreateUserGoal(String *input1, String *input2, char goalId[32])
{
	lazy_load();

	String title, extra_info, deep_search_result;
	time_t estimated_time = 0;

	personalize_goal(input1, input2, &deep_search_result, goalId);
	extract_goal(&deep_search_result, &title, &extra_info, &estimated_time);

	// emit info to client
	goal_emit(goalId, "title", title.p, title.len);
	goal_emit(goalId, "extra-info", extra_info.p, extra_info.len);

	char total_time_buff[16] = {0};
	size_t len = snprintf(total_time_buff, 16, "%zu", extra_info.len);
	change_assert(len < 16, "Compiler is retarded\n");

	goal_emit(goalId, "time", total_time_buff, len);

	printf("Goal created:\ntime [%ld]\nextra_info [%s]\ntitle [%s]\n\n", estimated_time, extra_info.p, title.p);
	
	return create_goal(&title, &extra_info, estimated_time, 0, 1);
}
