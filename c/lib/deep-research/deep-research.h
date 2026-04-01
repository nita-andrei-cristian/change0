#ifndef DEEP_RESEARCH_CHANGE_APP_UTILS
#define DEEP_RESEARCH_CHANGE_APP_UTILS

#include <stddef.h>
#include <stdint.h>

#define CONTEXT_INIT_SIZE 2
#define RESPONSE_NULL_ERROR "Error : Failed reading Response\n"
#define RESPONSE_FAIL_PARSE "Error : Failed parsing response as json\n"
#define JSON_INVALID_CHECKS "Error : Json was parsed, but invalid checks\n"

#define NODE_NOT_FOUND_MESSSAGE "Node not found : "

// general context about the round
typedef struct{
	char* payload;
	size_t len;
	size_t capacity;

	size_t round;
	size_t minDepth;
	size_t maxDepth;
} Context;

enum DEEP_RESEARCH_EXIT {
	COMMON_FAIL,
	NEXT_STEP,
	FINISH,
	MAX_LIMIT,
};

char* DeepResearchStart();

enum DEEP_RESEARCH_EXIT DeepResearchLoop(Context *context);

#endif
