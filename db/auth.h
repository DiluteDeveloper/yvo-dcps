#include <libpq-fe.h>

int yvo_register_user(PGconn* conn, const char* username, unsigned int username_len, 
        const char* password, unsigned int password_len);
int yvo_login_user(PGconn* conn, const char* username, unsigned int username_len, 
        const char* password, unsigned int password_len);
// extern const char* yvo_query_login_username_buffer_same_thread();
