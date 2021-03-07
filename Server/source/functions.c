#include "my_server.h"


/* TODO
1) Refactor daemon, upgrade logger macros DONE
2) Replace printfs with writing to log file DONE
3) Add client input 
*/

/* Print message info */
void print_info(struct message* msg) {
    printf("ID: %d\n", msg->id);
    printf("Command: %s\n", msg->cmd);
    printf("Data: %s\n", msg->data);
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


void start_shell(char* buf, char* cmd) {
    LOG("Starting shell on server%s\n", "");
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
    term.c_lflag = 0;

    ret = tcsetattr(resfd, 0, &term);
    if (ret < 0) {
        LOG("Error setting attr: %s\n", strerror(errno));
        ERROR(errno)    ;
        exit(EXIT_FAILURE);
    }


    pid_t pid = fork();
    if (pid == 0) {
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

    /* Add \n to the end of command assuming we have enough space */
    int cmd_len = strlen(cmd);
    cmd[cmd_len] = '\n';
    cmd[cmd_len + 1] = '\0';
    cmd_len = cmd_len + 1;

    LOG("Command to be executed:%s", cmd);   
    /* Writing command */
    ret = write(fd, cmd, cmd_len);
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

    /* Here is a problem: we always receive command promt at the end of reading
        therefore, stop at the second output */
    
    int cmd_num = 1;
    char real_output[BUFSIZ];

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

        if (cmd_num == 2) {
            memcpy(real_output, buf, BUFSIZ);
        }
        ++cmd_num;
    }

    /* Copy back to main buffer */
    memcpy(buf, real_output, BUFSIZ);
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

// void enter_command() {
//     int ret = 0;

//     printf("Enter command:\n");

//     char cmd[CMDSIZE];

//     fgets
// }

