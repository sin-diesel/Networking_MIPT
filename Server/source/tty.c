#include "my_server.h"


int main(int argc, char** argv) {

    struct termios term;
    int ret = 0;
    int tty = STDOUT_FILENO;

    ret = tcgetattr(tty, &term);
    if (ret < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    printf("c_iflag: %d\n", term.c_iflag);
    printf("c_oflag: %d\n", term.c_oflag);
    printf("c_cflag: %d\n", term.c_cflag);
    printf("c_lflag: %d\n", term.c_lflag);


    return 0;
}