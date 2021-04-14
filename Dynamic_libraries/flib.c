#include "field.h"

static struct field* fields[256];

int register_field(struct field* f) {
	if (f->m > 256) {
		return -1;}

	fields[f-> m] = f;

	return 0;
}

int sum(int m, int a, int b) {
	struct field* f;

	f = fields[m];
	if (!f) {
		f = fields[0];
		f->m = m;
	}

	return f->sum(a, b);
}