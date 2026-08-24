#include <arpa/inet.h>
#include <libpq-fe.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define YVO_DB_QUERY_EXISTING_USER                                             \
    "select username from " YVO_DB_USER_TABLE " where username=\'%s\'"
#define YVO_DB_QUERY_EXISTING_USER_PASS                                        \
    "select password from " YVO_DB_USER_TABLE " where username=\'%s\'"
#define YVO_DB_INSERT_USER                                                     \
    "insert into " YVO_DB_USER_TABLE                                           \
    " (username, password) VALUES (\'%s\', \'%s\')"

#define YVO_DB_CONNECTION_STRING "postgresql://localhost/yvo_dcps_db"

#define YVO_DB_USER_TABLE "platform.users"

PGconn *yvo_db_connect() {
    PGconn *conn = PQconnectdb(YVO_DB_CONNECTION_STRING);
    if (PQstatus(conn) == CONNECTION_OK) {
        return conn;
    } else {
        return NULL;
    }
}

void yvo_db_disconnect(PGconn *conn) { PQfinish(conn); }

int yvo_db_register_user(PGconn *conn, const char *username,
                         unsigned int username_len, const char *password,
                         unsigned int password_len) {
    // if(username_len > YVO_MAX_USERNAME_LENGTH) {
    //         fprintf(stderr, "yvo_register_user failed: username is too
    //         long"); return -1;
    // }
    // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
    //         fprintf(stderr, "yvo_register_user failed: password is too
    //         long"); return -1;
    // }
    char *username_clean = malloc(username_len + 1);
    memcpy(username_clean, username, username_len);
    username_clean[username_len + 1] = '\0';
    char *password_clean = malloc(password_len + 1);
    memcpy(password_clean, password, password_len);
    password_clean[password_len + 1] = '\0';

    char query_existing_user[sizeof(YVO_DB_QUERY_EXISTING_USER) + username_len];
    snprintf(query_existing_user, sizeof(query_existing_user),
             YVO_DB_QUERY_EXISTING_USER, username_clean);

    PGresult *query_existing_user_result = PQexec(conn, query_existing_user);
    if (PQresultStatus(query_existing_user_result) != PGRES_TUPLES_OK) {
        fprintf(stderr,
                "yvo_register_user failed: query_existing_user failed: %s\n",
                PQresultErrorMessage(query_existing_user_result));
        return -1;
    }
    if (PQntuples(query_existing_user_result) != 0) {
        fprintf(stderr, "yvo_register_user failed: username already exists");
        return -1;
    }

    PQclear(query_existing_user_result);

    char sql_insert_user[sizeof(YVO_DB_INSERT_USER) + username_len +
                         password_len];

    snprintf(sql_insert_user, sizeof(sql_insert_user), YVO_DB_INSERT_USER,
             username_clean, password_clean);

    printf("sql_insert_user: %s\n", sql_insert_user);
    PGresult *sql_insert_user_result = PQexec(conn, sql_insert_user);
    free(username_clean);
    free(password_clean);
    if (PQresultStatus(sql_insert_user_result) != PGRES_COMMAND_OK) {
        fprintf(stderr,
                "yvo_register_user failed: sql_insert_user failed: %s\n",
                PQresultErrorMessage(sql_insert_user_result));
        return -1;
    }
    PQclear(sql_insert_user_result);

    return 0;
}

