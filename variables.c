#include "variables.h"
#include <stdlib.h>

static void add_int32_var(UA_Server *server, int parentNode, int varNode, char *name, UA_Int32 initVal) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &initVal, &UA_TYPES[UA_TYPES_INT32]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, varNode),
                              UA_NODEID_NUMERIC(1, parentNode),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, name),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);
}

static void add_string_var(UA_Server *server, int parentNode, int varNode, char *name, char *initVal) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_String strVal = UA_STRING(initVal);
    UA_Variant_setScalar(&attr.value, &strVal, &UA_TYPES[UA_TYPES_STRING]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, varNode),
                              UA_NODEID_NUMERIC(1, parentNode),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, name),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);
}

void add_production_variables(UA_Server *server) {
    // Inject ProductionLine01 counters
    add_string_var(server, 1001, 2001, "LineState", "STOPPED"); 
    add_int32_var(server, 1001, 2002, "ProductionTarget", 100); 
    add_int32_var(server, 1001, 2003, "TotalProduced", 0);
    add_int32_var(server, 1001, 2004, "TotalAccepted", 0);
    add_int32_var(server, 1001, 2005, "TotalRejected", 0);

    // Inject FeedingStation variables
    add_string_var(server, 1002, 3001, "State", "STOPPED");
    add_int32_var(server, 1002, 3002, "ProductsReceived", 0);
}