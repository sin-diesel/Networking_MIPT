
#include "logger.h"

static char buf[BUFSIZ];
static int logfd;
static int pos;


int log_init(char* path) {

	if (path == NULL) {
		logfd = open("/var/log/logger.log", O_WRONLY | O_CREAT, 0666);
		if (logfd < 0) {
			return -1;
		}
	} else {
		logfd = open(path, O_WRONLY | O_CREAT, 0666);
		if (logfd < 0) {
			return -1;
		}
	}

	return logfd;
}

int pr_date() {
   time_t curtime;

   time(&curtime);
	char* tm = ctime(&curtime);
	int len = strlen(tm);

	int ret = sprintf(buf + pos, tm);
	if (ret != len) {
		printf("error writing time to buf");
	}

	pos += ret;
}	

int pr_pid() {
	int pid = getpid();
	int ret = sprintf(buf + pos, "PID: %d ", pid);

	pos += ret;

}

int pr_warn(int log_level) {
	int ret = 0;
	if (log_level == LOG_ERR) {
		ret = sprintf(buf + pos, "[] %d ", pid);
	}
	pos += ret;
}




int pr_log_level(int log_level, char* fmt, ...) {
	if (logfd < 0) {
		log_init(NULL);
	}

	pr_date();
	pr_pid();
	pr_warn(log_level);

	va_list params;
	va_start(params, fmt);

	int ret = 0;

	ret = vsnprintf(buf + pos, BUFSIZ - pos, fmt, params);
	printf("pos: %d\n", pos);
	printf("Bytes written: %d\n", ret);
	perror("Buffer:");

	va_end(params);

	if (write(logfd, buf, BUFSIZ) < 0) {
		perror("Error writing to fd: ");
	}

}