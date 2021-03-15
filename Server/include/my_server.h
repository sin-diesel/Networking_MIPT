#pragma once


#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <termios.h>
#include <assert.h>
#include <sys/stat.h>

/* for sockets */
#include <sys/socket.h>
#include <sys/un.h>

/* for ip addressing */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

/* For multi-threaded server */
#include <pthread.h>

/* UDP server */
#define UDP

/* Debugging macros */
#define DEBUG 0
#define D(expr) if (DEBUG) { expr; }
#define ERROR(error) fprintf(stderr, "Error in line %d, function %s: "         \
                                    "%s\n", __LINE__, __func__, strerror(error)) \

/* Logger macros */
FILE* log_file;
/* Path for daemon logs */
static const char log_path[] = "/var/log/server.log";

#define LOG(expr, ...)  do { \
                  log_file = fopen(log_path, "a");      \
                  if (log_file == NULL) {               \
                      fprintf(stderr, "Error opening log file\n"); \
                      exit(EXIT_FAILURE);               \
                  }                                     \
                  fprintf(log_file, expr, __VA_ARGS__); \
                  fflush(log_file);                     \
                  fclose(log_file);                     \
                } while (0);                            \

/* Commands that a client can send to a server */
#define PRINT "print"
#define EXIT "exit"
#define LS "ls"
#define CD "cd"
#define BROAD "broadcast"
#define SHELL "shell"
#define QUIT "quit"

/* Command and message size */
#define CMDSIZE 32
#define MSGSIZE 1024

/* Max path that can be sent via cd command */
#define MAXPATH 1024




/* Client_data field to send info back to client */
struct message {    
    int id;
    char cmd[CMDSIZE];
    char data[MSGSIZE];
    struct sockaddr_in client_data;
};

enum connection_type {
    UDP_CON = 0,
    TCP_CON,
};

/* Types of commands that can be sent to our server */
enum cmds {
    PRINT_CMD = 0,
    EXIT_CMD,
    LS_CMD,
    CD_CMD,
    BRCAST_CMD,
    SHELL_CMD,
    QUIT_CMD,
    BAD_CMD
};

/* Lengths of commands */
enum cmd_len {
    PRINT_LEN = sizeof(PRINT) - 1,
    EXIT_LEN = sizeof(EXIT) - 1,
    LS_LEN = sizeof(LS) - 1,
    CD_LEN = sizeof(CD) - 1 ,
    BROAD_LEN = sizeof(BROAD) - 1,
    SHELL_LEN = sizeof(SHELL) - 1,
    QUIT_LEN = sizeof(QUIT) - 1
};


/* Path for local communication with sockets */
#define PATH "/tmp/my_socket"

/* Port for communication */
#define PORT 23456

/* Maximum amount of clients */
#define MAXCLIENTS 100000

/* Maximum amount of pending requests */
#define BACKLOG 20

/* Mutexes which are responsible for threads */
extern pthread_mutex_t mutexes[];

int check_input(int argc, char** argv, char** command, char** arg);

int send_message(int sk, struct message* msg, int msg_len, struct sockaddr_in* sk_addr);

int lookup(int* id_map, int n_ids, pid_t id);

void print_info(struct message* msg);

int init_shell(int* pid);

void start_shell(char* buf, char* input, char* cwd);

void init_daemon();

void mutex_init(pthread_mutex_t* mutexes, int* id_map);

void udp_get_msg(int sk, struct sockaddr_in* sk_addr, struct message* msg, struct sockaddr_in* client_data);

void* handle_connection(void* memory);

void check_thread(pthread_t* thread_ids, struct message* thread_memory, int* id_map, struct message* msg, void* handle_connection);

void terminate_server(int sk);

void send_broadcast(int sk, struct message* msg, struct sockaddr_in* client_data);

void reply_to_client(struct message* msg);

void send_to_server(int sk, struct message* msg, struct sockaddr_in* sk_addr, \
                     struct sockaddr_in* server_data, socklen_t* addrlen); 

void ask_broadcast(int sk, struct message* msg, struct sockaddr_in* sk_broad, \
                     struct sockaddr_in* server_data, socklen_t* addrlen);

int get_input(char* input);

int get_cmd(char* input, char* cmd);

int get_args(char* input, char* args);

void addr_init(struct sockaddr_in* sk_addr, in_addr_t addr);

void udp_work();

void tcp_work();

void construct_input(char* cmd, char* new_input, char* cwd);