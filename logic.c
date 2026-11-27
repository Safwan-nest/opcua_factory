#include "logic.h"
#include "engine.h"
#include "ua_helpers.h"
#include <open62541/plugin/log_stdout.h>

static EngineState g_engine;

static UA_StatusCode start_line_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    return engine_start(s, &g_engine);
}

static UA_StatusCode stop_line_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    return engine_stop(s, &g_engine);
}

static UA_StatusCode reset_fault_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    return engine_reset_fault(s, &g_engine);
}

static UA_StatusCode reset_counters_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    return engine_reset_counters(s, &g_engine);
}

static void fault_monitor_cb(UA_Server *s, void *data) {
    (void)data;
    UA_Int32 fault_inj = read_int32(s, 2009, 0);
    if (fault_inj < 0 || fault_inj > 3) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Invalid FaultInjection=%d; resetting to 0.", fault_inj);
        update_int32(s, 2009, 0);
        return;
    }
    if (fault_inj >= 1 && fault_inj != g_engine.active_fault_station) {
        engine_apply_fault(s, &g_engine, fault_inj);
    }
}

static void production_loop_cb(UA_Server *s, void *data) {
    (void)data;
    engine_step(s, &g_engine, 0.5);
}

static void add_method(UA_Server *s, UA_UInt32 mId, char *name, UA_MethodCallback cb) {
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.executable = true;
    attr.userExecutable = true;
    UA_Server_addMethodNode(s, UA_NODEID_NUMERIC(1, mId), UA_NODEID_NUMERIC(1, 1001),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, name),
                            attr, cb, 0, NULL, 0, NULL, NULL, NULL);
}

void initialize_production_logic(UA_Server *server, const AppConfig *cfg) {
    engine_init(&g_engine, cfg);
    add_method(server, 4001, "StartLine", &start_line_method);
    add_method(server, 4002, "StopLine", &stop_line_method);
    add_method(server, 4003, "ResetFault", &reset_fault_method);
    add_method(server, 4004, "ResetCounters", reset_counters_method);

    UA_UInt64 fId = 0, pId = 0;
    UA_Server_addRepeatedCallback(server, fault_monitor_cb, NULL, 1000, &fId);
    UA_Server_addRepeatedCallback(server, production_loop_cb, NULL, 500, &pId);
}