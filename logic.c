#include "logic.h"
#include <open62541/plugin/log_stdout.h>

static UA_Boolean is_running = false;
static UA_Int32 total_produced = 0;
static UA_Int32 total_accepted = 0;
static UA_Int32 total_rejected = 0;

// Method: StartLine [O09]
static UA_StatusCode start_line_method(UA_Server *server,
                                       const UA_NodeId *sessionId, void *sessionHandle,
                                       const UA_NodeId *methodId, void *methodContext,
                                       const UA_NodeId *objectId, void *objectContext,
                                       size_t inputSize, const UA_Variant *input,
                                       size_t outputSize, UA_Variant *output) {
    is_running = true;
    
    // Update LineState to RUNNING
    UA_Variant stateVar;
    UA_String stateStr = UA_STRING("RUNNING");
    UA_Variant_setScalar(&stateVar, &stateStr, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2001), stateVar); // 2001 is LineState

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Production Line STARTED.");
    return UA_STATUSCODE_GOOD;
}

// Method: StopLine [O10]
static UA_StatusCode stop_line_method(UA_Server *server,
                                      const UA_NodeId *sessionId, void *sessionHandle,
                                      const UA_NodeId *methodId, void *methodContext,
                                      const UA_NodeId *objectId, void *objectContext,
                                      size_t inputSize, const UA_Variant *input,
                                      size_t outputSize, UA_Variant *output) {
    is_running = false;
    
    // Update LineState to STOPPED
    UA_Variant stateVar;
    UA_String stateStr = UA_STRING("STOPPED");
    UA_Variant_setScalar(&stateVar, &stateStr, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2001), stateVar);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Production Line STOPPED.");
    return UA_STATUSCODE_GOOD;
}

// 5-Second Production Loop [O11 - O18]
static void production_loop_callback(UA_Server *server, void *data) {
    if (!is_running) return;

    total_produced++; // [O16]
    
    // Every 10th product is rejected [O14, O15]
    if (total_produced % 10 == 0) {
        total_rejected++; // [O18]
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Product #%d Processed -> REJECTED", total_produced);
    } else {
        total_accepted++; // [O17]
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Product #%d Processed -> ACCEPTED", total_produced);
    }

    // Write updated totals to the OPC UA variables
    UA_Variant val;
    UA_Variant_setScalar(&val, &total_produced, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2003), val); // TotalProduced
    
    UA_Variant_setScalar(&val, &total_accepted, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2004), val); // TotalAccepted
    
    UA_Variant_setScalar(&val, &total_rejected, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2005), val); // TotalRejected
}

void initialize_production_logic(UA_Server *server) {
    // 1. Add StartLine Method to ProductionLine01 (Node 1001) [O09]
    UA_MethodAttributes startAttr = UA_MethodAttributes_default;
    startAttr.displayName = UA_LOCALIZEDTEXT("en-US", "StartLine");
    startAttr.executable = true;
    startAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 4001),
                            UA_NODEID_NUMERIC(1, 1001),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "StartLine"),
                            startAttr, &start_line_method,
                            0, NULL, 0, NULL, NULL, NULL);

    // 2. Add StopLine Method to ProductionLine01 [O10]
    UA_MethodAttributes stopAttr = UA_MethodAttributes_default;
    stopAttr.displayName = UA_LOCALIZEDTEXT("en-US", "StopLine");
    stopAttr.executable = true;
    stopAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 4002),
                            UA_NODEID_NUMERIC(1, 1001),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "StopLine"),
                            stopAttr, &stop_line_method,
                            0, NULL, 0, NULL, NULL, NULL);

    // 3. Register the 5-second (5000ms) callback loop [O11]
    UA_UInt64 callbackId = 0;
    UA_Server_addRepeatedCallback(server, production_loop_callback, NULL, 5000, &callbackId);
}