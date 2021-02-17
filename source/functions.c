#include "my_server.h"


int check_input(int argc) {
    if (argc < 2) {
        printf("Usage: ./client [--exit] [--print] <message>\n");
        return -1;
    } 
    return 0;
}

int send_message(int sk, int which_cmd, char* command, char* msg) {


    return 0;
}

