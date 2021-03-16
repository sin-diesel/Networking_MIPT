/* This is server program, which creates a server and listens for
    messages from clients */

#include "my_server.h"

/* Mutexes for threads */
pthread_mutex_t mutexes[MAXCLIENTS];
int connection_type = UDP_CON;

//---------------------------------------------------
void* handle_connection(void* memory) {

    struct message msg;
    memset(&msg, 0, sizeof(struct message));

    /* Buffer for maintaining data */
    char buf[BUFSIZ];
    buf[BUFSIZ - 1] = '\0';
    memset(buf, 0, BUFSIZ);
    char ack[] = "Message received";
    char none[] = "None";

    /* Construct default ack message to client */
    memcpy(msg.cmd, none, sizeof(none));
    memcpy(msg.data, ack, sizeof(ack));

    LOG("Entered new thread %s\n", "");

    /* Init client directory*/
    char dir[MAXPATH];
    char* dirp = getcwd(dir, MAXPATH);
    if (dirp == NULL) {
        LOG("Error changing directory: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }
    LOG("Current thread directory: %s\n", dir);

    while (1) {

        /* Copy data from memory */
        memcpy(&msg, memory, sizeof(struct message));
        /* Lock mutex */
        pthread_mutex_lock(&mutexes[msg.id]);
        memcpy(&msg, memory, sizeof(struct message));

        print_info(&msg);

        char* addr = inet_ntoa(msg.client_data.sin_addr);
        if (addr == NULL) {
            LOG("Client address invalid %s\n", "");
        }

        LOG("Client address: %s\n", addr);
        LOG("Client port: %d\n", msg.client_data.sin_port);

        /* Handle client's command */   
        if (strncmp(msg.cmd, LS, LS_LEN) == 0) {

        //     /* Redirect output to pipe and then read it */
        //     int ls_pipe[2];
        //     ret = pipe(ls_pipe);

        //     if (ret < 0) {
        //         LOG("Error creating pipe: %s\n", strerror(errno));
        //         ERROR(errno);
        //         exit(EXIT_FAILURE);
        //     }

        //     int status = 0;

        //     int pid = fork();
        //     if (pid < 0) {
        //         LOG("Error forking: %s\n", strerror(errno));
        //         ERROR(errno);
        //         exit(EXIT_FAILURE);
        //     }
        //     /* Execute ls command and save output, then send it to client */
        //     if (pid == 0) {
        //         char* arg[3];
        //         arg[0] = "ls";
        //         arg[1] = dir;
        //         arg[2] = NULL;

        //         /* Redirect to pipe */
        //         ret = dup2(ls_pipe[1], STDOUT_FILENO);
        //         close(ls_pipe[0]); // not reading

        //         if (ret < 0) {
        //             LOG("Error dupping: %s\n", strerror(errno));
        //             ERROR(errno);
        //             exit(EXIT_FAILURE);
        //         }   

        //         execvp(arg[0],arg);
        //         ERROR(errno);
        //         exit(EXIT_FAILURE);
        //     }

        //     /* Wait until read is complete */
        //     wait(&status);

        //     /* Read data to buffer */
        //     ret = read(ls_pipe[0], buf, MSGSIZE);
        //     if (ret < 0) {
        //         LOG("Error reading from pipe: %s\n", strerror(errno));
        //         ERROR(errno);
        //         exit(EXIT_FAILURE);
        //     }

        //     LOG("Bytes read from pipe: %d\n", ret);
        //     LOG("LS result: %s\n", buf);

        //     /* Copy data from buf to msg */
        //     memcpy(msg.data, buf, MSGSIZE);
        //     close(ls_pipe[0]);
        //     close(ls_pipe[1]);

        } else if (strncmp(msg.cmd, CD, CD_LEN) == 0) {

            LOG("Cwd: %s\n", dir);
            memcpy((void*) dir, &msg.data, MSGSIZE);
            LOG("Changing cwd to %s\n", msg.data);

        } else if (strncmp(msg.cmd, SHELL, SHELL_LEN) == 0) {

            LOG("Message data to be executed in shell:%s\n", msg.data);
            /* Send cwd so shell knows where to execute command */
            start_shell(buf, msg.data, dir);
            /* Copy data from shell return buf to msg */
            memcpy(msg.data, buf, MSGSIZE);
            LOG("Data ready to be sent to client: %s\n", msg.data);
        }   
        if (connection_type == UDP_CON) {
            LOG("UDP reply.%s\n", "");
            reply_to_client(&msg);
        } else {
            LOG("TCP reply.%s\n", "");
            tcp_reply_to_client(msg.client_sk, &msg);
        }
    }


    return NULL;
}


//---------------------------------------------------
int main(int argc, char** argv) {

    /* Choose connection type */;

    if (argc == 2) {
        if (strcmp(argv[1], "--udp") == 0) {
            printf("UDP connection set\n");
            connection_type = UDP_CON;
        } else if (strcmp(argv[1], "--tcp") == 0) {
            printf("TCP connection set\n");
            connection_type = TCP_CON;
        }
    }

    /* Creating and initializing socket */
    int sk = 0;
    int ret = 0;
    struct sockaddr_in sk_addr;

    /* Run server as daemon */
    init_daemon();

    if (connection_type == UDP_CON) {
        sk = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        sk = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (sk < 0) {
        ERROR(errno);
        LOG("Error opening socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Init socket address and family */
    addr_init(&sk_addr, INADDR_ANY);

    ret = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
    if (ret < 0) {
        LOG("Error binding: %s\n", strerror(errno));
        ERROR(errno);
        close(sk);
        exit(EXIT_FAILURE);
    }
    
    /* Set up fixed amount of thread identifiers, each thread identifier associates
    with a particular client */
    pthread_t thread_ids[MAXCLIENTS];

    /* Basically bitmap */
    int id_map[MAXCLIENTS];

    mutex_init(mutexes, id_map);

    /* Allocating memory for sharing between threads */
    struct message* memory = (struct message*) calloc(MAXCLIENTS, sizeof(struct message));
    if (memory == NULL) {
        LOG("Error allocating memory for clients: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* First get ready for listening */
    if (connection_type == TCP_CON) {
        ret = listen(sk, BACKLOG);
        if (ret < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }
    }

    /* Accept messages */
    while (1) {

        struct message msg;
        memset(&msg, '\0', sizeof(struct message));
        struct sockaddr_in client_data;
        struct message* thread_memory = NULL;

        /* Get message from client */
        if (connection_type == UDP_CON) {
            udp_get_msg(sk, &sk_addr, &msg, &client_data, UDP_CON);
        } else {
            /* Accept client connections */
            int client_sk = 0;
            client_sk = accept(sk, NULL, NULL);
            if (client_sk < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            //msg.client_sk = client_sk;
            udp_get_msg(client_sk, &sk_addr, &msg, &client_data, TCP_CON);
            msg.client_sk = client_sk;
            LOG("Client sk assigned: %d\n", client_sk);

            //tcp_get_msg(client_sk, &sk_addr, &msg, &client_data);

        }

        /* Decide which message was sent, handle exit and broadcast */
        if (strncmp(msg.cmd, EXIT, EXIT_LEN) == 0) {
            /* Closing server */
            terminate_server(sk);
        }

        /* Access the corresponding location in memory */
        thread_memory = &memory[msg.id];
        memcpy(thread_memory, &msg, sizeof(struct message));

        /* Check whether we need a new thread. Create one if needed */
        check_thread(thread_ids, thread_memory, id_map, &msg, handle_connection);
        
        /* Transfer data to corresponding client's memory cell */
        thread_memory = &memory[msg.id];
        memcpy(thread_memory, &msg, sizeof(struct message));

        /* Unlock mutex so client thread could access the memory */
        pthread_mutex_unlock(&mutexes[msg.id]);
        printf("\n\n\n");

        #ifdef TCP

        int client_sk = 0;

        client_sk = accept(sk, NULL, NULL);

        if (client_sk < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }
            

        /* read message that was accepted */
        /* buf for accepting command */
        char buf[BUFSZ] = {};
        /* message buffer */
        char msg[BUFSZ] = {};
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
        printf("Message from client: %s\n", msg);

        /* then change sockaddr if needed */
        #ifdef INET

            struct in_addr addr;
            res = inet_pton(AF_INET, msg, &addr);
            if (res != 1) {
                printf("IP address is invalid\n");
            }
            sk_addr.sin_addr.s_addr = addr.s_addr;

        #endif

        } else if (strcmp(buf, EXIT) == 0) {
            close(client_sk);
            unlink(PATH);
            exit(EXIT_SUCCESS);
        } else {
            printf("Command from client not recognized\n");
        }

            /* finish communication */
        close(client_sk);

        #endif
        // примечание: если файл (или сокет) удалили с файловой системы, им еще могут пользоваться программы которые не закрыли его до закрытия
    }

    return 0;
}