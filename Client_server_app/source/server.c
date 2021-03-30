/* This is server program, which creates a server and listens for
    messages from clients */

#include "my_server.h"

/* Mutexes for threads */
/* Connection type, TCP or UDP */
static int connection_type = NONE;
//pthread_mutex_t* p_mutexes = mutexes;
/* Pointers to functions so we could access them in another c file */
//void* (*udp_handler)(void*) = &udp_handle_connection;


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
    LOG("Input successfull%s\n", "");

    /* Prepare server for main routine */
    ret =  server_init(connection_type, &sk, &sk_addr, id_map, &memory, mutexes);
    if (ret < 0) {
        exit(EXIT_FAILURE);
    }
    //LOG("Memory: %p\n", memory);
    LOG("Init successfull%s\n", "");

    ret = server_routine(connection_type, sk, &sk_addr, memory, mutexes, thread_ids, id_map);
    /* Accept messages */
    // while (1) {

    //     memset(&msg, '\0', sizeof(struct message));
    //     /* Get message from client */
    //     ret = get_msg(sk, &sk_addr, &msg, &client_data, &client_sk, pclient_sk, connection_type);
    //     if (ret < 0) {
    //         exit(EXIT_FAILURE);
    //     }

    //     ret = threads_distribute(connection_type, thread_memory, memory,
    //                                 &msg, thread_ids, id_map, client_sk, pclient_sk);
    //     if (ret < 0) {
    //         exit(EXIT_FAILURE);
    //     }
    //     // примечание: если файл (или сокет) удалили с файловой системы, им еще могут пользоваться программы которые не закрыли его до закрытия
    // }

    return 0;
}