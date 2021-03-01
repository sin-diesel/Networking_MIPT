
#include "my_server.h"


int main(int argc, char** argv) {

    char* command = NULL;
    char* arg = NULL;
    int which_cmd = check_input(argc, argv, &command, &arg);
    if (which_cmd == BAD_CMD || command == NULL) {
        fprintf(stderr, "Error in command recognition\n");
        exit(EXIT_FAILURE);
    }

    int sk = 0;
    int ret = 0;

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
        return -1;
    }

    /* init socket address and family */
    #ifdef TCP

    #ifdef INET

    struct sockaddr_in sk_addr = {0};
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(23456); /* using here htons for network byte order conversion */
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* same goes for ip */

    #endif

    #ifdef COMM

    struct in_addr addr;
    res = inet_pton(AF_INET, FRIENDIP, &addr);
    if (res != 1) {
        printf("IP address is invalid\n");
    }
    sk_addr.sin_addr.s_addr = addr.s_addr;

    #endif


    #ifdef LOCAL

    struct sockaddr_un sk_addr = {0};
    sk_addr.sun_family = AF_UNIX;
    strncpy(sk_addr.sun_path, PATH, sizeof(sk_addr.sun_path) - 1);

    #endif

    // bind - для тех сокетов, который появляются в системе, для остальных не надо 

    res = connect(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
    if (res < 0) {
        ERROR(errno);
        return -1;
    }

    #endif

    #ifdef UDP
        
    struct sockaddr_in sk_addr;
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(PORT); /* using here htons for network byte order conversion */
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    /* Binding to address */
    //ret = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));
    // if (ret < 0) {
    //     ERROR(errno);
    //     exit(EXIT_FAILURE);
    // }


    /* Send command to a server */
    int cmd_len = 0;
    int arg_len = 0;

    cmd_len = strlen(command); // fix later
    if (arg != NULL) {
        arg_len = strlen(arg);
    }

    printf("Command length: %d\n", cmd_len);

    printf("Sending command\n");
    if (which_cmd == EXIT_CMD) {
        ret = send_message(sk, command, cmd_len, &sk_addr);
        printf("Bytes sent: %d\n\n\n", ret);
    } else if (which_cmd == PRINT_CMD) {

        ret = send_message(sk, command, cmd_len, &sk_addr);
        printf("Sending message\n");
        ret = send_message(sk, arg, arg_len, &sk_addr);
        printf("Bytes sent: %d\n\n\n", ret);


    } else if (which_cmd == LS_CMD) {
        ret = send_message(sk, command, cmd_len, &sk_addr);
        printf("Bytes sent: %d\n\n\n", ret);
    } else if (which_cmd == CD_CMD) {
        ret = send_message(sk, command, cmd_len, &sk_addr);
        printf("Bytes sent: %d\n\n\n", ret);

        printf("Sending argument for cd\n");
        ret = send_message(sk, arg, arg_len, &sk_addr);
        printf("Bytes sent: %d\n\n\n", ret);


    } else if (which_cmd == BRCAST_CMD) {

        /* Allowing broadcast */
        int confirm = 1;
        setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &confirm, sizeof(confirm));

        struct sockaddr_in receiver_data;
        receiver_data.sin_family = AF_INET;
        receiver_data.sin_port = htons(PORT); /* using here htons for network byte order conversion */
        receiver_data.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        /* Binding socket */
        struct sockaddr_in bind_data;
        bind_data.sin_family = AF_INET;
        bind_data.sin_port = htons(0);
        bind_data.sin_addr.s_addr = htonl(INADDR_ANY);

        ret = bind(sk, (struct sockaddr*) &bind_data, sizeof(bind_data));
        if (ret < 0) {
            ERROR(errno);
            return -1;
        }

        printf("Sending broadcast message\n");
        ret = send_message(sk, command, cmd_len, &receiver_data);
        printf("Bytes sent: %d\n\n\n", ret);

        /* Receiving broadcast */
        /* Buffer for data */
        char buf[BUFSIZ];
        struct sockaddr_in sender_data;
        socklen_t addrlen = sizeof(sender_data);

        ret = recvfrom(sk, buf, BUFSIZ, 0, (struct sockaddr*) &sender_data, &addrlen);
        if (ret < 0) {
            ERROR(errno);
            return -1;
        }

        printf("Bytes received: %d\n", ret);

        printf("Server address received from broadcast: %s\n\n\n", inet_ntoa(sender_data.sin_addr));
    }

    if (ret < 0) {
        fprintf(stderr, "Error sending message\n");
        close(sk);
        exit(EXIT_FAILURE);
    } else {
        printf("Bytes sent: %d\n", ret);
    }

    close(sk);

    #endif

    return 0;
}