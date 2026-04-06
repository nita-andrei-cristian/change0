/*
#include "search.h"
#include "node.h"
#include "../util.h"
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include "stdio.h"
#include "string.h"

static inline void swap_double(double *a, double *b)
{
    double t = *a;
    *a = *b;
    *b = t;
}

struct Connection* FindNeighbours(Node* node){
	return node->neighbours;
}

// AI GENERATED CODE FOR QUICK_SELECT
static int partition_desc_double(double arr[], int left, int right)
{ double pivot = arr[right];
    int store = left;

    for (int i = left; i < right; i++) {
        if (arr[i] > pivot) {
            swap_double(&arr[i], &arr[store]);
            store++;
        }
    }

    swap_double(&arr[store], &arr[right]);
    return store;
}

static double quickselect_desc_double(double arr[], int count, int k)
{
    int left = 0;
    int right = count - 1;

    while (left <= right) {
        int pivot_index = partition_desc_double(arr, left, right);

        if (pivot_index == k)
            return arr[pivot_index];

        if (k < pivot_index)
            right = pivot_index - 1;
        else
            left = pivot_index + 1;
    }

    return arr[k];
}

static inline int top_count_percent(int total, int percent)
{
    return (total * percent + 99) / 100;
}

// Used AI to recreate a generic function
void **FilterTopPercent(
    void *container,
    size_t containerSize,
    size_t elemSize,
    int_fast64_t percentage,
    size_t *count,
    GetValueFn getValue
){
    if (!container || containerSize == 0 || !count || !getValue)
        return NULL;

    *count = 0;
    percentage = CLAMP(0, 100, percentage);

    int n = (int)containerSize;
    int top_n = top_count_percent(n, percentage);
    if (top_n <= 0)
        return NULL;

    int k = top_n - 1;

    double *values = malloc(n * sizeof(*values));
    if (!values)
        return NULL;

    for (size_t i = 0; i < containerSize; i++) {
        void *elem = (char*)container + i * elemSize; // use char to symbolize one byte
        values[i] = getValue(elem);
    }

    double threshold = quickselect_desc_double(values, n, k);
    free(values);

    void **result = malloc(containerSize * sizeof(*result));
    if (!result)
        return NULL;

    for (size_t i = 0; i < containerSize; i++) {
        void *elem = (char*)container + i * elemSize;
        if (getValue(elem) >= threshold) {
            result[(*count)++] = elem;
        }
    }

    return result;
}

// TODO : Refactor this since it's similar to FilterTopPercent function.
Node** FilterNodeNeighboursByActivation(Node* node, int_fast64_t percentage, size_t *count){
	percentage = CLAMP(0, 100, percentage);

	int top_n = top_count_percent(node->ncount, percentage);
	if (top_n <= 0)
		return NULL;

	double *values = malloc(node->ncount * sizeof(*values));
	if (!values)
		return NULL;

	for (size_t i = 0; i < node->ncount; i++) {
		values[i] = node->neighbours[i].activation;
	}

	double threshold = quickselect_desc_double(values, node->ncount, top_n - 1);
	free(values);

	Node **result = malloc(node->ncount * sizeof(*result));
	if (!result)
		return NULL;

	for (size_t i = 0; i < node->ncount; i++) {
		if (node->neighbours[i].activation >= threshold) {
			result[(*count)++] = node->neighbours[i].target;
		}
	}

	return result;

}

// TODO : Refactor this since it's similar to FilterTopPercent function.
Node** FilterNodeNeighboursByWeight(Node* node, int_fast64_t percentage, size_t *count){
	percentage = CLAMP(0, 100, percentage);

	int top_n = top_count_percent(node->ncount, percentage);
	if (top_n <= 0)
		return NULL;

	double *values = malloc(node->ncount * sizeof(*values));
	if (!values)
		return NULL;

	for (size_t i = 0; i < node->ncount; i++) {
		values[i] = node->neighbours[i].weight;
	}

	double threshold = quickselect_desc_double(values, node->ncount, top_n - 1);
	free(values);

	Node **result = malloc(node->ncount * sizeof(*result));
	if (!result)
		return NULL;

	for (size_t i = 0; i < node->ncount; i++) {
		if (node->neighbours[i].activation >= threshold) {
			result[(*count)++] = node->neighbours[i].target;
		}
	}

	return result;

}

// We assume the node exists, if not, too bad.
char* recursive_step(Node* node, int_fast64_t pA, int_fast64_t pW, size_t depth, double lastA, double lastW, size_t *count){
	if (depth == 0 || node == NULL) return NULL;
	if (depth == 1){
		// last one
		char* out = malloc(128 + NODE_LABEL_CAP);
		if (!out) return NULL;
		if (lastA || lastW)
			*count = sprintf(out, "{\"NodeName\" : \"%s\", \"act\" : %.2f, \"wght\" : %.2f},", node->label, lastA, lastW);
		else
			*count = sprintf(out, "{\"NodeName\" : \"%s\"},", node->label);
		
		return out;
	}

	pA = CLAMP(0, 100, pA);
	pW = CLAMP(0, 100, pW);

	int top_n_A = top_count_percent(node->ncount, pA);
	if (top_n_A <= 0)
		return NULL;

	int top_n_W = top_count_percent(node->ncount, pW);
	if (top_n_W <= 0)
		return NULL;

	double *valuesA = malloc(node->ncount * sizeof(*valuesA));
	if (!valuesA)
		return NULL;

	double *valuesW = malloc(node->ncount * sizeof(*valuesW));
	if (!valuesW){
		free(valuesA);
		return NULL;
	}

	for (size_t i = 0; i < node->ncount; i++) {
		valuesA[i] = node->neighbours[i].activation;
		valuesW[i] = node->neighbours[i].weight;
	}

	double thresholdA = quickselect_desc_double(valuesA, node->ncount, top_n_A - 1);
	double thresholdW = quickselect_desc_double(valuesW, node->ncount, top_n_W - 1);
	free(valuesA);
	free(valuesW);

	size_t capacity = 1024;
	char *out = malloc(capacity);
	if(!out) return NULL;
	*count = 0;

	char buff[256 + NODE_LABEL_CAP];
	size_t header_len;
	if (lastA || lastW)
		header_len = sprintf(buff, "{\"NodeName\" : \"%s\", \"act\" : %.2f, \"wght\" : %.2f, \"children\" : [", node->label, lastA, lastW);
	else
		header_len = sprintf(buff, "{\"NodeName\" : \"%s\", \"children\" : [", node->label);

	memcpy(out, buff, header_len); *count += header_len;

	for (size_t i = 0; i < node->ncount; i++) {
		if (node->neighbours[i].activation >= thresholdA && node->neighbours[i].weight >= thresholdW) {
			char *item;
			size_t len = 0;

			item = recursive_step(node->neighbours[i].target, pA, pW, depth - 1, node->neighbours[i].activation, node->neighbours[i].weight, &len);
			if (!item) continue;

			size_t new_capacity = capacity;
			while(*count + len + 3 >= new_capacity){ // we append 3 chars at the end
				// out of bounds;
				new_capacity *= 2;
			}

			if (new_capacity >= capacity){
				char* tmp = realloc(out, new_capacity);
				if (!tmp){
					fprintf(stderr, "Error : Critical, coudln't allocate more memory when computing node family");
					free(out);
					return NULL;
				}
				out = tmp;
				capacity = new_capacity;
			}

			memcpy(out + *count, item, len);
			*count += len;
			free(item);
		}
	}
	if (*count > 0 && out[*count-1] == ',') (*count)--; // remove tail comma

	memcpy(out + *count, "]},", 3); *count += 3;

	return out;
}

char* ComputeNodeFamily(Node* node, int_fast64_t percA, int_fast64_t percW, size_t depth, size_t *length){
	if (!node || !length) return NULL;
	if (depth > MAX_FAMILY_DEPTH) depth = MAX_FAMILY_DEPTH;

	*length = 0;

	size_t count;
	char* root = recursive_step(node, percA, percW, depth, 0, 0, &count);
	if(!root) {
		return NULL;
	}

	char* base = malloc(count + 256);
	if (!base){
		free(root);
		return NULL;
	}

	char s[256];
	size_t header_len = sprintf(s, "Performing Depth search with filter on Weight [%ld] and Activation [%ld]:\n", percW, percA);
	memcpy(base + *length, s, header_len); *length += header_len;

	if (count > 0 && root[count-1] == ',') count--;
	memcpy(base + *length, root, count); *length += count;
	base[*length] = '\0';

	free(root);

	return base;
}

struct Connection **FilterConnectionsByActivation(struct Connection *container, size_t containerSize, int_fast64_t percentage, size_t *count){
	return (struct Connection**) FilterTopPercent((void*) container, containerSize, sizeof(struct Connection), percentage, count, (GetValueFn) ReadConnectionActivation);
}

struct Connection **FilterConnectionsByWeight(struct Connection *container, size_t containerSize, int_fast64_t percentage, size_t *count){
	return (struct Connection**) FilterTopPercent((void*) container, containerSize, sizeof(struct Connection), percentage, count, (GetValueFn) ReadConnectionWeight);
}

Node** FilterNodeByActivationGlobal(int_fast64_t percentage, size_t *count){
	return (Node**) FilterTopPercent((void*)Nodes.items, Nodes.count, sizeof(Node), percentage, count, (GetValueFn) ReadNodeActivation);
}

*/
