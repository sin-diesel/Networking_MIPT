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

/* For multi-threaded server */
#include <pthread.h>

/* For communication between computers */
//#define COMM
//#define FRIENDIP "192.168.43.159"

/* For internet communication */
//#define INET

/* UDP server */
#define UDP


//#define LOCAL

/* TCP server */
//#define TCP

/* Debugging macros */
#define DEBUG 1
#define D(expr) if (DEBUG) { expr };
#define ERROR(error) fprintf(stderr, "Error in line %d, function %s: "         \
                                    "%s\n", __LINE__, __func__, strerror(error)) \

/* commands that a client can send to a server */
#define PRINT "--print"
#define EXIT "--exit"
#define LS "ls"
#define CD "cd"
#define BROAD "--broadcast"

#define CMDSIZE 32
#define MSGSIZE 512

struct message {
    int id;
    char cmd[CMDSIZE];
    char data[MSGSIZE];
};

struct pip {
    int rd;
    int wr;
};

/* Types of commands that can be sent to our server */
enum cmds {
    PRINT_CMD = 0,
    EXIT_CMD,
    LS_CMD,
    CD_CMD,
    BRCAST_CMD,
    BAD_CMD
};

/* Lengths of commands */
enum cmd_len {
    PRINT_LEN = 7,
    EXIT_LEN = 6,
    LS_LEN = 2,
    CD_LEN = 2,
    BROAD_LEN = 11,
};


/* Path for local communication with sockets */
#define PATH "/tmp/my_socket"

/* Port for communication */
#define PORT 23456

/* Maximum amount of clients */
#define MAXCLIENTS 100000

/* Maximum amount of pending requests */
#define MAX_QUEUE 20

int check_input(int argc, char** argv, char** command, char** arg);

int send_message(int sk, struct message* msg, int msg_len, struct sockaddr_in* sk_addr);

void* handle_connection(int* client_pipe);

int lookup(int* id_map, int n_ids, pid_t id);