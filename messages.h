#include <libpq-fe.h>
#include "client_thread_params.h"

int yvo_process_message(struct YVOClientThreadParams* params, char* msg, unsigned int len);
