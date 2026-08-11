#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include "messages.h"
#include "db/auth.h"

struct YVOLoggedInUser {
    int socket;
    const char* username; 
};
struct YVOLoggedInUser yvo_users[50];
int yvo_next_user = 0;
pthread_mutex_t yvo_users_mutex = PTHREAD_MUTEX_INITIALIZER;

const char COMMAND_PREFIX = '/';

#define YVO_REGISTER_COMMAND "register"
#define YVO_LOGIN_COMMAND "login"
#define YVO_MSG_COMMAND "msg"

void yvo_handle_sigint_client(int sig) {
	pthread_mutex_lock(&yvo_users_mutex);
	for(unsigned int i = 0; i < yvo_next_user; i++) {
		close(yvo_users[i].socket); 
	}
	pthread_mutex_unlock(&yvo_users_mutex);
}
int yvo_process_command(struct YVOClientThreadParams* params, char* msg, unsigned int len);
int yvo_process_message(struct YVOClientThreadParams* params, char* msg, unsigned int len) {
    printf("message: %s\n", msg);
    if(len == 0) {
        fprintf(stderr, "process_message received message with length 0\n");
        return -1;
    }
    if(*msg == COMMAND_PREFIX) {
        return yvo_process_command(params, msg + 1, len);
    }
    return 0;
}

int yvo_process_command(struct YVOClientThreadParams* params, char* cmd, unsigned int len) {
    printf("Processing command: /%s", cmd);
    // char cmd[MAX_COMMAND_LENGTH];
    // unsigned int cmd_len = 0;
    // for(unsigned int i = 0; i < len; i++) {
    //     if(i <= MAX_COMMAND_LENGTH) {
    //         fprintf(stderr, "yvo_process_command failed: Command exceeded maximum command length");
    //     }
    //     cmd[i] = msg[i];
    //     cmd_len++;
    //     if(msg[i] == ' ')
    //         break;
    // }
    char* first_space_ptr = strchr(cmd, ' ');
    unsigned int cmd_len = first_space_ptr ? (first_space_ptr - cmd) : len;
    // printf("COMMAND: %s\n LEN: %d", cmd, cmd_len);
    if(strncmp(cmd, YVO_REGISTER_COMMAND, strlen(YVO_REGISTER_COMMAND)) == 0) {
        if(cmd_len == len) {
            fprintf(stderr, "yvo_process_command failed: malformed register command: no args\n");
            return -1;
        }
        char* username_ptr = cmd + cmd_len + 1;
        char* username_space_ptr = strchr(username_ptr, ' ');
        if(username_space_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed register command: no password supplied\n");
            return -1;
        }
        unsigned int username_len = username_space_ptr - username_ptr;
        // if(username_len > YVO_MAX_USERNAME_LENGTH) {
        //     fprintf(stderr, "yvo_process_command failed: malformed register command: username is too long\n");
        //     return -1;
        // }
        char* password_ptr = username_ptr + username_len + 1;
        char* password_end_ptr = strchr(password_ptr, '\n');
        if(password_end_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed register command: password not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int password_len = password_end_ptr - password_ptr;
        // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
        //     fprintf(stderr, "yvo_process_command failed: malformed register command: password is too long\n");
        //     return -1;
        // }
        printf("Password len: %d", password_len);
        if(yvo_register_user(params->conn, username_ptr, username_len, password_ptr, password_len) == -1) {
            fprintf(stderr, "yvo_process_command failed: yvo_register_user failed\n");
            return -1;
        }
        printf("Successfully registered user.\n");
        return 0;
    }
    else if(strncmp(cmd, YVO_LOGIN_COMMAND, strlen(YVO_LOGIN_COMMAND)) == 0) {
        if(cmd_len == len) {
            fprintf(stderr, "yvo_process_command failed: malformed login command: no args\n");
            return -1;
        }
        char* username_ptr = cmd + cmd_len + 1;
        char* username_space_ptr = strchr(username_ptr, ' ');
        if(username_space_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed login command: no password supplied\n");
            return -1;
        }
        unsigned int username_len = username_space_ptr - username_ptr;
        // if(username_len > YVO_MAX_USERNAME_LENGTH) {
        //     fprintf(stderr, "yvo_process_command failed: malformed login command: username is too long\n");
        //     return -1;
        // }
        char* password_ptr = username_ptr + username_len + 1;
        char* password_end_ptr = strchr(password_ptr, '\n');
        if(password_end_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed login command: password not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int password_len = password_end_ptr - password_ptr;
        // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
        //     fprintf(stderr, "yvo_process_command failed: malformed login command: password is too long\n");
        //     return -1;
        // }
        if(yvo_login_user(params->conn, username_ptr, username_len, password_ptr, password_len) == -1) {
            fprintf(stderr, "yvo_process_command failed: yvo_login_user failed\n");
            return -1;
        }
        printf("Successfully logged in user.\n");
		send(params->client_socket, "SUCCESS", 8, 0);
	    pthread_mutex_lock(&yvo_users_mutex);
        yvo_users[yvo_next_user].socket = params->client_socket;
        yvo_users[yvo_next_user].username = malloc(username_len);
        memcpy((void*)yvo_users[yvo_next_user].username, (void*)username_ptr, username_len);
        yvo_next_user++;
	    pthread_mutex_unlock(&yvo_users_mutex);
        return 0;
    }
    else if(strncmp(cmd, YVO_MSG_COMMAND, strlen(YVO_MSG_COMMAND)) == 0) {
        if(cmd_len == len) {
            fprintf(stderr, "yvo_process_command failed: malformed message command: no args\n");
            return -1;
        }
        char* username_ptr = cmd + cmd_len + 1;
        char* username_space_ptr = strchr(username_ptr, ' ');
        if(username_space_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed message command: no send user supplied\n");
            return -1;
        }
        unsigned int username_len = username_space_ptr - username_ptr;
        // if(username_len > YVO_MAX_USERNAME_LENGTH) {
        //     fprintf(stderr, "yvo_process_command failed: malformed message command: send username is too long\n");
        //     return -1;
        // }
        char* msg_ptr = username_ptr + username_len + 1;
        char* msg_end_ptr = strchr(msg_ptr, '\n');
        if(msg_end_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed msg command: message not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int msg_len = msg_end_ptr - msg_ptr;
        // if(msg_len > YVO_MAX_PASSWORD_LENGTH) {
        //     fprintf(stderr, "yvo_process_command failed: malformed login command: password is too long\n");
        //     return -1;
        // }
        // if(yvo_login_user(params->conn, username_ptr, username_len, password_ptr, password_len) == -1) {
        //     fprintf(stderr, "yvo_process_command failed: yvo_login_user failed\n");
        //     return -1;
        // }
	    pthread_mutex_lock(&yvo_users_mutex);
        for(int i = 0; i < yvo_next_user; i++) {
            printf("Username: %s", yvo_users[i].username);
           if(strncmp(yvo_users[i].username, username_ptr, strlen(yvo_users[i].username)) == 0) {
		        send(yvo_users[i].socket, msg_ptr, msg_len, 0);
                printf("Successfully sent message.\n");
	            pthread_mutex_unlock(&yvo_users_mutex);
                return 0;
           } 
        }
	    pthread_mutex_unlock(&yvo_users_mutex);
        printf("Message recipient not found.\n");
        return -1;
    }
    fprintf(stderr, "yvo_process_command failed: command was not recognised\n");
    return -1;
}
