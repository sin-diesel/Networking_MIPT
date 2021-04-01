
#include "my_server.h"

/* TODO
1) Refactor daemon, upgrade logger macros DONE
2) Replace printfs with writing to log file DONE
3) Add client input  DONE
4) Add broadcast DONE
5) Add exit in server and client DONE
6) Add shell command to change cwd in corresponding thread  DONE
7) Fix no job control when first starting bash DONE
8) add cd arguments to bash DONE
9) add tcp_init and udp_init functions DONE
10) split big output from bash into packages DONE
11) Fix bash settings, add continous bash operation
12) Fix mutexes locking/unlocking DONE
13) fix buffers DONE
*/

/* Print message info */
void print_info(struct message* msg) {
    LOG("Message received: %s\n", "");
    LOG("ID: %d\n", msg->id);
    LOG("Command: %s\n", msg->cmd);
    LOG("Data: %s\n", msg->data);
    LOG("Client sk: %d\n", msg->client_sk);
}

//---------------------------------------------------
/* Check options passed to server */
int check_input(int argc, char** argv, int* connection_type) {
    if (argc == 2) {
        if (strcmp(argv[1], "--udp") == 0) {
            printf("UDP connection setting set.\n");
            *connection_type = UDP_CON;
            return 0;
        } else if (strcmp(argv[1], "--tcp") == 0) {
            printf("TCP connection setting set.\n");
            *connection_type = TCP_CON;
            return 0;
        } else {
            return -1;
        }
    }
    return -1;
}


int print_client_addr(struct message* msg) {
    char* addr = inet_ntoa(msg->client_data.sin_addr);
    if (addr == NULL) {
        return -1;
    }

    LOG("Client address: %s\n", addr);
    LOG("Client port: %d\n", msg->client_data.sin_port);
    return 0;
}

//---------------------------------------------------
/* Decode which kind of message was sent to server */
int handle_message(struct message* msg, char* dir, char* buf) {
    int ret = 0;

    /* Manually change directory if command is cd */
    if (strncmp(msg->cmd, CD, CD_LEN) == 0) {
        LOG("Cwd: %s\n", dir);
        memcpy((void*) dir, msg->data, MSGSIZE);
        LOG("Changing cwd to %s\n", msg->data);
        return 0;
    } else if (strncmp(msg->cmd, EXIT, EXIT_LEN) == 0) {
    /* Quit server */
        terminate_server();
    } else {
    /* Otherwise start shell and execute command */
        ret = shell_execute(buf, msg, dir);
        if (ret < 0) {
            LOG("Errors executing cmd in shell.%s\n", "");
            return -1;
        }
        /* Copy data from shell return buf to msg */
        memcpy(msg->data, buf, MSGSIZE);
        LOG("Data ready to be sent to client: %s\n", msg->data);
        }
    return 0;
}

