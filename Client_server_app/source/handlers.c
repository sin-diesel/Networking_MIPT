#include "my_server.h"


pthread_mutex_t mutexes[MAXCLIENTS] = {0};
int client_sockets[MAXCLIENTS];

void* tcp_handle_connection(void* memory) {

    struct message msg;
    int ret = 0;
    int client_sk = *((int*) memory);

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
/* This functions is responsible for handling connection within a single thread */
void* udp_handle_connection(void* memory) {

    struct message msg;
    int ret = 0;
    memset(&msg, 0, sizeof(struct message));

    /* Buffer for maintaining data */
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);
    char ack[] = "Message received";
    char none[] = "None";
    char dir[MAXPATH];

    /* Construct default ack message to client */
    memcpy(msg.cmd, none, sizeof(none));
    memcpy(msg.data, ack, sizeof(ack));

    LOG("Entered new thread %s\n", "");

    /* Init client directory*/
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
        ret = print_client_addr(&msg);
        
        if (ret < 0) {
            LOG("Client address invalid %s\n", "");
        }

        /* Handle client's command */
        ret = handle_message(&msg, dir, buf);
        if (ret < 0) {
            LOG("Error handling client's message.%s\n", "");
            exit(EXIT_FAILURE);
        }
        ret = reply_to_client(&msg);
        if (ret < 0) {
            LOG("Error replying to client.%s\n", "");
            exit(EXIT_FAILURE);
        }
    }


    return NULL;
}