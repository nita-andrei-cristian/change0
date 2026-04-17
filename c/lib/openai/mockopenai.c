/*
 * 90% AI GENERATED CODE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "openai.h"
#include "mockopenai.h"
#include "../jsonp/json.h"
#include "../util/util.h"

#include <sys/stat.h>
#include <sys/types.h>


/* ---------------- JSON helpers ---------------- */

static json_value* json_obj_get(json_value *obj, const char *key){
    if (!obj || obj->type != json_object) return NULL;
    for (unsigned i = 0; i < obj->u.object.length; i++){
        if (strcmp(obj->u.object.values[i].name, key) == 0)
            return obj->u.object.values[i].value;
    }
    return NULL;
}

static const char* extract_output_text_bundle(json_value *root){
    json_value *output = json_obj_get(root, "output");
    if (!output || output->type != json_array) return NULL;

    for (unsigned i = 0; i < output->u.array.length; i++){
        json_value *item = output->u.array.values[i];

        json_value *type = json_obj_get(item, "type");
        if (!type || type->type != json_string) continue;

        if (strcmp(type->u.string.ptr, "message") != 0) continue;

        json_value *content = json_obj_get(item, "content");
        if (!content || content->type != json_array) continue;

        for (unsigned j = 0; j < content->u.array.length; j++){
            json_value *c = content->u.array.values[j];

            json_value *ctype = json_obj_get(c, "type");
            if (!ctype || ctype->type != json_string) continue;

            if (strcmp(ctype->u.string.ptr, "output_text") != 0) continue;

            json_value *text = json_obj_get(c, "text");
            if (!text || text->type != json_string) continue;

            return text->u.string.ptr;
        }
    }

    return NULL;
}

/* ---------------- JSON write ---------------- */

static void json_write_escaped(FILE *f, const char *s){
    fputc('"', f);
    for (; *s; s++){
        if (*s == '"') fputs("\\\"", f);
        else if (*s == '\\') fputs("\\\\", f);
        else fputc(*s, f);
    }
    fputc('"', f);
}

static void json_write_value(FILE *f, json_value *v){
    if (!v){ fputs("null", f); return; }

    switch (v->type){
        case json_null:    fputs("null", f); break;
        case json_boolean: fputs(v->u.boolean ? "true" : "false", f); break;
        case json_integer: fprintf(f, "%lld", (long long)v->u.integer); break;
        case json_double:  fprintf(f, "%.17g", v->u.dbl); break;

        case json_string:
            json_write_escaped(f, v->u.string.ptr);
            break;

        case json_array:
            fputc('[', f);
            for (unsigned i = 0; i < v->u.array.length; i++){
                if (i) fputc(',', f);
                json_write_value(f, v->u.array.values[i]);
            }
            fputc(']', f);
            break;

        case json_object:
            fputc('{', f);
            for (unsigned i = 0; i < v->u.object.length; i++){
                if (i) fputc(',', f);
                json_write_escaped(f, v->u.object.values[i].name);
                fputc(':', f);
                json_write_value(f, v->u.object.values[i].value);
            }
            fputc('}', f);
            break;

        default:
            fputs("null", f);
    }
}

static _Bool write_json_file(const char *path, json_value *v){
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    json_write_value(fp, v);
    fputc('\n', fp);
    fclose(fp);
    return 1;
}

static _Bool write_text_file(const char *path, const char *data){
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    fwrite(data, 1, strlen(data), fp);
    fclose(fp);
    return 1;
}

/* ---------------- utils ---------------- */

static void ensure_dir(const char *dir){
    if (MKDIR(dir) == 0) return;
    if (errno == EEXIST) return;
    cassert(0, "Failed to create directory\n");
}

/* ---------------- request builder ---------------- */

static char* BuildOpenAIRequestBody(void){
    char *escaped_prompt = json_escape_dup(OPENAI_PROMPT_TEXT);

    size_t cap = strlen(escaped_prompt) + strlen(OPENAI_SCHEMA_JSON) + 32768;
    char *body = (char*)calloc(cap, 1);
    cassert(body != NULL, "Failed to allocate OpenAI request body.\n");

    int n = snprintf(
        body, cap,
        "{"
          "\"model\":\"%s\","
          "\"reasoning\":{\"effort\":\"low\"},"
          "\"max_output_tokens\":12000,"
          "\"input\":\"%s\","
          "\"text\":{"
            "\"format\":{"
              "\"type\":\"json_schema\","
              "\"strict\":true,"
              "\"name\":\"identity_mock_bundle\","
              "\"schema\":%s"
            "}"
          "}"
        "}",
        "gpt-5.4",
        escaped_prompt,
        OPENAI_SCHEMA_JSON
    );

    free(escaped_prompt);

    cassert(n > 0 && (size_t)n < cap, "OpenAI request body overflow.\n");

    json_value *test = json_parse(body, strlen(body));
    cassert(test != NULL, "Built OpenAI request body is invalid JSON.\n");
    json_value_free(test);

    return body;
}

/* ---------------- MAIN FUNCTION ---------------- */

_Bool RegenMocksOpenAI(){
    const char* base = DEFAULT_MOCK_DIRECTORY;
    const char* nodes = DEFAULT_MOCK_DIRECTORY "nodes/";
    const char* actions = DEFAULT_MOCK_DIRECTORY "action-data/";

    if (massert(getenv("OPENAI_API_KEY"), "Missing API key")) return 0;

    printf("Seems good, wait for OpenAI to send back the result...\n\n");

    ensure_dir(base);
    ensure_dir(nodes);
    ensure_dir(actions);

    char *body = BuildOpenAIRequestBody();

    ai_openai_response r = {0};
    ai_openai_status st = ai_openai_create_response(body, &r);

    free(body);

    if (st != AI_OPENAI_OK){
        if (r.body.data) fprintf(stderr, "%s\n", r.body.data);
        cassert(0, "OpenAI failed");
        return 0;
    }

    cassert(r.body.data, "empty response");

    /* debug full response */
    write_text_file("openai_raw.json", r.body.data);

    json_value *root = json_parse(r.body.data, strlen(r.body.data));
    cassert(root, "parse failed");

    const char *bundle_text = extract_output_text_bundle(root);
    cassert(bundle_text, "failed to extract bundle");

    /* debug extracted */
    write_text_file("openai_bundle.json", bundle_text);

    json_value *bundle = json_parse(bundle_text, strlen(bundle_text));
    cassert(bundle, "bundle parse failed");

    json_value *graphs = json_obj_get(bundle, "graph_mocks");
    json_value *acts   = json_obj_get(bundle, "action_mocks");

    cassert(graphs && graphs->type == json_array, "bad graph_mocks");
    cassert(acts   && acts->type   == json_array, "bad action_mocks");

    for (unsigned i = 0; i < graphs->u.array.length; i++){
        char path[OPENAI_PATH_CAP];
        snprintf(path, sizeof(path), "%sgraph_%03u.json", nodes, i);
        cassert(write_json_file(path, graphs->u.array.values[i]), "write graph failed");
    }

    for (unsigned i = 0; i < acts->u.array.length; i++){
        char path[OPENAI_PATH_CAP];
        snprintf(path, sizeof(path), "%saction_%03u.json", actions, i);
        cassert(write_json_file(path, acts->u.array.values[i]), "write action failed");
    }

    printf("DONE: %u graphs, %u actions\n",
        graphs->u.array.length,
        acts->u.array.length
    );

    json_value_free(bundle);
    json_value_free(root);
    ai_openai_response_free(&r);

    return 1;
}
