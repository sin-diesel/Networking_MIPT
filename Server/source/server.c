/* This is server program, which creates a server and listens for
    messages from clients */


#include "my_server.h"

/* Mutexes for threads */
pthread_mutex_t mutexes[MAXCLIENTS];

void* handle_connection(void* memory) {
    int ret = 0;

    LOG("Entered new thread %s\n", "");

    char dir[MAXPATH];
    char* dirp = getcwd(dir, MAXPATH);
    if (dirp == NULL) {
        LOG("Error changing directory: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    LOG("Current thread directory: %s\n", dir);

    while (1) {
        struct message msg;

        /* Buffer for maintaining data */
        char buf[BUFSIZ];
        buf[BUFSIZ - 1] = '\0';

        /* Copy data from memory */
        memcpy(&msg, memory, sizeof(struct message));
        pthread_mutex_lock(&mutexes[msg.id]);
        memcpy(&msg, memory, sizeof(struct message));

        /* Print info */
        LOG("Message received: %s\n", "");
        LOG("ID: %d\n", msg.id);
        LOG("Command: %s\n", msg.cmd);
        LOG("Data: %s\n", msg.data);

        char* addr = inet_ntoa(msg.client_data.sin_addr);
        if (addr == NULL) {
            LOG("Client address invalid %s\n", "");
        }

        LOG("Client address: %s\n", addr);
        LOG("Client port: %d\n", msg.client_data.sin_port);

        /* Handle client's command */   
        if (strncmp(msg.cmd, LS, LS_LEN) == 0) {

            /* Redirect output to pipe and then read it */
            int ls_pipe[2];
            ret = pipe(ls_pipe);

            if (ret < 0) {
                LOG("Error creating pipe: %s\n", strerror(errno));
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            int status = 0;

            int pid = fork();
            if (pid < 0) {
                LOG("Error forking: %s\n", strerror(errno));
                ERROR(errno);
                exit(EXIT_FAILURE);
            }
            /* Execute ls command and save output, then send it to client */
            if (pid == 0) {
                char* arg[3];
                arg[0] = "ls";
                arg[1] = dir;
                arg[2] = NULL;

                /* Redirect to pipe */
                ret = dup2(ls_pipe[1], STDOUT_FILENO);
                close(ls_pipe[0]); // not reading

                if (ret < 0) {
                    LOG("Error dupping: %s\n", strerror(errno));
                    ERROR(errno);
                    exit(EXIT_FAILURE);
                }   

                execvp(arg[0],arg);
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            /* Wait until read is complete */
            wait(&status);

            /* Read data to buffer */
            ret = read(ls_pipe[0], buf, MSGSIZE);
            if (ret < 0) {
                LOG("Error reading from pipe: %s\n", strerror(errno));
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            LOG("Bytes read from pipe: %d\n", ret);
            LOG("LS result: %s\n", buf);

            /* Copy data from buf to msg */
            memcpy(msg.data, buf, MSGSIZE);
            close(ls_pipe[0]);
            close(ls_pipe[1]);

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

        int sk = socket(AF_INET, SOCK_DGRAM, 0);
        if (sk < 0) {
            LOG("Error creating sk: %s\n", strerror(errno));
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

        LOG("SENDING MESSAGE BACK TO CLIENT%s\n", "");
        /* Print info */
        LOG("Message received: %s\n", "");
        LOG("ID: %d\n", msg.id);
        LOG("Command: %s\n", msg.cmd);
        LOG("Data: %s\n", msg.data);

        ret = send_message(sk, &msg, sizeof(struct message), &msg.client_data);
        if (ret < 0) {
            LOG("Error sending message: %s\n", strerror(errno));
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

        close(sk);
        LOG("MESSAGE SENT%s\n", "");
    }

    return NULL;
}


int main() {

    /* Creating and initializing socket */
    int sk = 0;
    int ret = 0;

    /* Run server as daemon */
    init_daemon();

    #ifdef UDP
    sk = socket(AF_INET, SOCK_DGRAM, 0);
    #endif
    
    #ifdef INET
    sk = socket(AF_INET, SOCK_STREAM, 0);
    #endif

    #ifdef LOCAL
    sk = socket(AF_UNIX, SOCK_STREAM, 0);
    #endif

    if (sk < 0) {
        ERROR(errno);
        LOG("Error opening socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    /* init socket address and family */
    #ifdef INET

    struct sockaddr_in sk_addr = {0};
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(23456);
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // fix later

    #endif

    #ifdef LOCAL

    struct sockaddr_un sk_addr = {0};
    sk_addr.sun_family = AF_UNIX;
    strncpy(sk_addr.sun_path, PATH, sizeof(sk_addr.sun_path) - 1);

    #endif

    #ifdef COMM

    struct in_addr addr;
    res = inet_pton(AF_INET, FRIENDIP, &addr);
    if (res != 1) {
        printf("IP address is invalid\n");
    }
    sk_addr.sin_addr.s_addr = addr.s_addr;

    #endif

    #ifdef UDP

    struct sockaddr_in sk_addr;
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(PORT);
    sk_addr.sin_addr.s_addr = htonl(INADDR_ANY); // fix later CAREFUL

    /* Set up fixed amount of thread identifiers, each thread identifier associates
    with a particular client */
    pthread_t thread_ids[MAXCLIENTS];

    /* Basically bitmap */
    int id_map[MAXCLIENTS];
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


    /* memory for sharing between threads */
    struct message* memory = (struct message*) calloc(MAXCLIENTS, sizeof(struct message));
    if (memory == NULL) {
        LOG("Error allocating memory for clients: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    #endif

    ret = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

    if (ret < 0) {
        LOG("Error binding: %s\n", strerror(errno));
        ERROR(errno);
        close(sk);
        exit(EXIT_FAILURE);
    }

    /* activate socket and listen for transmissions */

    #ifdef TCP

    res = listen(sk, MAX_QUEUE);

    if (res < 0) {
        ERROR(errno);
        close(sk);
        return -1;
    }

    #endif

    /* Listen for clients */
    while (1) {
        /* client connects to socket */

        #ifdef UDP

        struct message msg;
        memset(&msg, '\0', sizeof(struct message));

        /* Receiving message from client */
        struct sockaddr_in client_data;
        socklen_t addrlen = sizeof(client_data);

        ret = recvfrom(sk, &msg, sizeof(struct message), 0, (struct sockaddr*) &client_data, &addrlen);
        if (ret < 0) {
            ERROR(errno);
            LOG("Error receiving msg: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        /* Copy client address manually */
        memcpy(&msg.client_data, &client_data, sizeof(struct sockaddr_in));


        LOG("\n\n\nBytes received: %d\n", ret);
        LOG("Message size expected: %ld\n", sizeof(struct message));

        char* addr = inet_ntoa(client_data.sin_addr);
        if (addr == NULL) {
            LOG("Client address invalid: %s\n", strerror(errno));
        }

        LOG("Client address: %s\n", addr);
        LOG("Client port: %d\n", htons(client_data.sin_port));

        /* Check whether we need a new thread */
        struct message* thread_memory = &memory[msg.id];
        memcpy(thread_memory, &msg, sizeof(struct message));

        int exists = lookup(id_map, MAXCLIENTS, msg.id);
        if (exists == 0) {
            LOG("New client: %d\n", msg.id);
            id_map[msg.id] = 1;
            /* Handing over this client to a new thread */
            ret = pthread_create(&thread_ids[msg.id], NULL, handle_connection, thread_memory);
            if (ret < 0) {
                LOG("Error creating thread: %s\n", strerror(errno));
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

        } else {
            LOG("Old client: %d\n", msg.id);
        }

        /* Decide which message was sent, handle exit and broadcast */
        if (strncmp(msg.cmd, EXIT, EXIT_LEN) == 0) {
            /* Closing server */
                LOG("Closing server%s", "");
                close(sk);
                unlink(PATH);
                exit(EXIT_SUCCESS);
        } else if (strncmp(msg.cmd, BROAD, BROAD_LEN) == 0) {
            LOG("Broadcasting server IP%s\n", "");
            char reply[] = "Reply to client";
            memcpy(msg.data, reply, sizeof(reply));

            ret = sendto(sk, reply, sizeof(reply), 0,           \
                             (struct sockaddr*) &client_data, sizeof(client_data));
            if (ret < 0) {
                LOG("Error sending message to client: %s\n", strerror(errno));
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

        }
        

        thread_memory = &memory[msg.id];
        memcpy(thread_memory, &msg, sizeof(struct message));
        pthread_mutex_unlock(&mutexes[msg.id]);
        
        printf("\n\n\n");
        #endif
        

            


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