/* Initialize socket, mutexes */
//---------------------------------------------------
int server_init(int connection_type, int* sk, struct sockaddr_in* sk_addr, int* id_map,
                struct message** memory, pthread_mutex_t* mutexes) {
    int ret = 0;
    /* Run server as daemon */
    init_daemon();

    /* Create and initialize socket */
    if (connection_type == UDP_CON) {
        *sk = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        *sk = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (*sk < 0) {
        ERROR(errno);
        LOG("Error opening socket: %s\n", strerror(errno));
        return -1;
    }

    /* Init socket address and family */
    addr_init(sk_addr, INADDR_ANY);

    ret = bind(*sk, (struct sockaddr*) sk_addr, sizeof(*sk_addr));
    if (ret < 0) {
        LOG("Error binding: %s\n", strerror(errno));
        ERROR(errno);
        close(*sk);
        return -1;
    }

    ret = mutex_init(mutexes, id_map);
    if (ret < 0) {
        return -1;
    }

    /* Allocating memory for sharing between threads */
    *memory = (struct message*) calloc(MAXCLIENTS, sizeof(struct message));
    if (*memory == NULL) {
        LOG("Error allocating memory for clients: %s\n", strerror(errno));
        return -1;
    }

    /* First get ready for listening */
    if (connection_type == TCP_CON) {
        ret = listen(*sk, BACKLOG);
        if (ret < 0) {
            ERROR(errno);
            return -1;
        }
    }

    return 0;
}

int client_init(int connection_type, int* sk, char* ip_addr, struct sockaddr_in* sk_addr,
                struct sockaddr_in* sk_bind, struct sockaddr_in* sk_broad) {
    /* Init socket address and family */
    in_addr_t ipin_addr = -1;
    struct sockaddr_in server_data;
    socklen_t addrlen = sizeof(server_data);
    int ret = 0;

    if (connection_type == UDP_CON) {
        *sk = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        *sk = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (*sk < 0) {
        ERROR(errno);
        return -1;
    }

    /* Initialize client address, either with loopback or with IP */
    if (ip_addr == NULL) {
        addr_init(sk_addr, INADDR_LOOPBACK);
    } else {
        ipin_addr = inet_addr(ip_addr);
        if (ipin_addr < 0) {
            printf("Error converting IP to valid address.\n");
            return -1;
        }
        addr_init(sk_addr, ipin_addr);
        struct in_addr addr;
        ret = inet_pton(AF_INET, ip_addr, &addr);
        if (ret != 1) {
            printf("IP address is invalid\n");
        }
        sk_addr->sin_addr.s_addr = addr.s_addr;
        printf("IP address of server assigned:%s\n", ip_addr);
    }

    /* Binding socket */
    addr_init(sk_bind, INADDR_ANY);

    /* Allowing broadcast */
    int confirm = 1;
    setsockopt(*sk, SOL_SOCKET, SO_BROADCAST, &confirm, sizeof(confirm));

    addr_init(sk_broad, INADDR_BROADCAST);

    /* Use connect if TCP enabled */
    if (connection_type == TCP_CON) {
        ret = connect(*sk, sk_addr, addrlen);
        if (ret < 0) {
            ERROR(errno);
            return -1;
        }
    }
    return 0;
}

int parse_input(char* input, char* cmd, char* args) {

    int ret = 0;
    int input_len = 0;
    int cmd_len = 0;
    int args_len = 0;

    ret = get_input(input);
    if (ret < 0) {
        printf("Error in input.\n");
        return -1;
    }

    input_len = strlen(input); // fix later

    printf("Input: %s\n", input);
    printf("Input length: %d\n", input_len);

    ret = get_cmd(input, cmd);
    if (ret < 0) {
        printf("Error in parsing command.\n");
        return -1;
    }

    cmd_len = strlen(cmd);
    printf("Cmd: %s\n", cmd);
    printf("Cmd length: %d\n", cmd_len);

    ret = get_args(input, args);
    if (ret < 0) {
        printf("Error in parsing args.\n");
        printf("No args provided.\n");
    }

    if (ret >= 0) {
        args_len = strlen(args);
        printf("Args: %s\n", args);
        printf("Args length: %d\n", args_len);
    }

    return 0;
}

int construct_command(char* input, char* cmd, char* args, struct message* msg) {
    int ret = 0;
    ret = get_input(input);
    if (ret < 0) {
        printf("Error in input.\n");
        exit(EXIT_FAILURE);
    }

    int input_len = 0;
    int cmd_len = 0;
    int args_len = 0;
    input_len = strlen(input); // fix later

    printf("Input: %s\n", input);
    printf("Input length: %d\n", input_len);

    ret = get_cmd(input, cmd);
    if (ret < 0) {
        printf("Error in parsing command.\n");
        exit(EXIT_FAILURE);
    }

    cmd_len = strlen(cmd);
    printf("Cmd: %s\n", cmd);
    printf("Cmd length: %d\n", cmd_len);

    ret = get_args(input, args);
    if (ret < 0) {
        printf("Error in parsing args.\n");
        printf("No args provided.\n");
    }

    if (ret >= 0) {
        args_len = strlen(args);
        printf("Args: %s\n", args);
        printf("Args length: %d\n", args_len);
    }

    /* Exit client if command quit was specified */
    if (strncmp(cmd, QUIT, QUIT_LEN) == 0) {
        printf("Exiting client.\n");
        return 0;
    }

    /* For now pid is identifier */
    pid_t pid = getpid();
    /* Sending message containing command, client identifier and arguments */

    memset(msg, '\0', sizeof(struct message));
    memcpy(msg->cmd, cmd, cmd_len);
    memcpy(&(msg->id), &pid, sizeof(pid_t));
    memcpy(msg->data, args, args_len);
    return 0;
}

int client_routine(int connection_type, int sk,
                                                struct sockaddr_in* sk_addr,
                                                struct sockaddr_in* sk_broad,
                                                struct sockaddr_in* server_data) {
    int ret = 0;
    socklen_t addrlen = sizeof(*server_data);

    while(1) {

        char input[BUFSIZ];
        char cmd[CMDSIZE];
        char args[MSGSIZE];

        memset(input, 0, BUFSIZ);
        memset(cmd, 0, CMDSIZE);
        memset(args, 0, MSGSIZE);

        struct message msg;


        ret = construct_command(input, cmd, args, &msg);

        printf("Message to be sent:\n");
        printf("ID: %d\n", msg.id);
        printf("Command: %s\n", msg.cmd);
        printf("Data: %s\n", msg.data);
        printf("Sending command\n");

        /* Send broadcast message */
        // if (strncmp(msg.cmd, BROAD, BROAD_LEN) == 0) {
        //     if (connection_type == TCP_CON) {
        //         printf("Unicast only in TCP.\n");
        //         printf("Enter another command");
        //     } else {
        //         ask_broadcast(sk, &msg, &sk_broad, &server_data, &addrlen);
        //     }
        // } else {

        ret = send_message(sk, &msg, sizeof(struct message), sk_addr);
        printf("Bytes sent: %d\n\n\n", ret);

        if (strncmp(msg.cmd, EXIT, EXIT_LEN) == 0) {
            printf("Terminating session.\n");
            exit(EXIT_SUCCESS);
        }
            
            /* Receive reply from server */
            if (connection_type == UDP_CON) {
                ret = recvfrom(sk, &msg, sizeof(struct message), 0, (struct sockaddr*) server_data, &addrlen);
            } //else {
            //     while ((ret = read(sk, &msg, sizeof(struct message))) != sizeof(struct message)) {
            //         printf("Bytes received: %d\n", ret);
            //     }
            // }
            printf("Bytes received: %d\n", ret);
            if (ret < 0) {
                ERROR(errno);
                close(sk);
                return -1;
            }
            
            if (ret != sizeof(struct message)) {
                printf("Error receiving message in client\n");
                close(sk);
                return -1;
            }

            printf("Message received:\n");
            printf("ID: %d\n", msg.id);
            printf("Command: %s\n", msg.cmd);
            printf("Data: %s\n", msg.data);
       // }
    }
    close(sk);
}

//---------------------------------------------------
/* Main server routine, work and accept messages */
int server_routine(int connection_type, int sk, struct sockaddr_in* sk_addr, struct message* memory, \
                     pthread_mutex_t* mutexes, pthread_t* thread_ids, int* id_map) {

    struct message msg = {0};
    int ret = 0;
    struct sockaddr_in client_data = {0};
    int client_sk = 0;
    int* pclient_sk = NULL;

    while (1) {

        memset(&msg, 0, sizeof(struct message));
        /* Get message from client */
        ret = get_msg(sk, sk_addr, &msg, &client_data, &client_sk, pclient_sk, connection_type);
        if (ret < 0) {
            return -1;
        }
        ret = threads_distribute(connection_type, memory,
                                    &msg, thread_ids, id_map, client_sk, pclient_sk);
        if (ret < 0) {
            return -1;
        }
    }
    return 0;
}

//---------------------------------------------------
/* Send message and handle errors, return number of bytes successfully sent */
int send_message(int sk, struct message* msg, int msg_len, struct sockaddr_in* sk_addr) {
    int ret = sendto(sk, msg, msg_len, 0, (struct sockaddr*) sk_addr, sizeof(*sk_addr));
    if (ret < 0) {
        LOG("Error sending to client: %s\n", strerror(errno));
        ERROR(errno);
        return -1;
    }
    return ret;
}

int lookup(int* id_map, int n_ids, pid_t id) {
    if (id_map[id] == 1) {
        return 1;
    }
    return 0;
}

//---------------------------------------------------
/* Initialize bash shell settings on server, return fd of bash
    terminal descriptor */
int shell_init(int* pid) {
    
    int ret = 0;

    int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        LOG("Error opening /dev/ptmx: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    ret = grantpt(fd);
    if (ret < 0) {
        LOG("Error in grantpr: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    ret = unlockpt(fd);  
    if (ret < 0) {
        LOG("Error in unlockpt: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    } 

    char* path = ptsname(fd);
    if (path == NULL) {
        LOG("Error in ptsname: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int resfd = open(path, O_RDWR);
    if (resfd < 0) {
        LOG("Error opening path: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    struct termios term;
    /* Default terminal settings */
    term.c_cflag = 191;
    term.c_oflag = 5;
    term.c_iflag = 17664;
    term.c_lflag = 35387;
    
    /* Remove echo flag */
    LOG("C_lflag before removing ECHO: %d\n", term.c_lflag);
    term.c_lflag = term.c_lflag & (~ECHO);   
    LOG("C_lflag after removing ECHO: %d\n", term.c_lflag);

    ret = tcsetattr(resfd, 0, &term);
    if (ret < 0) {
        LOG("Error setting attr: %s\n", strerror(errno));
        ERROR(errno)    ;
        exit(EXIT_FAILURE);
    }


    *pid = fork();
    if (*pid == 0) {
        dup2(resfd, STDIN_FILENO);
        dup2(resfd, STDOUT_FILENO);
        dup2(resfd, STDERR_FILENO);

        ret = setsid();
        if (ret < 0) {
            LOG("Error in setsid: %s\n", strerror(errno));
            ERROR(errno);
        }

        execl("/bin/bash", "/bin/bash", NULL);
        LOG("Error in exec: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

int shell_cmd(struct message*  msg, char* input) {
    int cmd_len = strlen(msg->cmd);
    int data_len = strlen(msg->data);

    memset(input, '\0', BUFSIZ);
    memcpy(input, msg->cmd, cmd_len);
    input[cmd_len] = ' ';
    memcpy(input + cmd_len + 1, msg->data, data_len);

    LOG("Output from shell_cmd:%s\n", input);
    return 0;
}



//---------------------------------------------------
/* Start shell, continiously read command from client and execute it */
int shell_execute(char* buf, struct message* msg, char* cwd) {

    char input_copy[BUFSIZ];
    int ret = 0;
    int cmd_len;
    char cmd[CMDSIZE];
    char args[MSGSIZE];
    char new_input[BUFSIZ];
    char input[BUFSIZ];

    pid_t pid;
    /* Initialize shell */
    int fd = shell_init(&pid);

    /* Construct input from command field and arguments field in msg */
    ret = shell_cmd(msg, input);
    if (ret < 0) {
        LOG("Error constructing input from msg%s\n", "");
        return -1;
    }

    LOG("Current client directory: %s\n", cwd);
    LOG("Executing shell command on server%s\n", "");
    LOG("Input: %s\n", input);
    /* Copying buffer */
    strcpy(input_copy, input);

    cmd_len = strlen(input);

    /* Get cmd and args */

    /* Get cmd from command field of message */
    ret = get_cmd(input, cmd);
    if (ret < 0) {
        LOG("Error parsing command.%s\n", "");
    }

    ret = get_args(input, args);
    if (ret < 0) {
        LOG("Error parsing args.%s\n", "");
    }

    LOG("Cmd: %s\n", cmd);
    LOG("Args: %s\n", args);

    /* Change directory if cd */
    if (strncmp(cmd, CD, CD_LEN) == 0) {
        LOG("Cwd:%s\n", cwd);
        /* Copy to cwd new directory */
        memcpy((void*) cwd, args, MSGSIZE);
        LOG("Changed to: %s\n", cwd);
    }  


    /* Writing command */
    /* Manually construct ls command with cwd,
        if we do not need to construct manually, simply copy old input to new input */
    /* Copy back to main buffer. If input has not been changed, than everything
        is ok */
    strncpy(new_input, input, BUFSIZ);

    if (strncmp(cmd, LS, LS_LEN) == 0) {
        construct_input(cmd, new_input, cwd);
    }

    cmd_len = strlen(new_input);   
    LOG("Input to shell len: %d\n", cmd_len);
    LOG("Input to shell: %s\n", new_input);                             

    ret = write(fd, new_input, cmd_len);
    if (ret != cmd_len) {
        LOG("Error writing to fd: %s\n", strerror(errno));
        ERROR(errno);
        return -1;
    }

    /* Check whether all data has been written with poll */
    struct pollfd pollfds;
    pollfds.fd = fd;
    pollfds.events = POLLIN;
    int wait_ms = 100;

    
    char output[BUFSIZ];
    int offset = 0;

    while ((ret = poll(&pollfds, 1, wait_ms)) != 0) {
        if (pollfds.revents == POLLIN) {
            ret = read(fd, buf, BUFSIZ);
            if (ret < 0) {
                ERROR(errno);
                return -1;
            }
        }
        LOG("Bytes read: %d\n", ret);
        buf[ret] = '\0';
        LOG("Data read: %s\n", buf);
        if (ret < 0) {
            ERROR(errno);
            return -1;
        }
        
        if (offset + ret  < BUFSIZ) {
            memcpy(output + offset, buf, ret);
            offset += ret;
        } else {    
            /* If buffer is not large enough */
            break;
        }
    }

    /* Copy back to main buffer */
    memcpy(buf, output, BUFSIZ);
    /* Null terminate */
    LOG("Output total size: %d\n", offset);
    buf[offset] = '\0';
    LOG("Full output: %s\n", buf);
    
    /* Terminate bash */
    ret = kill(pid, SIGTERM);
    if (ret < 0) {
        LOG("Error terminating bash: %s\n", strerror(errno));
        ERROR(errno);
        return -1;
    }
    return 0;
}

//---------------------------------------------------
/* Initialize server in daemon mode */
void init_daemon() {

    /* process of initialization of daemon */   
    LOG("Initilization of daemon with logs at %s\n", log_path);

    /* Create a new process */
    pid_t pid = fork();

    if (pid < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        /* Exit in parent */
        exit(EXIT_SUCCESS);
    }

    umask(0);
    pid_t sid = setsid();

    if (sid < 0) {
        LOG("Error setting sid: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* Change directory */
    if (chdir("/") < 0) {
        LOG("Error changing dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* Close all file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid_t daemon_pid = getpid();

    /* Write pid to pid file */
    int fd = open("server.pid", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        LOG("Error opening pid file dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_write = write(fd, &daemon_pid, sizeof(pid_t));
    assert(n_write == sizeof(pid_t));
    close(fd);

    /* Print success message */
    LOG("Daemon log initialized at %s\n", log_path);

}

int get_input(char* input) {
    printf("Enter command:\n");

    char* inp = NULL;

    inp = fgets(input, BUFSIZ, stdin);
    if (inp == NULL) {
        return -1;
    }

    return 0;
}

int get_cmd(char* input, char* cmd) {
    int cmd_len = 0;
    char buf[BUFSIZ];
    memcpy(buf, input, BUFSIZ);
    
    char* sp = strchr(buf, ' ');
    if (sp == NULL) {
        cmd_len = strlen(buf);
        memcpy(cmd, buf, cmd_len);
        return cmd_len;
    } else {
        *sp = '\0';
        cmd_len = strlen(buf);
        /* Copy null character as well */
        memcpy(cmd, buf, cmd_len + 1);
        return cmd_len;
    }

    return 0;
}

/* Args buffer is expected to be at least MSGSIZE */
int get_args(char* input, char* args) {
    int cmd_len = 0;
    char buf[BUFSIZ];
    memcpy(buf, input, BUFSIZ);

    char* sp = strchr(buf, ' ');
    if (sp == NULL) {
        return -1;
    }

    char* end = NULL;
    end = strchr(sp + 1, '\0');
    if (end == NULL) {
        return -1;
    }

    cmd_len = strlen(sp + 1);
    /* Include null terminating character as well */
    /* May need to remove \n at the end */
    memcpy(args, sp + 1, cmd_len + 1);

    return cmd_len;
}

void addr_init(struct sockaddr_in* sk_addr, in_addr_t addr) {
    sk_addr->sin_family = AF_INET;
    sk_addr->sin_port = htons(PORT); /* using here htons for network byte order conversion */
    sk_addr->sin_addr.s_addr = htonl(addr);
}

int mutex_init(pthread_mutex_t* mutexes, int* id_map) {
    int ret = 0;
    for (int i = 0; i < MAXCLIENTS; ++i) {
        /* Initialize mutexes */
        ret = pthread_mutex_init(&mutexes[i], NULL);
        if (ret < 0) {
            ERROR(errno);
            LOG("Error initializing mutex: %s\n", strerror(errno));
            return -1;
        }
        id_map[i] = 0;
    }
    return 0;
}


void udp_init(struct sockaddr_in* sk_addr, in_addr_t addr) {
    sk_addr->sin_family = AF_INET;
    sk_addr->sin_port = htons(PORT); /* using here htons for network byte order conversion */
    sk_addr->sin_addr.s_addr = htonl(addr);
}

void any_init(struct sockaddr_in* sk_addr) {
    sk_addr->sin_family = AF_INET;
    sk_addr->sin_port = htons(PORT);
    sk_addr->sin_addr.s_addr = htonl(INADDR_ANY);
}

void broad_init(struct sockaddr_in* sk_addr) {
    sk_addr->sin_family = AF_INET;
    sk_addr->sin_port = htons(PORT); /* using here htons for network byte order conversion */
    sk_addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
}

//---------------------------------------------------
/* Handle clients to corresponding threads */
int threads_distribute(int connection_type, struct message* memory, struct message* msg,
                        pthread_t* thread_ids, int* id_map, int client_sk, int* pclient_sk) {
    /* Access the corresponding location in memory */
    int ret = 0;
    struct message* thread_memory = NULL;

    if (connection_type == UDP_CON) {
        thread_memory = &memory[msg->id];
        memcpy(thread_memory, msg, sizeof(struct message));
    }

    /* Check whether we need a new thread. Create one if needed */
    if (connection_type == UDP_CON) {
        int exists = lookup(id_map, MAXCLIENTS, msg->id);
        if (exists == 0) {
            LOG("New client accepted: %d\n", msg->id);
            id_map[msg->id] = 1;
            /* Handing over this client to a new thread */
            ret = pthread_create(&thread_ids[msg->id], NULL, udp_handle_connection, thread_memory);
            if (ret < 0) {
                LOG("Error creating thread: %s\n", strerror(errno));
                ERROR(errno);
                return -1;
            }
        } else {
            LOG("Old client accepted: %d\n", msg->id);
        }
    } else {
        ret = pthread_create(&thread_ids[client_sk], NULL, tcp_handle_connection, pclient_sk);
        if (ret < 0) {
            LOG("Error creating thread: %s\n", strerror(errno));
            ERROR(errno);
            exit(EXIT_FAILURE);
        }
    }
    /* Transfer data to corresponding client's memory cell */
    if (connection_type == UDP_CON) {
        thread_memory = &memory[msg->id];
        memcpy(thread_memory, msg, sizeof(struct message));
    }

    /* Unlock mutex so client thread could access the memory */
    if (connection_type == UDP_CON) {
        ret = pthread_mutex_unlock(&mutexes[msg->id]);
        if (ret < 0) {
            LOG("Error unlocking mutex.%s\n", "");
            return -1;
        }
    }
    printf("\n\n\n");

    return 0;
}

//---------------------------------------------------
/* Accept message from client and place it in msg buffer */
int get_msg(int sk, struct sockaddr_in* sk_addr, struct message* msg, struct sockaddr_in* client_data,
             int* client_sk, int* pclient_sk, int connection_type) {
    
    int ret = 0;
    socklen_t addrlen;

    /* Receiving message from client */
    addrlen = sizeof(*client_data);

    if (connection_type == UDP_CON) {
        ret = recvfrom(sk, msg, sizeof(struct message), 0, (struct sockaddr*) client_data, &addrlen);
        if (ret < 0) {
            ERROR(errno);
            LOG("Error receiving msg: %s\n", strerror(errno));
            return -1;
        }
    } else {
        /* Accept client connections */
        LOG("Waiting for message to come\n%s", "");

        *client_sk = accept(sk, NULL, NULL);
        if (*client_sk < 0) {
            ERROR(errno);
            return -1;
        }

        pclient_sk = (int*) calloc(1, sizeof(int));
        if (pclient_sk == NULL) {
            LOG("Error allocating memory for client_sk%s\n", "");
            return -1;
        }
        *pclient_sk = *client_sk;

        LOG("Client sk assigned: %d\n", *client_sk);
        return 0;
    }

    /* Copy client address manually, we get here in UDP mode */
    memcpy(&(msg->client_data), client_data, sizeof(struct sockaddr_in));

    LOG("\n\n\nBytes received: %d\n", ret);
    LOG("Message size expected: %ld\n", sizeof(struct message));

    char* addr = inet_ntoa(client_data->sin_addr);
    if (addr == NULL) {
        LOG("ERROR: Client address invalid: %s\n", strerror(errno));
    }

    LOG("Client address: %s\n", addr);
    LOG("Client port: %d\n", htons(client_data->sin_port));
    return 0;
}

void check_thread(pthread_t* thread_ids, struct message* thread_memory, int* id_map, struct message* msg, void* handle_connection) {
    int ret = 0;

    int exists = lookup(id_map, MAXCLIENTS, msg->id);
    if (exists == 0) {
        LOG("New client: %d\n", msg->id);
        id_map[msg->id] = 1;
        /* Handing over this client to a new thread */
        ret = pthread_create(&thread_ids[msg->id], NULL, handle_connection, thread_memory);
        if (ret < 0) {
            LOG("Error creating thread: %s\n", strerror(errno));
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

    } else {
        LOG("Old client: %d\n", msg->id);
    }
}

void terminate_server() {
    LOG("Closing server%s", "");
    exit(EXIT_SUCCESS);
}

void send_broadcast(int sk, struct message* msg, struct sockaddr_in* client_data) {
    int ret = 0;

    LOG("Broadcasting server IP%s\n", "");
    char reply[] = "Reply to client";
    memcpy(msg->data, reply, sizeof(reply));

    ret = sendto(sk, msg, sizeof(struct message), 0,           \
                (struct sockaddr*) client_data, sizeof(struct sockaddr_in));
    if (ret < 0) {
        LOG("Error sending message to client: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }
}

//---------------------------------------------------
/* Reply to client's message */
int reply_to_client(struct message* msg) {
    int ret = 0;
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0) {
        LOG("Error creating sk: %s\n", strerror(errno));
        ERROR(errno);
        return -1;
    }

    LOG("SENDING MESSAGE BACK TO CLIENT%s\n", "");
    /* Print info */
    LOG("ID: %d\n", msg->id);
    LOG("Command: %s\n", msg->cmd);
    LOG("Data: %s\n", msg->data);

    ret = send_message(sk, msg, sizeof(struct message), &(msg->client_data));
    if (ret < 0) {
        LOG("Error sending message: %s\n", strerror(errno));
        ERROR(errno);
        return -1;
    }
    LOG("Bytes sent to client:%d\n", ret);

    close(sk);
    LOG("MESSAGE SENT%s\n", "");

    return 0;
}

//---------------------------------------------------
/* Function handling accepting messages in thread and handing them over to corresponding threads */
void thread_routine(struct message* msg, struct message* memory, char* dir, char* buf) {
    int ret = 0;

    while (1) {
        /* Copy data from memory */
        memcpy(msg, memory, sizeof(struct message));
        /* Lock mutex */
        LOG("Waiting for mutex to be unlocked%s\n", "");
        LOG("Mutex unlocked%s\n", "");

        pthread_mutex_lock(&mutexes[msg->id]);
        memcpy(msg, memory, sizeof(struct message));
        print_info(msg);
        ret = print_client_addr(msg);

        if (ret < 0) {
            LOG("Client address invalid %s\n", "");
        }

        /* Handle client's command */
        ret = handle_message(msg, dir, buf);
        if (ret < 0) {
            exit(EXIT_FAILURE);
        }
        ret = reply_to_client(msg);
        if (ret < 0) {
            exit(EXIT_FAILURE);
        }
    }
}

void ask_broadcast(int sk, struct message* msg, struct sockaddr_in* sk_broad, struct sockaddr_in* server_data, socklen_t* addrlen) {
     /* Buffer for IP address from server */
    int ret = 0;
    char buf[MSGSIZE];

    printf("Sending broadcast message\n");
    ret = send_message(sk, msg, sizeof(struct message), sk_broad);
    printf("Bytes sent: %d\n\n\n", ret);

    if (ret < 0) {
        fprintf(stderr, "Error sending message\n");
        close(sk);
        exit(EXIT_FAILURE);
    }

    ret = recvfrom(sk, buf, MSGSIZE, 0, (struct sockaddr*) server_data, addrlen);
    if (ret < 0) {
        close(sk);
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    printf("Bytes received: %d\n", ret);
    char* addr = inet_ntoa(server_data->sin_addr);
    if (addr == NULL) {
        printf("Server address invalid\n");
    }
    printf("Server address received from broadcast: %s\n", addr);
    printf("Bytes received: %d\n", ret);
}

void send_to_server(int sk, struct message* msg, struct sockaddr_in* sk_addr, struct sockaddr_in* server_data, socklen_t* addrlen) {
    int ret = 0;
    ret = send_message(sk, msg, sizeof(struct message), sk_addr);
    printf("Bytes sent: %d\n\n\n", ret);

    ret = recvfrom(sk, msg, sizeof(struct message), 0, (struct sockaddr*) server_data, addrlen);
    if (ret < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    printf("Bytes received: %d\n", ret);
    printf("Message received:\n");
    printf("ID: %d\n", msg->id);
    printf("Command: %s\n", msg->cmd);
    printf("Data: %s\n", msg->data);
}

void construct_input(char* cmd, char* new_input, char* cwd) {
    if (strncmp(cmd, LS, LS_LEN) == 0) {
        LOG("LS in cwd:%s\n", cwd);
        memcpy(new_input, cmd, LS_LEN);
        int cwd_len = strlen(cwd);
        LOG("CWD len: %d\n", cwd_len);
        new_input[LS_LEN] = ' ';
        memcpy(new_input + LS_LEN + 1 , cwd, cwd_len);
        /* Add newline at the end */
        new_input[LS_LEN + 1 + cwd_len] = '\n';
        new_input[LS_LEN + 3 + cwd_len] = '\0';
        LOG("Command constructed: %s", new_input);
    }
}

void tcp_reply_to_client(int client_sk, struct message* msg) {
    int ret = 0;
    LOG("SENDING MESSAGE BACK TO CLIENT%s\n", "");
    /* Print info */
    LOG("ID: %d\n", msg->id);
    LOG("Command: %s\n", msg->cmd);
    LOG("Data: %s\n", msg->data);

    ret = send(client_sk, msg, sizeof(struct message), 0);
    if (ret < 0) {
        LOG("Error sending message: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }
    LOG("Bytes sent to client: %d\n", ret);
    LOG("MESSAGE SENT%s\n", "");
}