/* This is server program, which creates a server and listens for
    messages from clients */

#include "my_server.h"

/* Mutexes for threads */
/* Connection type, TCP or UDP */
static int connection_type = NONE;

//---------------------------------------------------
int main(int argc, char** argv) {

    /* Choose connection type */;

    /* UDP connection by default */
    connection_type = UDP_CON;
    int ret = 0;
    int sk = 0;
    struct sockaddr_in sk_addr;
    /* Set up fixed amount of thread identifiers, each thread identifier associates
    with a particular client */
    pthread_t thread_ids[MAXCLIENTS];
    struct message* memory = NULL;

    /* Basically bitmap */
    int id_map[MAXCLIENTS];

    
    ret = check_input(argc, argv, &connection_type);
    if (ret < 0) {
        printf("Incorrect option passed.\n");
        exit(EXIT_FAILURE);
    }

    /* Prepare server for main routine */
    ret =  server_init(connection_type, &sk, &sk_addr, id_map, &memory, mutexes);
    if (ret < 0) {
        exit(EXIT_FAILURE);
    }

    ret = server_routine(connection_type, sk, &sk_addr, memory, mutexes, thread_ids, id_map);

    return 0;
}