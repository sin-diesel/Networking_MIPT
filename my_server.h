#ifndef _myserver_h
#define _myserver_h


#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define PRINT "--print"
#define EXIT "--exit"

#define PATH "/tmp/mysock"
#define BUFSZ 256




#endif _myserver_h