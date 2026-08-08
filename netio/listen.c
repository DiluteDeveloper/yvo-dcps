#include <sys/socket.h>
#include <netinet/in.h>
// #include <strings.h>
#include <stdio.h>
#include "listen.h"
int yvo_listen(int socket, int backlog) {
	if(listen(socket, backlog) == -1) {
		perror("Error occurred listening on socket:");
		return -1;
	}
	printf("Listening on socket...\n");
	return 0;
}
