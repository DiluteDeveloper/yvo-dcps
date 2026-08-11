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
#include "db/auth.h"
#include "messages.h"
#include "client_thread_params.h"

void* handle_client(void* client_fd);

int yvo_server_socket;

void yvo_handle_sigint_server(int sig) {
	close(yvo_server_socket);
    yvo_handle_sigint_client(sig);

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
	signal(SIGINT, yvo_handle_sigint_server);
	yvo_listen(yvo_server_socket, 10);

	while(true) {
		int client_socket;
		if((client_socket = accept(yvo_server_socket, NULL, NULL)) == -1) {
			perror("Error occurred accepting connection on socket:");
			return -1;
		}
        struct YVOClientThreadParams* ctp = malloc(sizeof(*ctp));
        ctp->conn = conn;
        ctp->client_socket = client_socket;

		printf("Successfully accepted connection on socket. Processing client...\n");

		// pthread_mutex_lock(&client_mutex);
		pthread_t thread;
		// clients[last_client] = client_socket;
		// pthread_mutex_unlock(&client_mutex);
		// last_client++;

		// Moves client_socket into other thread
        // handle_client((void*)&ctp);
		pthread_create(&thread, NULL, handle_client, (void*)ctp);
	}

    yvo_disconnect_db(conn);
	return 0;

}

void* handle_client(void* in) {
    struct YVOClientThreadParams* ctp = (struct YVOClientThreadParams*)in;
    printf("Client %d connected.", ctp->client_socket);
    while(true) {
        char buffer[200] = {0};
        printf("CLIENT SOCKET!: %d\n", ctp->client_socket);
        ssize_t dc_flag = recv(ctp->client_socket, &buffer, sizeof(buffer), 0);
        printf("recv n=%zd\n", dc_flag);
        for (int i = 0; i < dc_flag; i++) printf("%02x ", (unsigned char)buffer[i]);
        printf("\n");
        if(dc_flag <= 0) {
            printf("Client %d disconnected.", ctp->client_socket);
            return NULL;
        }
        if(yvo_process_message(ctp, buffer, strlen(buffer)) == -1) {
            fprintf(stderr, "yvo_process_message returned -1\n");
        }
    }

	return NULL;
}
