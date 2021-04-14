
#include "my_server.h"


int main(int argc, char** argv) {

    /* UDP connection by default */
    int connection_type = UDP_CON;
    int sk = 0;
    struct sockaddr_in sk_addr;
    struct sockaddr_in sk_bind;
    struct sockaddr_in sk_broad;

    char* ip_addr = NULL;

    struct sockaddr_in server_data;
    int ret = 0;

    if (argc >= 2) {
        if (strcmp(argv[1], "--udp") == 0) {
            printf("UDP connection set\n");
            connection_type = UDP_CON;
        } else if (strcmp(argv[1], "--tcp") == 0) {
            printf("TCP connection set\n");
            connection_type = TCP_CON;
        } else if (strcmp(argv[1], "--ip") == 0) {
            ip_addr =  argv[2];
            printf("IP address entered: %s\n", ip_addr);
        } else {
            printf("Invalid connection type.\n");
            exit(EXIT_FAILURE);
        }
    }

    

    ret = client_init(connection_type, &sk, ip_addr, &sk_addr, &sk_bind, &sk_broad);
    if (ret < 0) {
        exit(EXIT_FAILURE);
    }
    ret = client_routine(connection_type, sk, &sk_addr, &sk_broad, &server_data);
    if (ret < 0) {
        exit(EXIT_FAILURE);
    }
      
    return 0;
}