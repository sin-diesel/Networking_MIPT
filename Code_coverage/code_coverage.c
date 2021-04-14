
#include <stdlib.h>
#include <time.h>

static size_t foo1(size_t a, size_t b) {
	return a + b;
}

static size_t foo2(size_t a, size_t b) {
	return a * b;
}



int main() {

	size_t i, a, b, tmp;

	srand(time(NULL));

	for (i = 0; i < 10000000; ++i) {
		a = rand();
		b = rand();
		if (a > b) {
			tmp = foo1(a, b);
		} else {
			tmp = foo2(a, b);
		}
		tmp++;
	}		
	return 0;
}