#include "connect.h"

const char* YVO_CONNECTION_STRING = "postgresql://localhost/yvo_dcps_db";

PGconn* yvo_connect_db() {
    PGconn *conn = PQconnectdb(YVO_CONNECTION_STRING);
    if (PQstatus(conn) == CONNECTION_OK) {
        return conn;
    } else {
        return NULL;
    }
}
void yvo_disconnect_db(PGconn* conn) {
    PQfinish(conn);
}
