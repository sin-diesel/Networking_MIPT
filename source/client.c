
#include "my_server.h"
/* TODO 
1) maybe add a function for handling usage */
static int which_cmd = 0;

#define MSG "Hello there, general Kenobi."


int main(int argc, char** argv) {

    int res = 0;
    /* check input */
    res = check_input(argc);
    if (res < 0) {
        return -1;
    }

    char* command = argv[1];
    char* msg = NULL;

    /* handle exit command */
    if (argc == 2) {
        if (strcmp(command, EXIT) != 0) {
            printf("Usage: ./client [--exit] [--print] <message>\n");
            exit(EXIT_FAILURE);
        }
        which_cmd = EXIT_CMD;

    /* get message from argv and send it */

    } else if (argc == 3) {
        msg = argv[2];
        if (strcmp(command, PRINT) != 0) {
            printf("Usage: ./client [--exit] [--print] <message>\n");
            return -1;
        }

        which_cmd = PRINT_CMD;
    }

    int sk = 0;

    sk = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sk < 0) {
        ERROR(errno);
        return -1;
    }

    struct sockaddr_un sk_addr;
    init_address(&sk_addr);

    // bind - для тех сокетов, который появляются в системе, для остальных не надо 

    res = connect(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
    if (res < 0) {
        ERROR(errno);
        return -1;
    }
    
    /* send exit command */
    res = send_message(sk, which_cmd, command, msg);
    if (res < 0) {
        fprintf(stderr, "Error sending message\n");
        close(sk);
        return -1;
    }
    close(sk);

    return 0;
}