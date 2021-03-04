#include "my_server.h"


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
        } else {
            printf(usage);
            return BAD_CMD;
        }
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
        }
    }

    /* No match */
    printf(usage);

    return BAD_CMD;
}

int send_message(int sk, struct message* msg, int msg_len, struct sockaddr_in* sk_addr) {
    int ret = sendto(sk, msg, msg_len, 0, (struct sockaddr*) sk_addr, sizeof(*sk_addr));
    if (ret < 0) {
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

void* handle_connection(void* client_pipe) {
    int ret = 0;

    printf("Entered new thread\n");

    char dir[MAX_PATH];
    char* dirp = getcwd(dir, MAX_PATH);
    if (dirp == NULL) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    printf("Current thread directory:%s\n", dir);

    int sk = socket(AF_INET, SOCK_DGRAM, 0);
        if (sk < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

    while (1) {
        /* Read data from pipe and determine what to do with client */
        struct message msg;
        memset(&msg, '\0', sizeof(struct message));

        /* read message data */
        ret = read(*((int*) client_pipe), &msg, sizeof(struct message));
        printf("Bytes read from pipe: %d\n", ret);
        if (ret != sizeof(struct message)) {
            printf("Error reading from pipe\n");
            if (ret < 0) {
                ERROR(errno);
            }
            exit(EXIT_FAILURE);
        }

        /* Print info */
        printf("Message received:\n");
        printf("ID: %d\n", msg.id);
        printf("Command: %s\n", msg.cmd);
        printf("Data: %s\n", msg.data);

        char* addr = inet_ntoa(msg.client_data.sin_addr);
        if (addr == NULL) {
            printf("Client address invalid\n");
        }

        printf("Client address: %s\n", addr);

        /* Handle client's command */   
        if (strncmp(msg.cmd, LS, LS_LEN) == 0) {
            /* Printing current directory */
            printf("Executing LS command from %s directory\n", dir);

            /* Redirect output to pipe and then read it */
            int ls_pipe[2];
            //close(ls_pipe[1]); // not writing
            ret = pipe(ls_pipe);
            if (ret < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }


            int pid = fork();
            if (pid < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                char cmd[] = "ls";
                char* arg[3];
                arg[0] = "ls";
                arg[1] = dir;
                arg[2] = NULL;
                ret = dup2(ls_pipe[1], STDOUT_FILENO);
                close(ls_pipe[0]); // not reading
                //close(ls_pipe[1]);
                if (ret < 0) {
                    ERROR(errno);
                    exit(EXIT_FAILURE);
                }   
                execvp(arg[0],arg); // add current directory later

                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            /* read from pipe to buf */
            char buf[BUFSIZ];
            buf[BUFSIZ - 1] = '\0';
            ret = read(ls_pipe[0], buf, MSGSIZE);
            if (ret < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            printf("Bytes read from pipe: %d\n", ret);

            printf("LS result: %s\n", buf);

            memcpy(&(msg.data), buf, MSGSIZE);

            close(ls_pipe[0]);
            close(ls_pipe[1]);
        } else if (strncmp(msg.cmd, CD, CD_LEN) == 0) {
            memcpy((void*) dir, &msg.data, MSGSIZE);
            printf("Changing cwd to %s\n", msg.data);
        }

        /* Here we will simply use pipe to transfer data */
        /* Or maybe use ports? */

        ret = send_message(sk, &msg, sizeof(struct message), &msg.client_data);
        if (ret < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

    }

    close(sk);

    return NULL;
}

