#include "goal.h"
#include <assert.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "globals.h"
#include "node.h"
#include "json.h"
#include "config.h"
#include "openai.h"
#include "deep-search-session.h"

static goal_emit_like_func goal_emit = NULL;

#define INITIAL_GOAL_INDEX 1 // must be > 0
			     
static Goal *GOAL_CONTAINER[1024];
static size_t GOAL_CONTAINER_COUNT = INITIAL_GOAL_INDEX;

// AI generated function
static void create_subgoal_id(Goal *parent, size_t child_index, char out[33])
{
	memset(out, 0, 33);

	int n = snprintf(out, 33, "g%zu-%zu", parent->globalIndex, child_index + 1);

	change_assert(n > 0 && n < 33, "Failed to create subgoal id.\n");
}

static inline Goal *FindGoal(size_t id)
{
    if (id == 0 || id >= GOAL_CONTAINER_COUNT)
        return NULL;

    return GOAL_CONTAINER[id];
}

Goal *ExternalFindGoal(size_t id){
	return FindGoal(id);
};

static Goal *create_goal(char goalId[], String *input_goal, String *input_extrainfo, size_t estimated_time, size_t parent_index, size_t depth)
{
	Goal *g = malloc(sizeof(Goal));

	InitString(&g->title, input_goal->len + 1);
	CopyString(&g->title, input_goal);

	InitString(&g->extra_info, input_extrainfo->len + 1);
	CopyString(&g->extra_info, input_extrainfo);

	g->required_time = estimated_time;

	memset(g->id, 0, sizeof(g->id));
	memcpy(g->id, goalId, 32);
	g->id[32] = '\0';

	g->subgoals = NULL;
	g->subgoals_len = 0;
	g->priority = 0;
	g->prev = 0;
	g->next = 0;
	g->start_date = 0;
	g->end_date = 0;
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

static time_t calc_required_time(Goal *g){
	change_assert(g, "Goal with required time of 0 must not exist [%s]", g->title.p);

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

static enum GOAL_STATUS validate_goal(Goal *g, time_t now)
{
	// parent goals are valid because they depend on children validation
	if (g->subgoals_len > 0)
		return GOAL_VALID;

	if (g->start_date && !g->end_date && (now - g->start_date) > calc_required_time(g) * (1 + TIME_MARIGN) ){
		// goal is imposible to finish
		return GOAL_INVALID;
	}

	// goal hasn't started
	return GOAL_VALID;
}

static void call_title_extra(String *input, String *out)
{
    String prompt;
    InitString(&prompt, input->len + 2048);

    CatTemplateString(&prompt, GOAL_JSON_EXTRACT_PROMPT, c_str(input));

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

	json_value* doc = json_parse(c_str(&json_extract_result), json_extract_result.len);
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

#define OTHER_GOAL_TEMPLATE \
"{\"relation\":\"%s\",\"goal_title\":\"%s\",\"goal_extrainfo\":\"%s\",\"depth\":%zu,\"estimated_time\":%zu,\"started_on_date\" : \"%s\",\"finished_on_date\":\"%s\"}\n"

#define OTHER_GOAL_TEMPLATE_RICH \
"{\"relation\":\"%s\",\"goal_title\":\"%s\",\"goal_extrainfo\":\"%s\",\"depth\":%zu,\"estimated_time\":%zu,\"started_on_date\" : \"%s\",\"finished_on_date\":\"%s\",\"time_efficiency_metric\" : \"%s\"}\n"
 
inline static char* register_goal(Goal* g, size_t *length, char* relation, _Bool showExtraInfo){
	*length = sizeof(OTHER_GOAL_TEMPLATE_RICH) + g->extra_info.len + g->title.len + 512;

	char* main_buffer = malloc(*length);
	cassert(main_buffer, "Coudln't malloc mem for main buffer\n");

	char end_time_buffer[128];
	size_t temp_len = sprintf(end_time_buffer, "%s", g->end_date == 0 ? "goal is not finished" : ctime(&g->end_date));
	cassert(temp_len < 128, "Buffer too small 0\n");

	char start_time_buffer[128];
	temp_len = sprintf(start_time_buffer, "%s", g->start_date == 0 ? "goal is not started" : ctime(&g->start_date));
	cassert(temp_len < 128, "Buffer too small 0\n");

	end_time_buffer[strcspn(end_time_buffer, "\n")] = '\0';
	start_time_buffer[strcspn(start_time_buffer, "\n")] = '\0';

	char completion_metrics_buffer[256] = "\0";
	const size_t req_time = calc_required_time(g);
	_Bool can_display_efficiency = g->start_date != 0 && g->end_date != 0 && g->start_date != g->end_date && req_time;

	if (can_display_efficiency){
		size_t difference = (g->end_date - g->start_date);
		double efficiency = (double)req_time / (double)(g->end_date - g->start_date) * 100.0;
		temp_len = sprintf(completion_metrics_buffer, "Actual duration: about %zu seconds, Time efficiency: %.2f%%", difference, efficiency);
		change_assert(temp_len < 256, "Buffer too small 0\n");
	}
	
	int actual_len = -1; // this should give overflow and max value
	if (can_display_efficiency)
		actual_len = snprintf(main_buffer, *length, OTHER_GOAL_TEMPLATE_RICH, relation, g->title.p, showExtraInfo ? g->extra_info.p : "[hidden]", g->depth, req_time, start_time_buffer, end_time_buffer, completion_metrics_buffer);
	else
		actual_len = snprintf(main_buffer, *length, OTHER_GOAL_TEMPLATE, relation, g->title.p, showExtraInfo ? g->extra_info.p : "[hidden]", g->depth, req_time, start_time_buffer, end_time_buffer);

	change_assert(actual_len >= 0, "Failed to calc an actual len [%d]", actual_len);
	change_assert(actual_len < *length, "You calculated the max len wrong. max [%zu] and actual [%d]\n", *length, actual_len);

	*length = actual_len;

	return main_buffer;
}

static void compute_user_action_up_to(Goal* g, String *buffer, int max){

	size_t currentIndex = g->globalIndex - 1;
	int i = 0;

	while (currentIndex >= INITIAL_GOAL_INDEX && i < max){
		Goal *g = FindGoal(currentIndex);

		if (g->start_date != 0){
			size_t register_len_size = 0;
			char* info = register_goal(g, &register_len_size, "example-goal", 1);
			cassert(info, "Something failed when provdiing info.\n");
			CatString(buffer, info, register_len_size);
			free(info);
			i++;
		}

		currentIndex --;
	}
}

static void compute_same_layer(Goal *g, String *buffer){
	if (g->parent == 0) {
		CatString(buffer, FSTRING_SIZE_PARAMS("Root goal has no same-layer siblings.\n"));
		return;
	}

	Goal* parent = FindGoal(g->parent);

	for (size_t i = 0; i < parent->subgoals_len; i++){
		Goal *slibing = FindGoal(parent->subgoals[i]);

		size_t len = 0;
		char* info = register_goal(slibing, &len, "brother-goal", 1);
		CatString(buffer, info, len);

		free(info);
	}
}

static void compute_goal_parent_chain(Goal *g, String *buffer){
	
	while (g->parent != 0){
		g = FindGoal(g->parent);

		size_t len;
		char* info = register_goal(g, &len, "parent-goal", 1);

		CatString(buffer, info, len);

		free(info);
	}
}

static void compute_goal_brothers_chain(Goal *g, String *buffer, _Bool displayInfo){
	
	CatString(buffer, FSTRING_SIZE_PARAMS("Follow-up goals: \n"));
	while (g->next != 0){
		g = FindGoal(g->next);

		size_t len;
		char* info = register_goal(g, &len, "follow-up-goal", displayInfo);

		CatString(buffer, info, len);

		free(info);
	}

	CatString(buffer, FSTRING_SIZE_PARAMS("\n\nPrevious goals: \n"));
	while (g->prev != 0){
		g = FindGoal(g->prev);

		size_t len;
		char* info = register_goal(g, &len, "prev-goal", displayInfo);

		CatString(buffer, info, len);

		free(info);
	}
}

static void compute_uncle_chain(Goal *g, String *buffer, _Bool displayInfo){
	if (g->parent == 0){
		CatString(buffer, FSTRING_SIZE_PARAMS("Root goal has no uncle because it's a root goal."));
		return;
	}
	Goal* parent = FindGoal(g->parent);
	if (parent == 0){
		CatString(buffer, FSTRING_SIZE_PARAMS("This is a root goal, it doesn't have any uncles."));
		return;
	}

	CatString(buffer, FSTRING_SIZE_PARAMS("Uncles divided by follow up and previous relative to parent goal:\n"));

	compute_goal_brothers_chain(parent, buffer, displayInfo);
}


static void set_shorten_goal_prompt(Goal* g, String* prompt, time_t now){
	time_t old_required_time = g->required_time;
	cassert(g->start_date, "You can't shorten a goal without start date.");

	time_t estimated_end_date = g->start_date + old_required_time;

	int64_t remaining_time = estimated_end_date - now;
	time_t initial_timeframe = g->required_time;

	char* title = c_str(&g->title);
	char* extra_info = c_str(&g->extra_info);

	// user action will be last max 20 goals
	String user_action; InitString(&user_action, 4096);
	compute_user_action_up_to(g, &user_action, 20);

	String same_layer; InitString(&same_layer, 2048);
	compute_same_layer(g, &same_layer);

	String goal_parent_chain; InitString(&goal_parent_chain, 2048);
	compute_goal_parent_chain(g, &goal_parent_chain);

	String goal_brothers_chain; InitString(&goal_brothers_chain, 2048);
	compute_goal_brothers_chain(g, &goal_brothers_chain, 0);

	char* user_action_raw = user_action.p;
	char* same_layer_raw = same_layer.p;
	char* goal_parent_chain_raw = goal_parent_chain.p;
	char* goal_brothers_chain_raw = goal_brothers_chain.p;

	size_t estimated_size = user_action.len + same_layer.len + goal_parent_chain.len + g->title.len + g->extra_info.len + sizeof(SHORTEN_GOAL_AI_PROMPT) + 1024;

	CatTemplateString(prompt, SHORTEN_GOAL_AI_PROMPT, 
			title, extra_info,
			user_action_raw, 
			initial_timeframe,
			remaining_time,
			same_layer_raw,
			goal_parent_chain_raw,
			goal_brothers_chain_raw
		);

	FreeString(&user_action);
	FreeString(&same_layer);
	FreeString(&goal_parent_chain);
	FreeString(&goal_brothers_chain);
}

static void set_decompose_goal_prompt(Goal* g, String* prompt, time_t now){
	time_t old_required_time = g->required_time;

	time_t estimated_end_date = g->start_date + old_required_time;

	time_t required_time = calc_required_time(g);
	size_t depth = g->depth;
	char* title = c_str(&g->title);
	char* extra_info = c_str(&g->extra_info);

	// user action will be last max 20 goals
	String user_action; InitString(&user_action, 4096);
	compute_user_action_up_to(g, &user_action, 20);

	String same_layer; InitString(&same_layer, 2048);
	compute_same_layer(g, &same_layer);

	String goal_parent_chain; InitString(&goal_parent_chain, 2048);
	compute_goal_parent_chain(g, &goal_parent_chain);

	String goal_brothers_chain; InitString(&goal_brothers_chain, 2048);
	compute_goal_brothers_chain(g, &goal_brothers_chain, 0);

	String goal_uncle_chain; InitString(&goal_uncle_chain, 2048);
	compute_uncle_chain(g, &goal_uncle_chain, 1);

	size_t estimated_size = user_action.len + same_layer.len + goal_parent_chain.len + g->title.len + g->extra_info.len + sizeof(SHORTEN_GOAL_AI_PROMPT) + 1024;

	String personalization_context; InitString(&personalization_context, 2048);
	Task task = {0};
	task.minDepth = 2;
	InitString(&task.name, 2048);
	CatTemplateString(&task.name, GOAL_DECOMPOSITION_PERSONAL_CONTEXT_PROMPT, g->title.p, g->extra_info.p, goal_parent_chain.p);

	start_ds_session(&task, g->id, &personalization_context);

	char* user_action_raw = user_action.p;
	char* same_layer_raw = same_layer.p;
	char* goal_parent_chain_raw = goal_parent_chain.p;
	char* goal_brothers_chain_raw = goal_brothers_chain.p;
	char* goal_uncle_chain_raw = goal_uncle_chain.p;
	char* personalization_context_raw = personalization_context.p;


	CatTemplateString(prompt, DECOMPOSE_GOAL_AI_PROMPT, 
			title, 
			extra_info,
			required_time,
			depth,
			user_action_raw,
			personalization_context_raw,
			goal_parent_chain_raw,
			goal_brothers_chain_raw,
			goal_uncle_chain_raw
		);

	FreeString(&user_action);
	FreeString(&same_layer);
	FreeString(&goal_parent_chain);
	FreeString(&goal_brothers_chain);
	FreeString(&goal_uncle_chain);
	FreeString(&task.name);
	FreeString(&personalization_context);
}


void shorten_goal(Goal *g, time_t now){
	String prompt; InitString(&prompt, 256);

	set_shorten_goal_prompt(g, &prompt, now);

	ai_gpt_request req = {0};
	req.prompt = prompt;
	InitString(&req.schema, sizeof(OPENAI_GOAL_EXTRACT_SCHEMA_JSON) + 1);
	CatString(&req.schema, FSTRING_SIZE_PARAMS(OPENAI_GOAL_EXTRACT_SCHEMA_JSON));

	req.model = AI_OPENAI_MODEL_GPT_5_4_MINI;
	req.schema_name = "goal_extract";

	String *out = ai_openai_call_gpt_request(&req);

	String new_title, new_extra_info;
	time_t new_estimated_time = 0; // this should not matter
	extract_goal(out, &new_title, &new_extra_info, &new_estimated_time);
	
	change_assert(new_title.len > 1 && new_extra_info.len > 1, "Goal title or extra info is broken. title : [%s], info : [%s]\n", new_title.p, new_extra_info.p);

	CatTemplateString(&g->extra_info, "\n user failed to finish goal [%s] so was shortened to [%s] in date [%s]\n", g->title.p, new_title.p, ctime(&now));

	CopyString(&g->title, &new_title);
	CopyString(&g->extra_info, &new_extra_info);

	FreeString(&prompt);
	FreeString(out);
}

static void repair_goal_leaf(Goal *g)
{
	change_assert(g->subgoals_len == 0, "Only leaf goals can be repaired directly [%s]\n", g->title.p);
	
	time_t max_end_date = g->start_date + g->required_time * (1 + TIME_MARIGN);
	time_t now = time(NULL);

	time_t delta = (max_end_date - now) * (max_end_date - g->start_date);
	
	_Bool shouldExtend = g->retry_depth < 2;
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

void UpdateGoal(Goal *g, time_t now)
{
	if (!g)
		return;

	for (size_t i = 0; i < g->subgoals_len; i++)
		UpdateGoal(FindGoal(g->subgoals[i]), now);

	enum GOAL_STATUS status = validate_goal(g, now);

	if (status == GOAL_VALID)
		return;

	repair_goal_leaf(g);
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
	for (size_t i = INITIAL_GOAL_INDEX; i < GOAL_CONTAINER_COUNT; i++)
	{
		Goal* g = FindGoal(i);

		if (!g || g->parent != 0) continue;

		free_goal(GOAL_CONTAINER[i]);
		GOAL_CONTAINER[i] = NULL;
	}

	GOAL_CONTAINER_COUNT = INITIAL_GOAL_INDEX;
}

static void create_goal_deep_search_id(char* name, char* deep_search_id){
	size_t index_end = MIN(strlen(name), 200);
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
Goal* CreateUserGoal(String *input1, String *input2, char goalId[])
{
	lazy_load();

	String title, extra_info, deep_search_result;
	time_t estimated_time = 0;

	personalize_goal(input1, input2, &deep_search_result, goalId);
	extract_goal(&deep_search_result, &title, &extra_info, &estimated_time);

	// emit info to client
	goal_emit(goalId, "title", title.p, title.len);
	goal_emit(goalId, "extra-info", extra_info.p, extra_info.len);

	char total_time_buff[32] = {0};
	size_t len = snprintf(total_time_buff, sizeof(total_time_buff), "%ld", (long)estimated_time);
	change_assert(len < sizeof(total_time_buff), "time buffer too small\n");

	goal_emit(goalId, "time", total_time_buff, len);

	printf("before create_goal\n");
	Goal *created = create_goal(goalId, &title, &extra_info, estimated_time, 0, 1);
	printf("after create_goal [%p]\n", (void*)created);

	FreeString(&title);
	FreeString(&extra_info);
	FreeString(&deep_search_result);

	return created;
}

// AI generated function
static json_value *json_object_get(json_value *obj, const char *name)
{
	change_assert(obj && obj->type == json_object, "Expected JSON object.\n");

	for (size_t i = 0; i < obj->u.object.length; i++) {
		json_object_entry entry = obj->u.object.values[i];

		if (strcmp(entry.name, name) == 0)
			return entry.value;
	}

	return NULL;
}

// AI generated function
static void parse_decomposition_subgoal(
	json_value *item,
	String *title,
	String *extrainfo,
	size_t *estimated_time
) {
	change_assert(item && item->type == json_object, "Subgoal item is not an object.\n");

	json_value *title_json = json_object_get(item, "title");
	json_value *extrainfo_json = json_object_get(item, "extrainfo");
	json_value *estimated_time_json = json_object_get(item, "estimated_time");

	change_assert(title_json && title_json->type == json_string, "Subgoal title missing or invalid.\n");
	change_assert(extrainfo_json && extrainfo_json->type == json_string, "Subgoal extrainfo missing or invalid.\n");
	change_assert(estimated_time_json && estimated_time_json->type == json_integer, "Subgoal estimated_time missing or invalid.\n");

	change_assert(title_json->u.string.length >= 3, "Subgoal title is too short.\n");
	change_assert(extrainfo_json->u.string.length >= 10, "Subgoal extrainfo is too short.\n");
	change_assert(estimated_time_json->u.integer > 0, "Subgoal estimated_time must be positive.\n");

	InitString(title, title_json->u.string.length + 64);
	CatString(title, title_json->u.string.ptr, title_json->u.string.length);

	InitString(extrainfo, extrainfo_json->u.string.length + 256);
	CatString(extrainfo, extrainfo_json->u.string.ptr, extrainfo_json->u.string.length);

	*estimated_time = (size_t)estimated_time_json->u.integer;
}

// AI generated function
static String *call_decompose_goal_ai(String *prompt)
{
	ai_gpt_request req = {0};

	req.prompt = *prompt;

	InitString(&req.schema, sizeof(OPENAI_GOAL_DECOMPOSITION_SCHEMA_JSON) + 1);
	CatString(&req.schema, FSTRING_SIZE_PARAMS(OPENAI_GOAL_DECOMPOSITION_SCHEMA_JSON));

	req.model = AI_OPENAI_MODEL_GPT_5_4_MINI;
	req.schema_name = "goal_decomposition";

	printf("Calling goal decomposition...\n\n");

	String *result = ai_openai_call_gpt_request(&req);
	cassert(result, "OpenAI goal decomposition call failed.\n");

	FreeString(&req.schema);

	return result;
}

// AI assisted function
_Bool DecomposeGoal(Goal *g){
	
	if (g->subgoals_len != 0){
		printf("Goal seems already decomposed.\n");
		return 0;
	}
	if (g->required_time < 60 * 15){
		printf("Goal si too short to decompose, shortest is 15 minutes");
		return 0;
	}

	String prompt;
	InitString(&prompt, 2048);

	set_decompose_goal_prompt(g, &prompt, time(NULL));

	String *out = call_decompose_goal_ai(&prompt);
	cassert(out, "Goal decomposition returned NULL.\n");

	json_value *doc = json_parse(c_str(out), out->len);
	change_assert(doc && doc->type == json_object, "Goal decomposition result is not a JSON object:\n%s\n", c_str(out));

	json_value *subgoals_json = json_object_get(doc, "subgoals");
	change_assert(subgoals_json && subgoals_json->type == json_array, "Goal decomposition JSON has no subgoals array.\n");

	size_t subgoal_count = subgoals_json->u.array.length;

	change_assert(subgoal_count >= 2, "Goal decomposition must create at least 2 subgoals.\n");
	change_assert(subgoal_count <= 9, "Goal decomposition created too many subgoals: [%zu].\n", subgoal_count);
	change_assert(GOAL_CONTAINER_COUNT + subgoal_count <= 1024, "Not enough room in GOAL_CONTAINER for decomposition.\n");

	size_t *subgoal_indexes = malloc(sizeof(size_t) * subgoal_count);
	cassert(subgoal_indexes, "Could not allocate subgoal index array.\n");

	Goal *previous = NULL;

	for (size_t i = 0; i < subgoal_count; i++) {
		json_value *item = subgoals_json->u.array.values[i];

		String title;
		String extrainfo;
		size_t estimated_time = 0;

		parse_decomposition_subgoal(item, &title, &extrainfo, &estimated_time);

		char child_goal_id[33];
		create_subgoal_id(g, i, child_goal_id);

		Goal *child = create_goal(
			child_goal_id,
			&title,
			&extrainfo,
			estimated_time,
			g->globalIndex,
			g->depth + 1
		);

		subgoal_indexes[i] = child->globalIndex;

		if (previous)
			link_goals(previous, child);

		previous = child;

		FreeString(&title);
		FreeString(&extrainfo);
	}

	g->subgoals = subgoal_indexes;
	g->subgoals_len = subgoal_count;
	g->required_time = calc_required_time(g);

	json_value_free(doc);
	FreeString(&prompt);
	FreeString(out);

	printf("Goal decomposed into [%zu] subgoals.\n", subgoal_count);

	return 1;
}

Goal **GetGoalsContainer(size_t *len){
	*len = GOAL_CONTAINER_COUNT - INITIAL_GOAL_INDEX;
	return &GOAL_CONTAINER[1];
}
