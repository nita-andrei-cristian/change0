/*
 * This is not made by me!
 * */

#ifndef HASHDICTC
#define HASHDICTC
#include <stdint.h> /* uint32_t */
#include <xmmintrin.h>

#define HASHDICT_VALUE_TYPE long
#define KEY_LENGTH_TYPE uint8_t

typedef int (*enumFunc)(void *key, int count, HASHDICT_VALUE_TYPE *value, void *user);

struct keynode {
	struct keynode *next;
	char *key;
	KEY_LENGTH_TYPE len;
	HASHDICT_VALUE_TYPE value;
};
		
struct dictionary {
	struct keynode **table;
	int length, count;
	double growth_treshold;
	double growth_factor;
	HASHDICT_VALUE_TYPE *value;
};

struct dictionary* dic_new(int initial_size);
void dic_delete(struct dictionary* dic);
int dic_add(struct dictionary* dic, void *key, int keyn);
int dic_find(struct dictionary* dic, void *key, int keyn);
void dic_forEach(struct dictionary* dic, enumFunc f, void *user);
#endif
