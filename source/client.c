
#include "my_server.h"
/* TODO 
1) maybe add a function for handling usage */
static int which_cmd = 0;

#define MSG "Hello there, general Kenobi."


int main(int argc, char** argv) {

    const char usage[] = "Usage: ./client [--exit] [--print] <message>, [ls], [cd] <dir>\n";
    int res = 0;
    /* check input */
    if (argc < 2) {
        printf(usage);;
        return -1;
    } 

    char* command = argv[1];
    char* msg = NULL;

    /* handle 2 argc command */
    if (argc == 2) {
        if (strcmp(command, EXIT) == 0) {
            which_cmd = EXIT_CMD;
        }

        if (strcmp(command, LS) == 0) {
            which_cmd = LS_CMD;
        }

        if (strcmp(command, CD) == 0) {
            which_cmd = CD_CMD;
        }

    /* get message from argv and send it */

    } else if (argc == 3) {
        msg = argv[2];
        if (strcmp(command, PRINT) == 0) {
            which_cmd = PRINT_CMD;
        }

    }

    int sk = 0;

    #ifdef UDP
    sk = socket(AF_INET, SOCK_DGRAM, 0);
    #endif

    #ifdef INET
    sk = socket(AF_INET, SOCK_STREAM, 0);
    #endif

    #ifdef LOCAL
    sk = socket(AF_UNIX, SOCK_STREAM, 0);
    #endif

    if (sk < 0) {
        ERROR(errno);
        return -1;
    }

    /* init socket address and family */
    #ifdef TCP

    #ifdef INET

    struct sockaddr_in sk_addr = {0};
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(23456); /* using here htons for network byte order conversion */
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* same goes for ip */

    #endif

    #ifdef COMM

    struct in_addr addr;
    res = inet_pton(AF_INET, FRIENDIP, &addr);
    if (res != 1) {
        printf("IP address is invalid\n");
    }
    sk_addr.sin_addr.s_addr = addr.s_addr;

    #endif


    #ifdef LOCAL

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

    #endif

    #ifdef UDP
        
    struct sockaddr_in sk_addr;
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(23456); /* using here htons for network byte order conversion */
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);


    /* send exit command */
    int cmd_len = 0;
    int len = 0;

    cmd_len = strlen(command); // fix later
    printf("Command len: %d\n", cmd_len);

    /* writing only exit command */
    if (which_cmd == EXIT_CMD) {

        res =  sendto(sk, command, cmd_len, 0, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
        if (res < 0) {
            ERROR(errno);
            return -1;
        }
    }

    /* writing print command and message */
    else if (which_cmd == PRINT_CMD) {


        res =  sendto(sk, command, cmd_len, 0, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

        if (res < 0) {
            ERROR(errno);
            return -1;
        }

        len = strlen(msg);
        /* null terminating message */
        msg[len] = '\0';

        res =  sendto(sk, msg, len, 0, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

        if (res < 0) {
            ERROR(errno);
            return -1;
        }

    } else if (which_cmd == LS_CMD) {
        res =  sendto(sk, command, cmd_len, 0, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

    }

    else if (which_cmd == CD_CMD) {
        res =  sendto(sk, command, cmd_len, 0, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

        /* cd arguments */
        if (argc > 2) {
            char* arg = argv[2];
            int arg_len = strlen(arg);
            res =  sendto(sk, arg, arg_len, 0, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
        }


    }

    if (res < 0) {
        fprintf(stderr, "Error sending message\n");
        close(sk);
        return -1;
    }
    close(sk);

    #endif

    return 0;
}