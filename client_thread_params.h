#pragma once
#include <libpq-fe.h>
struct YVOClientThreadParams {
    PGconn* conn;
    int client_socket;
};
