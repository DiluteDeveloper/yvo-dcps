#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "netio/sock.h"
#include "netio/listen.h"

void* handle_client(void* client_fd);

int yvo_server_socket;

int clients[50];
int last_client = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_sigint(int sig) {
	close(yvo_server_socket);
	pthread_mutex_lock(&client_mutex);
	for(unsigned int i = 0; i < last_client + 1; i++) {
		close(clients[i]); 
	}
	pthread_mutex_unlock(&client_mutex);
}

int main() {
	setbuf(stdout, NULL);
	yvo_server_socket = yvo_create_socket_ipv4_tcp(0x7F000001, 10000);
	signal(SIGINT, handle_sigint);
	yvo_listen(yvo_server_socket, 10);

	while(true) {
		int client_socket;
		if((client_socket = accept(yvo_server_socket, NULL, NULL)) == -1) {
			perror("Error occurred accepting connection on socket:");
			return -1;
		}

		printf("Successfully accepted connection on socket. Processing client...\n");

		pthread_mutex_lock(&client_mutex);
		pthread_t thread;
		clients[last_client] = client_socket;
		pthread_mutex_unlock(&client_mutex);
		last_client++;

		// Moves client_socket into other thread
		pthread_create(&thread, NULL, handle_client, &client_socket);
	}

	return 0;

}

void* handle_client(void* client_fd) {
	printf("Handling client %d...\n", *(int*)client_fd);
	const char* msg = "Thanks for coming!\n";
	while(true) {
		char buffer[100] = {0};
		int ptr = 0;
		int last_newline = 0;
		recv(*(int*)client_fd, &buffer, sizeof(buffer), 0);
		while(ptr< 100) {
			if(buffer[ptr]=='M' && buffer[ptr + 1]=='S' && buffer[ptr + 2]=='G') {
				int client_idx = (int)strtol(&buffer[last_newline], NULL, 10);
				buffer[ptr] = '\n';
				// pthread_mutex_lock(&client_mutex);
				send(client_idx, buffer + 1 + last_newline, ptr, 0);
				// pthread_mutex_unlock(&client_mutex);
				last_newline = ptr;
			}
			ptr++;
		}

	}
	return NULL;
}
