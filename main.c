#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <signal.h>
#include <stdlib.h>

// Include our modular sensor code
#include "factory_sensors.h" 

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received stop signal");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Starting OPC UA Factory Server...");

    // Setup our sensor variables in the server
    define_object_and_variables(server);

    // Tell the server to run the update_sensor_data function every 2000 milliseconds (2 seconds)
    UA_UInt64 callbackId = 0;
    UA_Server_addRepeatedCallback(server, update_sensor_data, NULL, 2000, &callbackId);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
