#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* for sockets */
#include <sys/socket.h>
#include <sys/un.h>

/* for ip addressing */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

//#define COMM

//#define FRIENDIP "192.168.43.159"

//#define INET

#define UDP

//#define LOCAL

//#define TCP

/* Debugging macro */
#define ERROR(error) fprintf(stderr, "Error in line %d, function %s: "         \
                                    "%s\n", __LINE__, __func__, strerror(error)) \

/* commands that a client can send to a server */
#define PRINT "--print"
#define EXIT "--exit"
#define LS "ls"
#define CD "cd"


enum commands {
    PRINT_CMD = 0,
    EXIT_CMD,
    LS_CMD,
    CD_CMD
};


/* Path for local communication with sockets */
#define PATH "/tmp/my_socket"

/* Buf size for messages */
#define BUFSZ 1024

/* Maximum amount of pending requests */
#define MAX_QUEUE 20

int check_input(int argc);