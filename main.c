/* assembly assembly assembly */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

struct actor {
    char *name;
	size_t size;
	uint32_t code[];
};

void parse(const char *input,
           struct actor *actor) {
	
}

int main() {
    struct actor actor;
    parse(, &actor);
}
