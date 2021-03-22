/* This is server program, which creates a server and listens for
    messages from clients */

#include "my_server.h"

/* Mutexes for threads */
pthread_mutex_t mutexes[MAXCLIENTS];
int client_sockets[MAXCLIENTS];
int connection_type = -1;

void* tcp_handle_connection(void* client_memory) {

    struct message msg;
    int ret = 0;
    int client_sk = *((int*) client_memory);

    LOG("Entered new thread (TCP) %s\n", "");

    /* Init client directory*/
    char dir[MAXPATH];
    char buf[BUFSIZ];
    char* dirp = getcwd(dir, MAXPATH);
    if (dirp == NULL) {
        LOG("Error changing directory: %s\n", strerror(errno));
        ERROR(errno);
        exit(EXIT_FAILURE);
    }
    LOG("Current thread directory: %s\n", dir);

    while (1) {
            printf("Reading from client_sk %d\n", client_sk);
            ret = read(client_sk, &msg, sizeof(struct message));
            printf("Message read.\n");
            if (ret < 0) {
                LOG("Error reading from client socket %d\n", client_sk);
                ERROR(errno);
                exit(EXIT_FAILURE);
            }
            /* Process message */
            print_info(&msg);

            char* addr = inet_ntoa(msg.client_data.sin_addr);
            if (addr == NULL) {
                LOG("Client address invalid %s\n", "");
            }
            LOG("Client address: %s\n", addr);
            LOG("Client port: %d\n", msg.client_data.sin_port);

            /* Handle client's command */   
            if (strncmp(msg.cmd, LS, LS_LEN) == 0) {

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
            LOG("TCP reply.%s\n", "");
            tcp_reply_to_client(client_sk, &msg);
        }

}

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
        LOG("Waiting for mutex to be unlocked%s\n", "");
        LOG("Mutex unlocked%s\n", "");
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
        if (strncmp(msg.cmd, CD, CD_LEN) == 0) {

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
        LOG("UDP reply.%s\n", "");
        reply_to_client(&msg);
    }


    return NULL;
}


//---------------------------------------------------
int main(int argc, char** argv) {

    /* Choose connection type */;

    /* UDP connection by default */
    connection_type = UDP_CON;
    
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
    // struct in_addr addr;
    // ret = inet_pton(AF_INET, "192.168.1.11", &addr);
    // if (ret != 1) {
    //     printf("IP address is invalid\n");
    // }
    // sk_addr.sin_family = AF_INET;
    // sk_addr.sin_port = htons(PORT); /* using here htons for network byte order conversion */
    // sk_addr.sin_addr.s_addr = addr.s_addr;

    ret = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
    if (ret < 0) {
        LOG("Error binding: %s\n", strerror(errno));
        ERROR(errno);
        close(sk);
        //exit(EXIT_FAILURE);
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
        int* pclient_sk = NULL;
        int client_sk;

        /* Get message from client */
        if (connection_type == UDP_CON) {
            udp_get_msg(sk, &sk_addr, &msg, &client_data, UDP_CON);
        } else {
            /* Accept client connections */
            LOG("Waiting for message to come\n%s", "");

            client_sk = accept(sk, NULL, NULL);
            if (client_sk < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

            pclient_sk = (int*) calloc(1, sizeof(int));
            if (pclient_sk == NULL) {
                LOG("Error allocating memory for client_sk%s\n", "");
            }
            *pclient_sk = client_sk;

            LOG("Client sk assigned: %d\n", client_sk);

        }

        /* Decide which message was sent, handle exit and broadcast */
        if (strncmp(msg.cmd, EXIT, EXIT_LEN) == 0) {
            /* Closing server */
            terminate_server(sk);
        }

        /* Access the corresponding location in memory */
        if (connection_type == UDP_CON) {
            thread_memory = &memory[msg.id];
            memcpy(thread_memory, &msg, sizeof(struct message));
        }

        /* Check whether we need a new thread. Create one if needed */
        if (connection_type == UDP_CON) {
            check_thread(thread_ids, thread_memory, id_map, &msg, handle_connection);
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
            thread_memory = &memory[msg.id];
            memcpy(thread_memory, &msg, sizeof(struct message));
        }

        /* Unlock mutex so client thread could access the memory */
        if (connection_type == UDP_CON) {
            pthread_mutex_unlock(&mutexes[msg.id]);
        }
        printf("\n\n\n");
        // примечание: если файл (или сокет) удалили с файловой системы, им еще могут пользоваться программы которые не закрыли его до закрытия
    }

    return 0;
}