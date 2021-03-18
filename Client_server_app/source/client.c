
#include "my_server.h"


int main(int argc, char** argv) {

    /* UDP connection by default */
    int connection_type = UDP_CON;

    if (argc == 2) {
        if (strcmp(argv[1], "--udp") == 0) {
            printf("UDP connection set\n");
            connection_type = UDP_CON;
        } else if (strcmp(argv[1], "--tcp") == 0) {
            printf("TCP connection set\n");
            connection_type = TCP_CON;
        } else {
            printf("Invalid connection type.\n");
            exit(EXIT_FAILURE);
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
    if (connection_type == UDP_CON) {
        sk = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        sk = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (sk < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    /* Initialize client address */
    addr_init(&sk_addr, INADDR_LOOPBACK);

    /* Binding socket */
    addr_init(&sk_bind, INADDR_ANY);

    /* Allowing broadcast */
    int confirm = 1;
    setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &confirm, sizeof(confirm));

    addr_init(&sk_broad, INADDR_BROADCAST);

    /* Use connect if TCP enabled */
    if (connection_type == TCP_CON) {
        ret = connect(sk, &sk_addr, addrlen);
        if (ret < 0) {
            ERROR(errno);
            exit(EXIT_FAILURE);
        }
    }
    

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
            if (connection_type == TCP_CON) {
                printf("Unicast only in TCP.\n");
                printf("Enter another command");
            } else {
                ask_broadcast(sk, &msg, &sk_broad, &server_data, &addrlen);
            }
        } else {
            /* Send other message either with send or send_message depending on the protocol */
            if (connection_type == UDP_CON) {
                ret = send_message(sk, &msg, sizeof(struct message), &sk_addr);
            } else {
                ret = send(sk, &msg, sizeof(struct message), 0);
            }
            printf("Bytes sent: %d\n\n\n", ret);
            
            /* Receive reply from server */
            if (connection_type == UDP_CON) {
                ret = recvfrom(sk, &msg, sizeof(struct message), 0, (struct sockaddr*) &server_data, &addrlen);
            } else {
                while ((ret = read(sk, &msg, sizeof(struct message))) != sizeof(struct message)) {
                    printf("Bytes received: %d\n", ret);
                }
            }
            printf("Bytes received: %d\n", ret);
            if (ret < 0) {
                ERROR(errno);
                close(sk);
                exit(EXIT_FAILURE);
            }
            
            if (ret != sizeof(struct message)) {
                printf("Error receiving message in client\n");
                close(sk);
                exit(EXIT_FAILURE);
            }
            printf("Message received:\n");
            printf("ID: %d\n", msg.id);
            printf("Command: %s\n", msg.cmd);
            printf("Data: %s\n", msg.data);
        }
    }
    close(sk);

    return 0;
}