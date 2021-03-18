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
        } else if (strcmp(*command, SHELL) == 0) {
            return SHELL_CMD;
        }
        printf(usage);
        return BAD_CMD;
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
        } else if (strcmp(*command, SHELL) == 0) {
            return SHELL_CMD;
        }
    }

    /* No match */
    printf(usage);

    return BAD_CMD;
}

int send_message(int sk, struct message* msg, int msg_len, struct sockaddr_in* sk_addr) {
    int ret = sendto(sk, msg, msg_len, 0, (struct sockaddr*) sk_addr, sizeof(*sk_addr));
    if (ret < 0) {
        LOG("Error sending to client: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }
    return ret;
}

int lookup(int* id_map, int n_ids, pid_t id) {
    if (id_map[id] == 1) {
        return 1;
    }
    return 0;
}

int init_shell(int* pid) {
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



void start_shell(char* buf, char* input, char* cwd) {
    LOG("Starting shell on server%s\n", "");
    int ret = 0;

    pid_t pid;
    int fd = init_shell(&pid);

    char input_copy[BUFSIZ];
    /* Copying buffer */
    strcpy(input_copy, input);

    /* Add \n to the end of command assuming we have enough space */
    int cmd_len = strlen(input);

    /* Get cmd and args */
    char cmd[CMDSIZE];
    char args[MSGSIZE];

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
    char new_input[BUFSIZ];
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
        exit(EXIT_FAILURE);
    }

    /* Check whether all data has been written with poll */
    struct pollfd pollfds;
    pollfds.fd = fd;
    pollfds.events = POLLIN;
    int wait_ms = 1000;

    
    char output[BUFSIZ];
    int offset = 0;

    while ((ret = poll(&pollfds, 1, wait_ms)) != 0) {

        if (pollfds.revents == POLLIN) {
            ret = read(fd, buf, BUFSIZ);
            if (ret < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }
        }

        LOG("Bytes read: %d\n", ret);
        buf[ret] = '\0';
        LOG("Data read: %s\n", buf);
        if (ret < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
}

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

void mutex_init(pthread_mutex_t* mutexes, int* id_map) {
    int ret = 0;
    for (int i = 0; i < MAXCLIENTS; ++i) {
        /* Initialize mutexes */
        ret = pthread_mutex_init(&mutexes[i], NULL);
        if (ret < 0) {
            ERROR(errno);
            LOG("Error initializing mutex: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        id_map[i] = 0;
    }
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

void udp_get_msg(int sk, struct sockaddr_in* sk_addr, struct message* msg, struct sockaddr_in* client_data, int connection_type) {
    
    int ret = 0;
    socklen_t addrlen;

    /* Receiving message from client */
    addrlen = sizeof(client_data);

    if (connection_type == UDP_CON) {
        ret = recvfrom(sk, msg, sizeof(struct message), 0, (struct sockaddr*) client_data, &addrlen);
    } else {
        ret = read(sk, msg, sizeof(struct message));
    }
    if (ret < 0) {
        ERROR(errno);
        LOG("Error receiving msg: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Copy client address manually */
    memcpy(&(msg->client_data), client_data, sizeof(struct sockaddr_in));

    LOG("\n\n\nBytes received: %d\n", ret);
    LOG("Message size expected: %ld\n", sizeof(struct message));

    char* addr = inet_ntoa(client_data->sin_addr);
    if (addr == NULL) {
        LOG("Client address invalid: %s\n", strerror(errno));
    }

    LOG("Client address: %s\n", addr);
    LOG("Client port: %d\n", htons(client_data->sin_port));
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

void terminate_server(int sk) {
    LOG("Closing server%s", "");
    close(sk);
    unlink(PATH);
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

void reply_to_client(struct message* msg) {
    int ret = 0;
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0) {
        LOG("Error creating sk: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
    LOG("Bytes sent to client:%d\n", ret);

    close(sk);
    LOG("MESSAGE SENT%s\n", "");

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
