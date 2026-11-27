#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "hierarchy.h"
#include "variables.h"
#include "logic.h"

static UA_Boolean running = true;

static void stopHandler(int sig) {
    (void)sig;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received stop signal");
    running = false;
}

static void apply_endpoint_config(UA_ServerConfig *serverCfg, const AppConfig *cfg) {
#if (UA_OPEN62541_VER_MAJOR > 1) || (UA_OPEN62541_VER_MAJOR == 1 && UA_OPEN62541_VER_MINOR >= 4)
    char urlBuf[128];
    snprintf(urlBuf, sizeof(urlBuf), "opc.tcp://%s:%u", cfg->hostname, cfg->port);
    if (serverCfg->serverUrlsSize > 0) {
        UA_Array_delete(serverCfg->serverUrls, serverCfg->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    }
    serverCfg->serverUrls = (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    serverCfg->serverUrls[0] = UA_String_fromChars(urlBuf);
    serverCfg->serverUrlsSize = 1;
#else
    UA_String_clear(&serverCfg->customHostname);
    serverCfg->customHostname = UA_String_fromChars(cfg->hostname);
#endif
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    AppConfig cfg;
    const char *cfgPath = (argc > 1) ? argv[1] : "config.ini";
    load_app_config(cfgPath, &cfg);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *serverCfg = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(serverCfg, cfg.port, NULL);
    apply_endpoint_config(serverCfg, &cfg);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Starting OPC UA Server at opc.tcp://%s:%u (cycle=%ums, target=%d)",
                cfg.hostname, cfg.port, cfg.cycle_interval_ms, cfg.default_target);

    build_factory_hierarchy(server);
    add_production_variables(server, &cfg);
    initialize_production_logic(server, &cfg);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}