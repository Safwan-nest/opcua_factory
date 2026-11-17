#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <signal.h>
#include <stdlib.h>

// Custom project headers
#include "hierarchy.h"
#include "variables.h"
#include "logic.h"

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received stop signal");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    
    // Set the default config for port 4840
    UA_ServerConfig_setDefault(config);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Starting OPC UA Manufacturing Server...");

    // 1. Build the physical factory hierarchy
    build_factory_hierarchy(server);
    
    // 2. Inject production counters into the hierarchy
    add_production_variables(server);
    
    // 3. Initialize the 5-second loop and start/stop methods
    initialize_production_logic(server);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}