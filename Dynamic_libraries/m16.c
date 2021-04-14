
#include <stdio.h>
#include "field.h"


static int m_sum_16(int a, int b);
static struct field mn_field = {
	.m = 0,
	.sum = &m_sum_16,
};

static int m_sum_16(int a, int b) {
	return (a + b) & 0b1111;
};

static void my_load_16() __attribute__((constructor)); 

static void my_load_16() {
	printf("I am loaded!\n");
	register_field(&mn_field);
}
