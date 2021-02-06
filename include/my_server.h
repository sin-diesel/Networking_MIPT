#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* for sockets */
#include <sys/socket.h>
#include <sys/un.h>

/* Debugging macro */
#define ERROR(error) fprintf(stderr, "Error in line %d, function %s: "         \
                                    "%s\n", __LINE__, __func__, strerror(error)) \

/* commands that a client can send to a server */
#define PRINT "--print"
#define EXIT "--exit"

/* redundant */
enum offsets {
    PRINT_L = 0,
    EXIT_L = 6
};

enum commands {
    PRINT_CMD = 0,
    EXIT_CMD,
};


#define PATH "/tmp/my_socket"
/* Buf size for messages */
#define BUFSZ 1024

#define MAX_QUEUE 20

void init_address(struct sockaddr_un* sk_addr);

int send_message(int sk, int which_cmd, char* command, char* msg);

int check_input(int argc);

void print_message(char* buf);

int read_message(int client_sk);