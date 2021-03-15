
#include "my_server.h"


int main(int argc, char** argv) {

    int connection_type = UDP_CON;

    if (argc == 2) {
        if (strcmp(argv[1], "--udp")) {
            connection_type = UDP_CON;
        } else if (strcmp(argv[1], "--tcp")) {
            connection_type = TCP_CON;
        }
    }

    int sk = 0;
    struct sockaddr_in sk_addr;
    struct sockaddr_in sk_bind;
    struct sockaddr_in sk_broad;
    struct message msg;
    struct sockaddr_in server_data;
    socklen_t addrlen = sizeof(server_data);

    int ret = 0;

    /* Init socket address and family */
    sk = socket(AF_INET, SOCK_DGRAM, 0);
    addr_init(&sk_addr, INADDR_LOOPBACK);

    if (sk < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    // bind - для тех сокетов, который появляются в системе, для остальных не надо 
    /* Binding socket */
    addr_init(&sk_bind, INADDR_ANY);

    /* Allowing broadcast */
    int confirm = 1;
    setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &confirm, sizeof(confirm));

    addr_init(&sk_broad, INADDR_BROADCAST);
    

    while(1) {
        /* Read command */
        char input[BUFSIZ];
        char cmd[CMDSIZE];
        char args[MSGSIZE];

        memset(input, 0, BUFSIZ);
        memset(cmd, 0, CMDSIZE);
        memset(args, 0, MSGSIZE);

        ret = get_input(input);
        if (ret < 0) {
            printf("Error in input.\n");
            exit(EXIT_FAILURE);
        }

        /* Send command to a server */
        int input_len = 0;
        int cmd_len = 0;
        int args_len = 0;
        input_len = strlen(input); // fix later

        printf("Input: %s\n", input);
        printf("Input length: %d\n", input_len);

        ret = get_cmd(input, cmd);
        if (ret < 0) {
            printf("Error in parsing command.\n");
            exit(EXIT_FAILURE);
        }

        cmd_len = strlen(cmd);
        printf("Cmd: %s\n", cmd);
        printf("Cmd length: %d\n", cmd_len);

        ret = get_args(input, args);
        if (ret < 0) {
            printf("Error in parsing args.\n");
            printf("No args provided.\n");
        }

        if (ret >= 0) {
            args_len = strlen(args);
            printf("Args: %s\n", args);
            printf("Args length: %d\n", args_len);
        }

        /* Exit client if command quit was specified */
        if (strncmp(cmd, QUIT, QUIT_LEN) == 0) {
            printf("Exiting client.\n");
            return 0;
        }

        /* For now pid is identifier */
        pid_t pid = getpid();
        /* Sending message containing command, client identifier and arguments */

        memset(&msg, '\0', sizeof(struct message));
        memcpy(msg.cmd, cmd, cmd_len);
        memcpy(&(msg.id), &pid, sizeof(pid_t));
        memcpy(msg.data, args, args_len);

        printf("Message to be sent:\n");
        printf("ID: %d\n", msg.id);
        printf("Command: %s\n", msg.cmd);
        printf("Data: %s\n", msg.data);
        printf("Sending command\n");

        /* Send broadcast message */
        if (strncmp(msg.cmd, BROAD, BROAD_LEN) == 0) {
            ask_broadcast(sk, &msg, &sk_broad, &server_data, &addrlen);
        } else {
            /* Send other message */
            if (connection_type == UDP_CON) {
                send_to_server(sk, &msg, &sk_addr, &server_data, &addrlen);
            } else {
                ret = connect(sk, &sk_addr, addrlen);
                if (ret < 0) {
                    ERROR(errno);
                    exit(EXIT_FAILURE);
                }
                send_to_server(sk, &msg, &sk_addr, &server_data, &addrlen);
            }
        }
    }
    close(sk);

    return 0;
}