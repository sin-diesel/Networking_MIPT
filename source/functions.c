#include "my_server.h"


int check_input(int argc) {
    if (argc < 2) {
        printf("Usage: ./client [--exit] [--print] <message>\n");
        return -1;
    } 
    return 0;
}

int send_message(int sk, int which_cmd, char* command, char* msg) {
    int res = 0;
    int cmd_len = 0;

    /* writing only exit command */
    if (which_cmd == EXIT_CMD) {
        cmd_len = strlen(command);
        res = write(sk, command, cmd_len);
        if (res < 0 || res != cmd_len) {
            ERROR(errno);
            return -1;
        }

    /* writing print command and message */
    } else if (which_cmd == PRINT_CMD) {

        int cmd_len = strlen(command);
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

    return 0;
}

void init_address(struct sockaddr_un* sk_addr) {
    sk_addr->sun_family = AF_UNIX;
    strncpy(sk_addr->sun_path, PATH, sizeof(PATH));
}

void print_message(char* buf) {
    /* advance pointer */
    char* msg = buf + PRINT_L;
    printf("Message from client: %s\n", msg);
}

int read_message(int client_sk) {
    /* message buffer */
    char buf[BUFSZ] = {};
    char msg[BUFSZ] = {};
    int res = 0;
    res = read(client_sk, buf, BUFSZ);

    if (res < 0 || res >= BUFSZ) {
        //fprintf(stderr, "Unexpected read error or overflow %d\n", res);
        ERROR(errno);
        return -1;
    }

    /* Commands are: PRINT, EXIT */
    if (strcmp(buf, PRINT) == 0) {
        /* read message and print it */
        res = read(client_sk, msg, BUFSZ);
        if (res < 0 || res >= BUFSZ) {
            printf("Unexpected read error or overflow %d\n", res);
            return -1;
        }
        /* Print message */
        print_message(msg);

    } else if (strcmp(buf, EXIT) == 0) {
        close(client_sk);
        unlink(PATH);
        exit(EXIT_SUCCESS);
    } else {
        printf("Command from client not recognized\n");
    }
    return 0;
}