int yvo_db_login_user(PGconn *conn, const char *username,
                      unsigned int username_len, const char *password,
                      unsigned int password_len) {
    // if(username_len > YVO_MAX_USERNAME_LENGTH) {
    //         fprintf(stderr, "yvo_login_user failed: username is too long");
    //         return -1;
    // }
    // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
    //         fprintf(stderr, "yvo_login_user failed: password is too long");
    //         return -1;
    // }
    char *username_clean = malloc(username_len + 1);
    memcpy(username_clean, username, username_len);
    username_clean[username_len + 1] = '\0';
    char *password_clean = malloc(password_len + 1);
    memcpy(password_clean, password, password_len);
    password_clean[password_len + 1] = '\0';

    char query_existing_user_pass[sizeof(YVO_DB_QUERY_EXISTING_USER_PASS) +
                                  username_len];
    snprintf(query_existing_user_pass, sizeof(query_existing_user_pass),
             YVO_DB_QUERY_EXISTING_USER_PASS, username_clean);

    PGresult *query_existing_user_pass_result =
        PQexec(conn, query_existing_user_pass);
    if (PQresultStatus(query_existing_user_pass_result) != PGRES_TUPLES_OK) {
        fprintf(stderr,
                "yvo_login_user failed: query_existing_user_pass failed: %s\n",
                PQresultErrorMessage(query_existing_user_pass_result));
        return -1;
    }
    // printf("B5");
    if (PQntuples(query_existing_user_pass_result) == 0) {
        fprintf(stderr,
                "yvo_login_user failed: account username does not exist");
        return -1;
    }
    // printf("pass entered:%s\n", password);
    // printf("actual pass:%s\n", PQgetvalue(query_existing_user_pass_result, 0,
    // 0)); printf("actual pass:%d\n", strcmp(password,
    // PQgetvalue(query_existing_user_pass_result, 0, 0)) != 0); printf("p0
    // strlen: %zu, p1 strlen: %zu", strlen(password),
    // strlen(PQgetvalue(query_existing_user_pass_result, 0, 0)));
    if (strcmp(password_clean,
               PQgetvalue(query_existing_user_pass_result, 0, 0)) != 0) {
        fprintf(stderr, "yvo_login_user failed: password is invalid");
        PQclear(query_existing_user_pass_result);
        return -1;
    }
    printf("User successfully logged in.");

    PQclear(query_existing_user_pass_result);
    return 0;
}

struct YVOLoggedInUser {
    int socket;
    const char *username;
};

struct YVOLoggedInUser yvo_users[50];
int yvo_next_user = 0;
pthread_mutex_t yvo_users_mutex = PTHREAD_MUTEX_INITIALIZER;

const char COMMAND_PREFIX = '/';

#define YVO_REGISTER_COMMAND "register"
#define YVO_LOGIN_COMMAND "login"
#define YVO_MSG_COMMAND "msg"

struct YVOClientThreadParams {
    PGconn *conn;
    int client_socket;
};

