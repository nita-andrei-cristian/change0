#ifndef AI_OPENAI_H
#define AI_OPENAI_H

#include <stddef.h>
#include "json.h"
#include "util.h"

#ifndef OPENAI_PATH_CAP
#define OPENAI_PATH_CAP 512
#endif

typedef struct {
    char id[256];
    String raw_http;
    String body;
} ai_openai_response;

typedef enum {
    AI_OPENAI_OK = 0,
    AI_OPENAI_ERR_ARG = -1,
    AI_OPENAI_ERR_ENV = -2,
    AI_OPENAI_ERR_DNS = -3,
    AI_OPENAI_ERR_SOCKET = -4,
    AI_OPENAI_ERR_CONNECT = -5,
    AI_OPENAI_ERR_TLS = -6,
    AI_OPENAI_ERR_WRITE = -7,
    AI_OPENAI_ERR_READ = -8,
    AI_OPENAI_ERR_PARSE = -9,
    AI_OPENAI_ERR_ALLOC = -10,
    AI_OPENAI_ERR_HTTP = -11
} ai_openai_status;

ai_openai_status ai_openai_create_response(
    const char *json_body,
    ai_openai_response *out
);

ai_openai_status ai_openai_get_response(
    const char *response_id,
    ai_openai_response *out
);

void ai_openai_response_free(ai_openai_response *resp);

const char *ai_openai_strerror(ai_openai_status status);

char *openai_extract_output_text_dup(json_value *root, size_t *out_len);
json_value *openai_extract_text_json(json_value *root);

typedef enum {
    AI_OPENAI_MODEL_GPT_5_5,
    AI_OPENAI_MODEL_GPT_5_4_MINI,
    AI_OPENAI_MODEL_GPT_5_4_NANO,
    AI_OPENAI_MODEL_GPT_5_1,
    AI_OPENAI_MODEL_GPT_5_MINI
} ai_openai_model;

typedef struct {
    String prompt;
    String schema;

    ai_openai_model model;

    const char *schema_name;

    double temperature;
    int use_temperature;
} ai_gpt_request;

const char *ai_openai_model_name(ai_openai_model model);

String *ai_openai_call_gpt_request(ai_gpt_request *req);

#endif
