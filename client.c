

#include "my_server.h"



#define DUMMY_STRING "Moscow is the capital of gread brutain"
int main(int argc, char** argv) {


    if (argc < 2) {
        printf("Usage: ./client [--exit] [--print] <message>\n");
        exit(EXIT_FAILURE);
    } 

    char* command = argv[1];
    char* msg = NULL;
    int cmd = 0;

    if (argc == 2) {

        if (strcmp(command, EXIT) != 0) {
            printf("Usage: ./client [--exit] [--print] <message>\n");
            exit(EXIT_FAILURE);
        }
        cmd = 0;

    } else if (argc == 3) {
        msg = argv[2];

        if (strcmp(command, PRINT) != 0) {
            printf("Usage: ./client [--exit] [--print] <message>\n");
            exit(EXIT_FAILURE);
        }
        cmd = 1;

    }

    struct sockaddr_un name;


    int sk, ret;

    sk = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sk < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, PATH, sizeof(PATH));

    // bind - для тех сокетов, который появляются в системе, для остальных не надо 

    ret = connect(sk, (struct sockaddr*) &name, sizeof(name));
    if (ret) {
        perror("Unable to connect to socket");
        exit(EXIT_FAILURE);
    }


    /* write message */
    if (cmd == 0) {
        ret = write(sk, command, sizeof(command));
            if (ret < 0) {
                perror("Cannot write to socket");
                exit(EXIT_FAILURE);
            }
    } else if (cmd == 1) {
            /* writing command and message */
            ret = write(sk, command, sizeof(command));
            if (ret < 0) {
                perror("Cannot write to socket");
                exit(EXIT_FAILURE);
            }

            int len = strlen(msg);
            printf("Sending message, length %d: %s\n", len, msg);
            ret = write(sk, msg, len);
            if (ret < 0) {
                perror("Cannot write to socket");
                exit(EXIT_FAILURE);
            }
    }




    close(sk);

    return 0;
}