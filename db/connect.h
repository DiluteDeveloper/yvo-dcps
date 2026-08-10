#include <libpq-fe.h>
#include <stdio.h>

extern PGconn* yvo_connect_db();
extern void yvo_disconnect_db(PGconn* conn);
