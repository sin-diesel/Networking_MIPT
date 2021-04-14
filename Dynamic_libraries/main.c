#define _GNU_SOURCE	
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "field.h"

int main(int argc, char** argv) {

	int res = 0;
	int* ret = NULL;
	ret = dlopen(argv[1], RTLD_NOW);
	if (ret == NULL) {
		printf("Error in dlopen\n");
		perror("Error: ");
		char* error = dlerror();
		printf("%s\n", error);
		return -1;
	}
	for (int i = 0; i < 100000000; ++i) {
		res = sum(16, 12345678, 837421);
		//printf("Result: %d", res);
	}
	printf("Result: %d", res);

}


// ./a.out mn.so