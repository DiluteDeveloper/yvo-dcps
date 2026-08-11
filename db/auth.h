#include <libpq-fe.h>

#define YVO_MAX_USERNAME_LENGTH 30
#define YVO_MAX_PASSWORD_LENGTH 30
int yvo_register_user(PGconn* conn, const char* username, unsigned int username_len, 
        const char* password, unsigned int password_len);
int yvo_login_user(PGconn* conn, const char* username, unsigned int username_len, 
        const char* password, unsigned int password_len);
