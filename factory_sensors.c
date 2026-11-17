#include "factory_sensors.h"
#include <open62541/plugin/log_stdout.h>
#include <stdlib.h> // For rand()

#define TEMP_NODE_ID 1001
#define RPM_NODE_ID 1002

void define_object_and_variables(UA_Server *server) {
    // 1. Setup Machine Temperature Variable
    UA_VariableAttributes tempAttr = UA_VariableAttributes_default;
    UA_Double currentTemp = 25.0;
    UA_Variant_setScalar(&tempAttr.value, &currentTemp, &UA_TYPES[UA_TYPES_DOUBLE]);
    tempAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Machine Temperature");
    tempAttr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, TEMP_NODE_ID),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "Temperature"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              tempAttr, NULL, NULL);

    // 2. Setup Motor RPM Variable
    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    UA_Int32 currentRpm = 0;
    UA_Variant_setScalar(&rpmAttr.value, &currentRpm, &UA_TYPES[UA_TYPES_INT32]);
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Motor RPM");
    rpmAttr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, RPM_NODE_ID),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "Motor RPM"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              rpmAttr, NULL, NULL);
}

void update_sensor_data(UA_Server *server, void *data) {
    // Generate minor random fluctuations
    UA_Double newTemp = 25.0 + (rand() % 100) / 10.0; // Varies between 25.0 and 34.9
    UA_Variant tempVar;
    UA_Variant_setScalar(&tempVar, &newTemp, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, TEMP_NODE_ID), tempVar);

    UA_Int32 newRpm = 1400 + (rand() % 200); // Varies between 1400 and 1599
    UA_Variant rpmVar;
    UA_Variant_setScalar(&rpmVar, &newRpm, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, RPM_NODE_ID), rpmVar);

    // Log to console so we can visually verify it's working without needing an external client
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Machine Telemetry -> Temp: %.1f C | Motor: %d RPM", newTemp, newRpm);
}
