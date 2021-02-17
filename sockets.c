

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>


//#include <netinet/in.h> // for internet connection
#include <arpa/inet.h>
// local area network
/* several machines connected together */
/* MAC - add

socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in
htonl() - host to network long, used for conversion byte order
ntohl() from network to host
htonl for ip, htons for ports
inet_addr - converts string with ip address to binary representation
INADDR_LOOPBACK - ip addres of yourself
*/

int main () {
	printf("%d\n", INADDR_LOOPBACK);
	return 0;
}