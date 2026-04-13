#define _POSIX_C_SOURCE 200809L // for setenv
				//
#include "cli/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void load_env(const char *filename) {
    FILE *file = fopen(filename, "r");
    if(!file) return;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (key && value) {
            setenv(key, value, 1);
        }
    }

    fclose(file);
}

int main(){

	load_env(".env");

	UIStart();
	UILoop();
	UIKill();
}
