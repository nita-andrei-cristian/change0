#include <stdio.h>
#include "cli/ui.h"

int main(){
	UIStart();
	UILoop();
	UIKill();

	printf("Hello world!\n");
}
