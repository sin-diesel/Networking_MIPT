/* This is server program, which creates a server and listens for
    messages from clients */


#include "my_server.h"

int main() {

    /* Creating and initializing socket */
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

    #endif

    ret = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

    if (ret < 0) {
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

        char buf[BUFSIZ];
        char msg[BUFSIZ];

        /* Receiving message from client */
        struct sockaddr_in client_data;
        socklen_t addrlen = sizeof(client_data);

        ret = recvfrom(sk, buf, BUFSIZ, 0, (struct sockaddr*) &client_data, &addrlen);
        if (ret < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

        printf("Bytes received: %d\n", ret);
        /* Null terminating buf */
        buf[ret] = '\0';

        //char receiver_ip[BUFSIZ];
        /* Taking address and handing over to inet_ntoa function */
        //char* addr = &receiver_ip;

        char* addr = inet_ntoa(client_data.sin_addr);
        if (addr == NULL) {
            printf("Client address invalid\n");
        }

        printf("Client addr: %s\n", addr);
        printf("Client port: %d\n", htons(client_data.sin_port));
        if (ret < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }

        /* Decide which message was sent */

        if (strncmp(buf, PRINT, PRINT_LEN) == 0) {
            /* read message and print it */
            ret = read(sk, msg, BUFSIZ);
            if (ret < 0 || ret >= BUFSIZ) {
                printf("Unexpected read error or overflow %d\n", ret);
                return -1;
            }
            msg[ret] = '\0';

            /* Print message */
            printf("Message from client: %s\n", msg);
        } else if (strncmp(buf, EXIT, EXIT_LEN) == 0) {
            /* Closing server */
                close(sk);
                unlink(PATH);
                exit(EXIT_SUCCESS);
        } else if (strncmp(buf, LS, LS_LEN) == 0) {
            /* Printing current directory */
            printf("Executing LS command\n");
            int pid = fork();
            if (pid == 0) {
                execlp("ls", "ls");
            }

        } else if (strncmp(buf, CD, CD_LEN) == 0) {
            printf("Executing CD command\n");

            // res = recvfrom(sk, buf, BUFSZ, 0, NULL, NULL);
            // //printf("res received: %d\n", res);
            // buf[res] = '\0';
            // if (res < 0) {
            //     ERROR(errno);
            //     exit(EXIT_FAILURE);
            // }
            // printf("Arg: %s\n", buf);
            // int arg_len = strlen(buf);

            // /* Null terminate the string so \n does not interfere */
            // buf[arg_len] = '\0';

            // res = chdir(buf);
            // if (res < 0) {
            //     ERROR(errno);
            // }

        } else if (strncmp(buf, BROAD, BROAD_LEN) == 0) {
            printf("Broadcasting server IP\n");
            char message[] = "Reply to client";
            ret =  sendto(sk, &message, sizeof(message), 0,
                             (struct sockaddr*) &client_data, sizeof(client_data));
            if (ret < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }

        }else {
            printf("Command from client not recognized\n");
            printf("Actual buffer sent: %s\n", buf);
        }

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