int yvo_process_command(struct YVOClientThreadParams *params, char *cmd,
                        unsigned int len) {
    printf("Processing command: /%s", cmd);
    // char cmd[MAX_COMMAND_LENGTH];
    // unsigned int cmd_len = 0;
    // for(unsigned int i = 0; i < len; i++) {
    //         if(i <= MAX_COMMAND_LENGTH) {
    //                 fprintf(stderr, "yvo_process_command failed: Command
    //                 exceeded maximum command length");
    //         }
    //         cmd[i] = msg[i];
    //         cmd_len++;
    //         if(msg[i] == ' ')
    //                 break;
    // }
    char *first_space_ptr = strchr(cmd, ' ');
    unsigned int cmd_len = first_space_ptr ? (first_space_ptr - cmd) : len;
    // printf("COMMAND: %s\n LEN: %d", cmd, cmd_len);
    if (strncmp(cmd, YVO_REGISTER_COMMAND, strlen(YVO_REGISTER_COMMAND)) == 0) {
        if (cmd_len == len) {
            fprintf(stderr, "yvo_process_command failed: malformed register "
                            "command: no args\n");
            return -1;
        }
        char *username_ptr = cmd + cmd_len + 1;
        char *username_space_ptr = strchr(username_ptr, ' ');
        if (username_space_ptr == NULL) {
            fprintf(stderr,
                    "yvo_process_command failed: malformed register command: "
                    "no password supplied\n");
            return -1;
        }
        unsigned int username_len = username_space_ptr - username_ptr;
        // if(username_len > YVO_MAX_USERNAME_LENGTH) {
        //         fprintf(stderr, "yvo_process_command failed: malformed
        //         register command: username is too long\n"); return -1;
        // }
        char *password_ptr = username_ptr + username_len + 1;
        char *password_end_ptr = strchr(password_ptr, '\n');
        if (password_end_ptr == NULL) {
            fprintf(stderr,
                    "yvo_process_command failed: malformed register command: "
                    "password not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int password_len = password_end_ptr - password_ptr;
        // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
        //         fprintf(stderr, "yvo_process_command failed: malformed
        //         register command: password is too long\n"); return -1;
        // }
        printf("Password len: %d", password_len);
        if (yvo_db_register_user(params->conn, username_ptr, username_len,
                                 password_ptr, password_len) == -1) {
            fprintf(stderr,
                    "yvo_process_command failed: yvo_register_user failed\n");
            return -1;
        }
        printf("Successfully registered user.\n");
        return 0;
    } else if (strncmp(cmd, YVO_LOGIN_COMMAND, strlen(YVO_LOGIN_COMMAND)) ==
               0) {
        if (cmd_len == len) {
            fprintf(stderr, "yvo_process_command failed: malformed login "
                            "command: no args\n");
            return -1;
        }
        char *username_ptr = cmd + cmd_len + 1;
        char *username_space_ptr = strchr(username_ptr, ' ');
        if (username_space_ptr == NULL) {
            fprintf(stderr,
                    "yvo_process_command failed: malformed login command: no "
                    "password supplied\n");
            return -1;
        }
        unsigned int username_len = username_space_ptr - username_ptr;
        // if(username_len > YVO_MAX_USERNAME_LENGTH) {
        //         fprintf(stderr, "yvo_process_command failed: malformed login
        //         command: username is too long\n"); return -1;
        // }
        char *password_ptr = username_ptr + username_len + 1;
        char *password_end_ptr = strchr(password_ptr, '\n');
        if (password_end_ptr == NULL) {
            fprintf(stderr,
                    "yvo_process_command failed: malformed login command: "
                    "password not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int password_len = password_end_ptr - password_ptr;
        // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
        //         fprintf(stderr, "yvo_process_command failed: malformed login
        //         command: password is too long\n"); return -1;
        // }
        if (yvo_db_login_user(params->conn, username_ptr, username_len,
                              password_ptr, password_len) == -1) {
            fprintf(stderr,
                    "yvo_process_command failed: yvo_login_user failed\n");
            return -1;
        }
        printf("Successfully logged in user.\n");
        send(params->client_socket, "SUCCESS", 8, 0);
        pthread_mutex_lock(&yvo_users_mutex);
        yvo_users[yvo_next_user].socket = params->client_socket;
        yvo_users[yvo_next_user].username = malloc(username_len);
        memcpy((void *)yvo_users[yvo_next_user].username, (void *)username_ptr,
               username_len);
        yvo_next_user++;
        pthread_mutex_unlock(&yvo_users_mutex);
        return 0;
    } else if (strncmp(cmd, YVO_MSG_COMMAND, strlen(YVO_MSG_COMMAND)) == 0) {
        if (cmd_len == len) {
            fprintf(stderr, "yvo_process_command failed: malformed message "
                            "command: no args\n");
            return -1;
        }
        char *username_ptr = cmd + cmd_len + 1;
        char *username_space_ptr = strchr(username_ptr, ' ');
        if (username_space_ptr == NULL) {
            fprintf(stderr,
                    "yvo_process_command failed: malformed message command: "
                    "no send user supplied\n");
            return -1;
        }
        unsigned int username_len = username_space_ptr - username_ptr;
        // if(username_len > YVO_MAX_USERNAME_LENGTH) {
        //         fprintf(stderr, "yvo_process_command failed: malformed
        //         message command: send username is too long\n"); return -1;
        // }
        char *msg_ptr = username_ptr + username_len + 1;
        char *msg_end_ptr = strchr(msg_ptr, '\n');
        if (msg_end_ptr == NULL) {
            fprintf(stderr,
                    "yvo_process_command failed: malformed msg command: "
                    "message not terminated with carriage return \'\\n\'\n");
            return -1;
        }
        unsigned int msg_len = msg_end_ptr - msg_ptr;
        // if(msg_len > YVO_MAX_PASSWORD_LENGTH) {
        //         fprintf(stderr, "yvo_process_command failed: malformed login
        //         command: password is too long\n"); return -1;
        // }
        // if(yvo_login_user(params->conn, username_ptr, username_len,
        // password_ptr, password_len) == -1) {
        //         fprintf(stderr, "yvo_process_command failed: yvo_login_user
        //         failed\n"); return -1;
        // }
        pthread_mutex_lock(&yvo_users_mutex);
        for (int i = 0; i < yvo_next_user; i++) {
            printf("Username: %s", yvo_users[i].username);
            if (strncmp(yvo_users[i].username, username_ptr,
                        strlen(yvo_users[i].username)) == 0) {
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

int yvo_process_message(struct YVOClientThreadParams *params, char *msg,
                        unsigned int len) {
    printf("message: %s\n", msg);
    if (len == 0) {
        fprintf(stderr, "process_message received message with length 0\n");
        return -1;
    }
    if (*msg == COMMAND_PREFIX) {
        return yvo_process_command(params, msg + 1, len);
    }
    return 0;
}

void *handle_client(void *in) {
    struct YVOClientThreadParams *ctp = (struct YVOClientThreadParams *)in;
    printf("Client %d connected.", ctp->client_socket);
    while (true) {
        char buffer[200] = {0};
        printf("CLIENT SOCKET!: %d\n", ctp->client_socket);
        ssize_t dc_flag = recv(ctp->client_socket, &buffer, sizeof(buffer), 0);
        printf("recv n=%zd\n", dc_flag);
        for (int i = 0; i < dc_flag; i++)
            printf("%02x ", (unsigned char)buffer[i]);
        printf("\n");
        if (dc_flag <= 0) {
            printf("Client %d disconnected.", ctp->client_socket);
            return NULL;
        }
        if (yvo_process_message(ctp, buffer, strlen(buffer)) == -1) {
            fprintf(stderr, "yvo_process_message returned -1\n");
        }
    }

    return NULL;
}

int main() {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    PGconn *conn = yvo_db_connect();
    if (conn == NULL) {
        fprintf(stderr, "Failed to connect to database");
        return -1;
    }

    const char *SERVER_IP = "127.0.0.1";
    const uint16_t SERVER_PORT = 10000;
    const int SERVER_LISTEN_BACKLOG = 1;
    int server_socket;

    if (((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)) {
        printf("Error while creating IPv4 TCP socket.\n");
        return -1;
    }

    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(SERVER_PORT);
    inet_aton(SERVER_IP, &sockaddr.sin_addr);

    if (sockaddr.sin_addr.s_addr == 0) {
        fprintf(stderr, "Server IP address is invalid\n");
        return -1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(server_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    if (listen(server_socket, SERVER_LISTEN_BACKLOG) == -1) {
        fprintf(stderr, "Error occurred listening on socket\n");
        return -1;
    }
    printf("Listening on socket...\n");

    while (true) {
        int client_socket;
        if ((client_socket = accept(server_socket, NULL, NULL)) == -1) {
            perror("Error occurred accepting connection on socket:");
            return -1;
        }
        struct YVOClientThreadParams *ctp = malloc(sizeof(*ctp));
        ctp->conn = conn;
        ctp->client_socket = client_socket;

        printf("Successfully accepted connection on socket. Processing "
               "client...\n");

        // pthread_mutex_lock(&client_mutex);
        pthread_t thread;
        // clients[last_client] = client_socket;
        // pthread_mutex_unlock(&client_mutex);
        // last_client++;

        // Moves client_socket into other thread
        // handle_client((void*)&ctp);
        pthread_create(&thread, NULL, handle_client, (void *)ctp);
    }

    yvo_db_disconnect(conn);
    return 0;
}
