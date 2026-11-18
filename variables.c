#include "variables.h"
#include <stdlib.h>

static void add_int32_var(UA_Server *server, int parentNode, int varNode, char *name, UA_Int32 initVal) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &initVal, &UA_TYPES[UA_TYPES_INT32]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, varNode), UA_NODEID_NUMERIC(1, parentNode),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, name),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void add_double_var(UA_Server *server, int parentNode, int varNode, char *name, UA_Double initVal) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &initVal, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, varNode), UA_NODEID_NUMERIC(1, parentNode),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, name),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void add_bool_var(UA_Server *server, int parentNode, int varNode, char *name, UA_Boolean initVal) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &initVal, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, varNode), UA_NODEID_NUMERIC(1, parentNode),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, name),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void add_string_var(UA_Server *server, int parentNode, int varNode, char *name, char *initVal) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_String strVal = UA_STRING(initVal);
    UA_Variant_setScalar(&attr.value, &strVal, &UA_TYPES[UA_TYPES_STRING]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, varNode), UA_NODEID_NUMERIC(1, parentNode),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, name),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

void add_production_variables(UA_Server *server) {
    // ProductionLine01 (1001)
    add_string_var(server, 1001, 2001, "LineState", "STOPPED"); 
    add_int32_var(server, 1001, 2002, "ProductionTarget", 100); 
    add_int32_var(server, 1001, 2003, "TotalProduced", 0);
    add_int32_var(server, 1001, 2004, "TotalAccepted", 0);
    add_int32_var(server, 1001, 2005, "TotalRejected", 0);
    add_double_var(server, 1001, 2006, "ProductionRate", 0.0);
    add_bool_var(server, 1001, 2007, "AlarmActive", false);
    add_string_var(server, 1001, 2008, "FaultMessage", "None");
    add_int32_var(server, 1001, 2009, "FaultInjection", 0);

    // FeedingStation (1002)
    add_string_var(server, 1002, 3001, "State", "STOPPED");
    add_int32_var(server, 1002, 3002, "ProductsReceived", 0);
    add_double_var(server, 1002, 3003, "ConveyorSpeed", 1.5);

    // AssemblyStation (1003)
    add_string_var(server, 1003, 3101, "State", "STOPPED");
    add_int32_var(server, 1003, 3102, "ProductsProcessed", 0);
    add_double_var(server, 1003, 3103, "CycleTime", 5.0);

    // InspectionStation (1004)
    add_string_var(server, 1004, 3201, "State", "STOPPED");
    add_int32_var(server, 1004, 3202, "ProductsInspected", 0);
    add_int32_var(server, 1004, 3203, "ProductsAccepted", 0);
    add_int32_var(server, 1004, 3204, "ProductsRejected", 0);
}