

#include "logger.h"

int main () {
	int ret = 0;
	ret = log_init(NULL);
	if (ret < 0) {
		perror("Init error: ");
	}

	pr_err("Im here!");
	return 0;
}