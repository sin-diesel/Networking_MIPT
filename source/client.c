
#include "my_server.h"
/* TODO 
1) maybe add a function for handling usage */
static int which_cmd = 0;

#define MSG "Hello there, general Kenobi."


int main(int argc, char** argv) {

    int res = 0;
    /* check input */
    if (argc < 2) {
        printf("Usage: ./client [--exit] [--print] <message>\n");
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

    #ifdef INET
    sk = socket(AF_INET, SOCK_STREAM, 0);
    #else
    sk = socket(AF_UNIX, SOCK_STREAM, 0);
    #endif

    if (sk < 0) {
        ERROR(errno);
        return -1;
    }

    /* init socket address and family */
    #ifdef INET
    struct sockaddr_in sk_addr = {0};
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(23456); /* using here htons for network byte order conversion */

    #ifdef COMM
    struct in_addr addr;
    res = inet_pton(AF_INET, FRIENDIP, &addr);
    if (res != 1) {
        printf("IP address is invalid\n");
    }
    sk_addr.sin_addr.s_addr = addr.s_addr;
    #else
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* same goes for ip */
    #endif


    #else
    struct sockaddr_un sk_addr = {0};
    sk_addr.sun_family = AF_UNIX;
    strncpy(sk_addr.sun_path, PATH, sizeof(sk_addr.sun_path) - 1);
    #endif

    // bind - для тех сокетов, который появляются в системе, для остальных не надо 

    res = connect(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
    if (res < 0) {
        ERROR(errno);
        return -1;
    }
    
    /* send exit command */
    int cmd_len = 0;

    /* writing only exit command */
    if (which_cmd == EXIT_CMD) {
        cmd_len = strlen(command);
        res = write(sk, command, cmd_len);
        if (res < 0 || res != cmd_len) {
            ERROR(errno);
            return -1;
        }
    }

    /* writing print command and message */
    else if (which_cmd == PRINT_CMD) {

        cmd_len = strlen(command);
        /* write null terminating char */
        res = write(sk, command, cmd_len);

        if (res < 0 || res != cmd_len) {
            ERROR(errno);
            return -1;
        }

        int len = strlen(msg);
        printf("Sending message: %s\n length of message: %d\n", msg, len);

        /* write null terminating char */
        res = write(sk, msg, len);

        if (res < 0 || res != len) {
            ERROR(errno);
            return -1;
        }
    }

    
    if (res < 0) {
        fprintf(stderr, "Error sending message\n");
        close(sk);
        return -1;
    }
    close(sk);

    return 0;
}