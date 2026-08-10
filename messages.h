#include <libpq-fe.h>

int yvo_process_message(PGconn* conn, char* msg, unsigned int len);
