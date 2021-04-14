
#include <stdio.h>
#include "field.h"


static int mn_sum(int a, int b);
static struct field mn_field = {
	.m = 0,
	.sum = &mn_sum,
};

static int mn_sum(int a, int b) {
	return (a + b) % mn_field.m;
};

static void my_load() __attribute__((constructor)); 

static void my_load() {
	printf("I am loaded!\n");
	register_field(&mn_field);
}

