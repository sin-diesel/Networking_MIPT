
#include "my_server.h"

int main () {


    // creating and initializing socket
    int sk, ret;
    struct sockaddr_un name = {0};

    sk = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sk < 0) {
        perror("Unable to create socket");
        return 1;
    }

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, PATH, sizeof(PATH));

    ret = bind(sk, (struct sockaddr*) &name, sizeof(name));

    if (ret < 0) {
        perror("Unable to create socket");
        close(sk);
        return 1;
    }

    /* activate socket and listen for transmissions */

    ret = listen(sk, 20);

    if (ret < 0) {
        perror("Unable to listen");
        close(sk);
        return 1;
    }

    while (1) {
        // client connects to socket
        int client_sk;
        client_sk = accept(sk, NULL, NULL);

        char buffer[BUFSZ] = {};


        if (client_sk < 0) {
            perror("Unable to accept client");
            exit(EXIT_FAILURE);
        }

        ret = read(client_sk, buffer, BUFSZ);
        if (ret < 0 ||  ret >= BUFSZ) {
            printf("Unexpected read error or overflow %d\n", ret);
            exit(EXIT_FAILURE);
        }

        if (strcmp(buffer, PRINT) == 0) {
            char msg[BUFSZ];
            /* read message */
            ret = read(client_sk, msg, BUFSZ);
            if (ret < 0 || ret >= BUFSZ) {
                printf("Unexpected read error or overflow %d\n", ret);
                exit(EXIT_FAILURE);
            }
            printf("%s\n", msg);

        } else if (strcmp(buffer, EXIT) == 0) {
            close(client_sk);
            unlink(PATH);
            return 0;
        } else {
            printf("Command from client not recognized\n");
        }

        //printf("%s\n", buffer);
        close(client_sk);
        // примечание: если файл (или сокет) удалили с файловой системы, им еще могут пользоваться программы которые не закрыли его до закрытия

    }







    return 0;
}