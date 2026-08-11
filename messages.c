#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "messages.h"
#include "db/auth.h"


const char COMMAND_PREFIX = '/';

#define YVO_REGISTER_COMMAND "register"
#define YVO_LOGIN_COMMAND "login"

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
        if(username_len > YVO_MAX_USERNAME_LENGTH) {
            fprintf(stderr, "yvo_process_command failed: malformed register command: username is too long\n");
            return -1;
        }
        char* password_ptr = username_ptr + username_len + 1;
        char* password_end_ptr = strchr(password_ptr, '\n');
        if(password_end_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed register command: password not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int password_len = password_end_ptr - password_ptr;
        if(password_len > YVO_MAX_PASSWORD_LENGTH) {
            fprintf(stderr, "yvo_process_command failed: malformed register command: password is too long\n");
            return -1;
        }
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
        if(username_len > YVO_MAX_USERNAME_LENGTH) {
            fprintf(stderr, "yvo_process_command failed: malformed login command: username is too long\n");
            return -1;
        }
        char* password_ptr = username_ptr + username_len + 1;
        char* password_end_ptr = strchr(password_ptr, '\n');
        if(password_end_ptr == NULL) {
            fprintf(stderr, "yvo_process_command failed: malformed login command: password not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int password_len = password_end_ptr - password_ptr;
        if(password_len > YVO_MAX_PASSWORD_LENGTH) {
            fprintf(stderr, "yvo_process_command failed: malformed login command: password is too long\n");
            return -1;
        }
        if(yvo_login_user(params->conn, username_ptr, username_len, password_ptr, password_len) == -1) {
            fprintf(stderr, "yvo_process_command failed: yvo_login_user failed\n");
            return -1;
        }
        printf("Successfully logged in user.\n");
		send(params->client_socket, "SUCCESS", 8, 0);
        return 0;
    }
    fprintf(stderr, "yvo_process_command failed: command was not recognised\n");
    return -1;
}
