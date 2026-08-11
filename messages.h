#include <libpq-fe.h>
#include "client_thread_params.h"

extern void yvo_handle_sigint_client(int sig);
extern int yvo_process_message(struct YVOClientThreadParams* params, char* msg, unsigned int len);
