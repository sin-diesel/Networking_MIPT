/* This is server program, which creates a server and listens for
    messages from clients */

#include "my_server.h"

int main() {

    /* creating and initializing socket */
    int sk = 0;
    int res = 0;

    sk = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sk < 0) {
        ERROR(errno);
        return -1;
    }

    struct sockaddr_un sk_addr = {0};

    /* init socket address and family */
    init_address(&sk_addr);

    res = bind(sk, (struct sockaddr*) &sk_addr, sizeof(sk_addr));

    if (res < 0) {
        ERROR(errno);
        close(sk);
        return -1;
    }

    /* activate socket and listen for transmissions */
    res = listen(sk, MAX_QUEUE);

    if (res < 0) {
        ERROR(errno);
        close(sk);
        return -1;
    }

    /* Listen for clients */
    while (1) {
        // client connects to socket
        int client_sk = 0;

        client_sk = accept(sk, NULL, NULL);

        if (client_sk < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }
        

        /* read message that was accepted */
        res = read_message(client_sk);
        /* finish communication */
        close(client_sk);
        // примечание: если файл (или сокет) удалили с файловой системы, им еще могут пользоваться программы которые не закрыли его до закрытия
    }

    return 0;
}