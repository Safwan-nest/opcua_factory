#include "logic.h"
#include <open62541/plugin/log_stdout.h>

static UA_Boolean is_running = false;
static UA_Boolean in_fault = false;
static UA_Int32 active_fault_station = 0;
static UA_UInt32 cycle_ms = 5000;
static UA_Int32 total_produced = 0, total_accepted = 0, total_rejected = 0;

static void update_string(UA_Server *s, UA_UInt32 id, const char *txt) {
    UA_Variant v; UA_String str = UA_STRING((char *)txt);
    UA_Variant_setScalar(&v, &str, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

static void update_int32(UA_Server *s, UA_UInt32 id, UA_Int32 num) {
    UA_Variant v; UA_Variant_setScalar(&v, &num, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

static void update_double(UA_Server *s, UA_UInt32 id, UA_Double num) {
    UA_Variant v; UA_Variant_setScalar(&v, &num, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

static void update_bool(UA_Server *s, UA_UInt32 id, UA_Boolean flag) {
    UA_Variant v; UA_Variant_setScalar(&v, &flag, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

static UA_Int32 read_int32(UA_Server *s, UA_UInt32 id, UA_Int32 fallback) {
    UA_Variant out; UA_Variant_init(&out);
    UA_Int32 res = fallback;
    if (UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_INT32])) {
        res = *(UA_Int32 *)out.data;
    }
    UA_Variant_clear(&out);
    return res;
}

static void set_stations_state(UA_Server *s, const char *state) {
    update_string(s, 3001, state);
    update_string(s, 3101, state);
    update_string(s, 3201, state);
}

static void sync_counters(UA_Server *s) {
    update_int32(s, 2003, total_produced);
    update_int32(s, 2004, total_accepted);
    update_int32(s, 2005, total_rejected);
    update_int32(s, 3002, total_produced);
    update_int32(s, 3102, total_produced);
    update_int32(s, 3202, total_produced);
    update_int32(s, 3203, total_accepted);
    update_int32(s, 3204, total_rejected);
}

static void apply_station_fault(UA_Server *s, UA_Int32 code) {
    const char *msg = "Unknown Station Fault";
    set_stations_state(s, "STOPPED");
    if (code == 1)      { update_string(s, 3001, "FAULT"); msg = "FeedingStation Fault Injected"; }
    else if (code == 2) { update_string(s, 3101, "FAULT"); msg = "AssemblyStation Fault Injected"; }
    else if (code == 3) { update_string(s, 3201, "FAULT"); msg = "InspectionStation Fault Injected"; }

    in_fault = true;
    is_running = false;
    active_fault_station = code;
    update_string(s, 2001, "FAULT");
    update_double(s, 2006, 0.0);
    update_bool(s, 2007, true);
    update_string(s, 2008, msg);
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "FAULT TRIGGERED: %s", msg);
}

static UA_StatusCode start_line_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    UA_Int32 fault_inj = read_int32(s, 2009, 0);
    UA_Int32 target = read_int32(s, 2002, 100);
    if (in_fault || (fault_inj >= 1 && fault_inj <= 3) || target <= 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    if (total_produced >= target) {
        total_produced = total_accepted = total_rejected = 0;
        sync_counters(s);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Previous target reached; counters reset for new batch.");
    }

    is_running = true;
    update_string(s, 2001, "RUNNING");
    set_stations_state(s, "RUNNING");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Production Line STARTED (target=%d).", target);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode stop_line_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    is_running = false;
    update_double(s, 2006, 0.0);
    if (!in_fault) {
        update_string(s, 2001, "STOPPED");
        set_stations_state(s, "STOPPED");
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Production Line STOPPED.");
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode reset_fault_method(UA_Server *s, const UA_NodeId *sId, void *sH,
        const UA_NodeId *mId, void *mC, const UA_NodeId *oId, void *oC,
        size_t inS, const UA_Variant *in, size_t outS, UA_Variant *out) {
    UA_Int32 fault_inj = read_int32(s, 2009, 0);
    if (fault_inj != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Reset rejected: clear FaultInjection (%d) first.", fault_inj);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    in_fault = false;
    active_fault_station = 0;
    update_string(s, 2001, "STOPPED");
    set_stations_state(s, "STOPPED");
    update_string(s, 2008, "None");
    update_bool(s, 2007, false);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Fault RESET -> Line STOPPED.");
    return UA_STATUSCODE_GOOD;
}

static void fault_monitor_cb(UA_Server *s, void *data) {
    (void)data;
    UA_Int32 fault_inj = read_int32(s, 2009, 0);
    if (fault_inj < 0 || fault_inj > 3) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Invalid FaultInjection=%d rejected; resetting to 0.", fault_inj);
        update_int32(s, 2009, 0);
        return;
    }
    if (fault_inj >= 1 && fault_inj != active_fault_station) {
        apply_station_fault(s, fault_inj);
    }
}

static void production_loop_cb(UA_Server *s, void *data) {
    (void)data;
    if (!is_running || in_fault) {
        update_double(s, 2006, 0.0);
        return;
    }

    UA_Int32 target = read_int32(s, 2002, 100);
    if (target <= 0 || total_produced >= target) {
        is_running = false;
        update_string(s, 2001, "STOPPED");
        set_stations_state(s, "STOPPED");
        update_double(s, 2006, 0.0);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Production TARGET (%d) REACHED.", target);
        return;
    }

    total_produced++;
    UA_Boolean rejected = (total_produced % 10 == 0);
    if (rejected) total_rejected++; else total_accepted++;
    sync_counters(s);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Product #%d -> %s (Accepted=%d, Rejected=%d)",
                total_produced, rejected ? "REJECTED" : "ACCEPTED",
                total_accepted, total_rejected);

    if (total_produced >= target) {
        is_running = false;
        update_string(s, 2001, "STOPPED");
        set_stations_state(s, "STOPPED");
        update_double(s, 2006, 0.0);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Production TARGET (%d) REACHED.", target);
    } else {
        update_double(s, 2006, 60000.0 / (UA_Double)cycle_ms);
    }
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
    cycle_ms = cfg->cycle_interval_ms;
    add_method(server, 4001, "StartLine", &start_line_method);
    add_method(server, 4002, "StopLine", &stop_line_method);
    add_method(server, 4003, "ResetFault", &reset_fault_method);

    UA_UInt64 fId = 0, pId = 0;
    UA_Server_addRepeatedCallback(server, fault_monitor_cb, NULL, 1000, &fId);
    UA_Server_addRepeatedCallback(server, production_loop_cb, NULL, (UA_Double)cycle_ms, &pId);
}