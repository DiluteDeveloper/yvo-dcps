#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "auth.h"
#include "constants.h"

// char* yvo_username_buf = NULL;
// const char* yvo_query_login_username_buffer_same_thread() {
//     return yvo_username_buf;
// }
#define YVO_SQL_QUERY_EXISTING_USER "select username from " YVO_DB_USER_TBL " where username=\'%s\'"
#define YVO_SQL_QUERY_EXISTING_USER_PASS "select password from " YVO_DB_USER_TBL " where username=\'%s\'"
#define YVO_SQL_INSERT_USER "insert into " YVO_DB_USER_TBL " (username, password) VALUES (\'%s\', \'%s\')"

int yvo_register_user(PGconn* conn, const char* username, unsigned int username_len, 
        const char* password, unsigned int password_len) {
    // if(username_len > YVO_MAX_USERNAME_LENGTH) {
    //     fprintf(stderr, "yvo_register_user failed: username is too long");
    //     return -1;
    // }
    // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
    //     fprintf(stderr, "yvo_register_user failed: password is too long");
    //     return -1;
    // }
    char* username_clean = malloc(username_len + 1);
    memcpy(username_clean, username, username_len);
    username_clean[username_len + 1] = '\0';
    char* password_clean = malloc(password_len + 1);
    memcpy(password_clean, password, password_len);
    password_clean[password_len + 1] = '\0';

    char query_existing_user[sizeof(YVO_SQL_QUERY_EXISTING_USER) + username_len];
    snprintf(query_existing_user, sizeof(query_existing_user), YVO_SQL_QUERY_EXISTING_USER, username_clean);

    PGresult* query_existing_user_result = PQexec(conn, query_existing_user);
    if(PQresultStatus(query_existing_user_result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "yvo_register_user failed: query_existing_user failed: %s\n", PQresultErrorMessage(query_existing_user_result));
        return -1;
    }
    if(PQntuples(query_existing_user_result) != 0) {
        fprintf(stderr, "yvo_register_user failed: username already exists");
        return -1;
    }

    PQclear(query_existing_user_result);

    char sql_insert_user[sizeof(YVO_SQL_INSERT_USER) + username_len + password_len];


    snprintf(sql_insert_user, sizeof(sql_insert_user), YVO_SQL_INSERT_USER, username_clean, password_clean);

    printf("sql_insert_user: %s\n", sql_insert_user);
    PGresult* sql_insert_user_result = PQexec(conn, sql_insert_user);
    free(username_clean);
    free(password_clean);
    if(PQresultStatus(sql_insert_user_result) != PGRES_COMMAND_OK) {
        fprintf(stderr, "yvo_register_user failed: sql_insert_user failed: %s\n", PQresultErrorMessage(sql_insert_user_result));
        return -1;
    }
    PQclear(sql_insert_user_result);

    return 0;
}
int yvo_login_user(PGconn* conn, const char* username, unsigned int username_len, 
        const char* password, unsigned int password_len) {
    // if(username_len > YVO_MAX_USERNAME_LENGTH) {
    //     fprintf(stderr, "yvo_login_user failed: username is too long");
    //     return -1;
    // }
    // if(password_len > YVO_MAX_PASSWORD_LENGTH) {
    //     fprintf(stderr, "yvo_login_user failed: password is too long");
    //     return -1;
    // }
    char* username_clean = malloc(username_len + 1);
    memcpy(username_clean, username, username_len);
    username_clean[username_len + 1] = '\0';
    char* password_clean = malloc(password_len + 1);
    memcpy(password_clean, password, password_len);
    password_clean[password_len + 1] = '\0';

    char query_existing_user_pass[sizeof(YVO_SQL_QUERY_EXISTING_USER_PASS) + username_len];
    snprintf(query_existing_user_pass, sizeof(query_existing_user_pass), YVO_SQL_QUERY_EXISTING_USER_PASS, username_clean);

    PGresult* query_existing_user_pass_result = PQexec(conn, query_existing_user_pass);
    if(PQresultStatus(query_existing_user_pass_result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "yvo_login_user failed: query_existing_user_pass failed: %s\n", PQresultErrorMessage(query_existing_user_pass_result));
        return -1;
    }
    // printf("B5");
    if(PQntuples(query_existing_user_pass_result) == 0) {
        fprintf(stderr, "yvo_login_user failed: account username does not exist");
        return -1;
    }
    // printf("pass entered:%s\n", password);
    // printf("actual pass:%s\n", PQgetvalue(query_existing_user_pass_result, 0, 0));
    // printf("actual pass:%d\n", strcmp(password, PQgetvalue(query_existing_user_pass_result, 0, 0)) != 0);
    // printf("p0 strlen: %zu, p1 strlen: %zu", strlen(password), strlen(PQgetvalue(query_existing_user_pass_result, 0, 0)));
    if(strcmp(password_clean, PQgetvalue(query_existing_user_pass_result, 0, 0)) != 0) {
        fprintf(stderr, "yvo_login_user failed: password is invalid");
        PQclear(query_existing_user_pass_result);
        return -1;
    }
    printf("User successfully logged in.");

    PQclear(query_existing_user_pass_result);
    return 0;
}
