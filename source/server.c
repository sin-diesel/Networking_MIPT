/* This is server program, which creates a server and listens for
    messages from clients */


/* TODO */
/* 1) fix not recognized command */

#include "my_server.h"

int main() {

    /* creating and initializing socket */
    int sk = 0;
    int res = 0;


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

    struct sockaddr_in sk_addr = {0};
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(23456);
    sk_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // fix later

    #endif

    res = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

    if (res < 0) {
        ERROR(errno);
        close(sk);
        return -1;
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

        char buf[BUFSZ];
        char msg[BUFSZ];
        res = recvfrom(sk, buf, BUFSZ, 0, NULL, NULL);
        buf[res] = '\0';

        if (res < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }


        if (strncmp(buf, PRINT, PRINT_LEN) == 0) {
            /* read message and print it */
            res = read(sk, msg, BUFSZ);
            if (res < 0 || res >= BUFSZ) {
                printf("Unexpected read error or overflow %d\n", res);
                return -1;
            }

            msg[res] = '\0';

            /* Print message */
            printf("Message from client: %s\n", msg);
        } else if (strncmp(buf, EXIT, EXIT_LEN) == 0) {
                close(sk);
                unlink(PATH);
                exit(EXIT_SUCCESS);
        } else if (strncmp(buf, LS, LS_LEN) == 0) {

            printf("Executing LS command\n");
            int pid = fork();
            if (pid == 0) {
                execlp("ls", "ls");
            }

        } else if (strncmp(buf, CD, CD_LEN) == 0) {
            printf("Executing CD command\n");

            res = recvfrom(sk, buf, BUFSZ, 0, NULL, NULL);
            //printf("res received: %d\n", res);
            buf[res] = '\0';
            if (res < 0) {
                ERROR(errno);
                exit(EXIT_FAILURE);
            }
            printf("Arg: %s\n", buf);
            int arg_len = strlen(buf);

            /* Null terminate the string so \n does not interfere */
            buf[arg_len] = '\0';

            res = chdir(buf);
            if (res < 0) {
                ERROR(errno);
            }

        } else {
            printf("Command from client not recognized\n");
            printf("%s\n", buf);
            printf("msg len: %d\n", strlen(buf));
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