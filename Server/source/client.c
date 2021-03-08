
#include "my_server.h"


int main(int argc, char** argv) {

    /* Get input */
    char cmd[CMDSIZE];
    // char* command = NULL;
    // char* arg = NULL;
    // int which_cmd = check_input(argc, argv, &command, &arg);
    // if (which_cmd == BAD_CMD || command == NULL) {
    //     fprintf(stderr, "Error in command recognition\n");
    //     exit(EXIT_FAILURE);
    // }

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

    /* Binding socket */
    struct sockaddr_in bind_data;
    bind_data.sin_family = AF_INET;
    bind_data.sin_port = htons(0);
    bind_data.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sk, (struct sockaddr*) &bind_data, sizeof(bind_data));
    if (ret < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    }

    /* Allowing broadcast */
    int confirm = 1;
    setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &confirm, sizeof(confirm));

    struct sockaddr_in receiver_data;
    receiver_data.sin_family = AF_INET;
    receiver_data.sin_port = htons(PORT); /* using here htons for network byte order conversion */
    receiver_data.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    

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


        /* For now pid is identifier */
        pid_t pid = getpid();

        /* Sending message containing command, client identifier and arguments */
        struct message msg;
        struct sockaddr_in sender_data;
        socklen_t addrlen = sizeof(sender_data);

        memset(&msg, '\0', sizeof(struct message));
        memcpy(msg.cmd, cmd, cmd_len);
        memcpy(&(msg.id), &pid, sizeof(pid_t));
        memcpy(msg.data, args, args_len);

        printf("Message to be sent:\n");
        print_info(&msg);

        printf("Sending command\n");

        /* Send broadcast message */
        if (strncmp(msg.cmd, BROAD, BROAD_LEN) == 0) {
            printf("Sending broadcast message\n");
            ret = send_message(sk, &msg, sizeof(struct message), &receiver_data);
            printf("Bytes sent: %d\n\n\n", ret);
        } else {
            ret = send_message(sk, &msg, sizeof(struct message), &sk_addr);
            printf("Bytes sent: %d\n\n\n", ret);
        }

        if (ret < 0) {
            fprintf(stderr, "Error sending message\n");
            close(sk);
            exit(EXIT_FAILURE);
        }

        /* Buffer for IP address from server */
        char buf[MSGSIZE];
        ret = recvfrom(sk, buf, MSGSIZE, 0, (struct sockaddr*) &sender_data, &addrlen);
        if (ret < 0) {
            close(sk);
            ERROR(errno);
            return -1;
        }

        printf("Bytes received: %d\n", ret);
        char* addr = inet_ntoa(sender_data.sin_addr);
        if (addr == NULL) {
            printf("Server address invalid\n");
        }
        printf("Server address received from broadcast: %s\n", addr);
        print_info(&msg);

        /* Here we manually enter commands */

    //     printf("Enter number of arguments:");
    //     ret = scanf("%d", &argc);
    //     if (ret != 1) {
    //         printf("Error reading input\n");
    //     }

    //     for (int i = 0; i < argc; ++i) {
    //         argv[i] = (char*) calloc(MSGSIZE, sizeof(char)); // Do not forget to free later
    //     }

    //     printf("Enter arguments:");
    //     for (int i = 0; i < argc; ++i) {
    //         ret = scanf("%s", argv[i]);
    //         if (ret != 1) {
    //             printf("Error reading input\n");
    //         }
    //     }

    //     which_cmd = check_input(argc, argv, &command, &arg);
    //     if (which_cmd == BAD_CMD || command == NULL) {
    //         fprintf(stderr, "Error in command recognition\n");
    //         exit(EXIT_FAILURE);
    //     }
    //     printf("Command entered: %s\n", command);
    }
    close(sk);

    #endif

    return 0;
}