#include "my_server.h"


/* return number of command to be sent to server */
int check_input(int argc, char** argv, char** command, char** arg) {
    
    const char usage[] = "Usage: ./client [--exit] [--print] <message>, [ls], [cd] <dir>\n";
    /* check input */
    if (argc < 2) {
        printf(usage);
        return BAD_CMD;
    } 

    printf("Command: %s\n", argv[1]);
    *command = argv[1];

    
    /* handle 2 argc command */
    if (argc == 2) {
        if (strcmp(*command, EXIT) == 0) {
            return EXIT_CMD;
        } else if (strcmp(*command, LS) == 0) {
            return LS_CMD;
        } else if (strcmp(*command, CD) == 0) {
            return CD_CMD;
        } else if (strcmp(*command, BROAD) == 0) {
            return BRCAST_CMD;
        } else {
            printf(usage);
            return BAD_CMD;
        }
    }
    /* get message from argv and send it */

    if (argc == 3) {
        *arg = argv[2];
        if (arg == NULL) {
            printf(usage);
            return BAD_CMD;
        }
        if (strcmp(*command, PRINT) == 0) {
            return PRINT_CMD;
        } else if (strcmp(*command, CD) == 0) {
            return CD_CMD;
        }
    }

    /* No match */
    printf(usage);

    return BAD_CMD;
}

int send_message(int sk, char* msg, int msg_len, struct sockaddr_in* sk_addr) {
    int ret = sendto(sk, msg, msg_len, 0, (struct sockaddr*) sk_addr, sizeof(*sk_addr));
    if (ret < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }
    return ret;
}

