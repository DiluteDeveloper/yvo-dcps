#include <libpq-fe.h>
#include <stdio.h>

static inline void yvo_insert_connection(int connection_id) {
    PGconn *conn = PQconnectdb("postgresql://localhost/yvo_dcps_db");
    if (PQstatus(conn) == CONNECTION_OK) {
        char query[500];
        snprintf(query, sizeof(query), "insert into platform.t_connections (connection_id) values (%d);", connection_id);
        PQexec(conn, query);
        printf("%s", query);
        // for (int i = 0; i < PQntuples(result); i++) {
        //     char *value = PQgetvalue(result, i, 0);
        //     if (value) printf("%s\n", value);
        // }
        // PQclear(result);
    } else {
        printf("Failed to establish postgreSQL connection.");
    }
    PQfinish(conn);
}
