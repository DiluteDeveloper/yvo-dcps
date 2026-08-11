#include <stdio.h>
#include <sys/socket.h>

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <libpq-fe.h>
#include <string.h>
#include "netio/sock.h"
#include "netio/listen.h"

#include "db/connect.h"
#include "messages.h"
#include "client_thread_params.h"

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
	setbuf(stderr, NULL);
    PGconn* conn = yvo_connect_db();
    if(conn == NULL) {
        fprintf(stderr, "Failed to connect to database");
        return -1;
    }

	yvo_server_socket = yvo_create_socket_ipv4_tcp(0x7F000001, 10000);
	signal(SIGINT, handle_sigint);
	yvo_listen(yvo_server_socket, 10);

	while(true) {
		int client_socket;
		if((client_socket = accept(yvo_server_socket, NULL, NULL)) == -1) {
			perror("Error occurred accepting connection on socket:");
			return -1;
		}
        struct YVOClientThreadParams ctp;
        ctp.conn = conn;
        ctp.client_socket = client_socket;

		printf("Successfully accepted connection on socket. Processing client...\n");

		pthread_mutex_lock(&client_mutex);
		pthread_t thread;
		clients[last_client] = client_socket;
		pthread_mutex_unlock(&client_mutex);
		last_client++;

		// Moves client_socket into other thread
        handle_client((void*)&ctp);
		// pthread_create(&thread, NULL, handle_client, (void*)&ctp);
	}

    yvo_disconnect_db(conn);
	return 0;

}

void* handle_client(void* in) {
    struct YVOClientThreadParams* ctp = (struct YVOClientThreadParams*)in;
    printf("Client %d connected.", ctp->client_socket);
	// printf("Handling client %d...\n", *(int*)client_fd);
    // yvo_insert_connection(*(int*)client_fd);
	// const char* msg = "Thanks for coming!\n";
    while(true) {
        char buffer[200] = {0};
        ssize_t dc_flag = recv(ctp->client_socket, &buffer, sizeof(buffer), 0);
        if(dc_flag <= 0) {
            printf("Client %d disconnected.", ctp->client_socket);
            return NULL;
        }
        if(yvo_process_message(ctp, buffer, strlen(buffer)) == -1) {
            fprintf(stderr, "yvo_process_message returned -1\n");
        }
    }

		// while(ptr< 100) {
		// 	if(buffer[ptr]=='M' && buffer[ptr + 1]=='S' && buffer[ptr + 2]=='G') {
		// 		int client_idx = (int)strtol(&buffer[last_newline], NULL, 10);
		// 		buffer[ptr] = '\n';
		// 		// pthread_mutex_lock(&client_mutex);
		// 		send(client_idx, buffer + 1 + last_newline, ptr, 0);
		// 		// pthread_mutex_unlock(&client_mutex);
		// 		last_newline = ptr;
		// 	}
		// 	ptr++;
		// }

	return NULL;
}
