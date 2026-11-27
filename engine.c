#include "engine.h"
#include "ua_helpers.h"
#include <open62541/plugin/log_stdout.h>

static double clamp_dbl(double v, double lo) { return v < lo ? lo : v; }
static UA_Int32 clamp_i32(UA_Int32 v, UA_Int32 lo) { return v < lo ? lo : v; }

static void set_stations(UA_Server *s, const char *st) {
    update_string(s, 3001, st);
    update_string(s, 3101, st);
    update_string(s, 3201, st);
}

static void clear_warnings(UA_Server *s) {
    update_string(s, 2010, "None");
    update_string(s, 3004, "None");
    update_string(s, 3106, "None");
    update_string(s, 3208, "None");
    
}

static void sync_counters(UA_Server *s, const EngineState *st) {
    update_int32(s, 2003, st->total_produced);
    update_int32(s, 2004, st->total_accepted);
    update_int32(s, 2005, st->total_rejected);
    update_int32(s, 3002, st->feed_received);
    update_int32(s, 3102, st->asm_processed);
    update_int32(s, 3105, st->asm_housing);
    update_int32(s, 3202, st->total_produced);
    update_int32(s, 3203, st->total_accepted);
    update_int32(s, 3204, st->total_rejected);
    update_int32(s, 3207, st->insp_housing);
}

static void reset_batch(EngineState *st) {
    st->feed_received = st->asm_processed = 0;
    st->asm_housing = st->insp_housing = 0;
    st->total_produced = st->total_accepted = st->total_rejected = 0;
    st->feed_acc = st->asm_acc = st->insp_acc = 0.0;
}

static void trigger_fault(UA_Server *s, EngineState *st, UA_Int32 code, const char *msg) {
    set_stations(s, "STOPPED");
    if (code == 1) update_string(s, 3001, "FAULT");
    else if (code == 2) update_string(s, 3101, "FAULT");
    else if (code == 3) update_string(s, 3201, "FAULT");
    st->in_fault = true;
    st->is_running = false;
    st->active_fault_station = code;
    st->feed_acc = st->asm_acc = st->insp_acc = 0.0;
    update_string(s, 2001, "FAULT");
    update_double(s, 2006, 0.0);
    update_bool(s, 2007, true);
    update_string(s, 2008, msg);
    clear_warnings(s);
    sync_counters(s, st);
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "FAULT: %s", msg);
}

void engine_init(EngineState *st, const AppConfig *cfg) {
    st->is_running = false;
    st->in_fault = false;
    st->active_fault_station = 0;
    reset_batch(st);
    (void)cfg;
}

void engine_apply_fault(UA_Server *s, EngineState *st, UA_Int32 code) {
    const char *msg = "Unknown Station Fault";
    if (code == 1) msg = "FeedingStation Fault";
    else if (code == 2) msg = "AssemblyStation Fault";
    else if (code == 3) msg = "InspectionStation Fault";
    trigger_fault(s, st, code, msg);
}

UA_StatusCode engine_start(UA_Server *s, EngineState *st) {
    UA_Int32 fault_inj = read_int32(s, 2009, 0);
    UA_Int32 target = read_int32(s, 2002, 100);
    if (st->in_fault || (fault_inj >= 1 && fault_inj <= 3) || target <= 0)
        return UA_STATUSCODE_BADINVALIDSTATE;
    if (st->total_produced >= target) { reset_batch(st); sync_counters(s, st); }
    st->is_running = true;
    update_string(s, 2001, "RUNNING");
    set_stations(s, "RUNNING");
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode engine_stop(UA_Server *s, EngineState *st) {
    st->is_running = false;
    st->feed_acc = st->asm_acc = st->insp_acc = 0.0;
    update_double(s, 2006, 0.0);
    clear_warnings(s);
    if (!st->in_fault) { update_string(s, 2001, "STOPPED"); set_stations(s, "STOPPED"); }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode engine_reset_fault(UA_Server *s, EngineState *st) {
    if (read_int32(s, 2009, 0) != 0) return UA_STATUSCODE_BADINVALIDSTATE;
    if (st->active_fault_station == 2) st->asm_housing = 0;
    if (st->active_fault_station == 3) st->insp_housing = 0;
    st->in_fault = false;
    st->active_fault_station = 0;
    st->feed_acc = st->asm_acc = st->insp_acc = 0.0;
    update_string(s, 2001, "STOPPED");
    set_stations(s, "STOPPED");
    update_string(s, 2008, "None");
    clear_warnings(s);
    update_bool(s, 2007, false);
    sync_counters(s, st);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode engine_reset_counters(UA_Server *s, EngineState *st) {
    if (st->is_running) return UA_STATUSCODE_BADINVALIDSTATE;
    reset_batch(st);
    sync_counters(s, st);
    update_double(s, 2006, 0.0);
    clear_warnings(s);
    return UA_STATUSCODE_GOOD;
}

static UA_Int32 advance_queue(double *acc, double rate, double dt, UA_Int32 *src_q) {
    if (*src_q <= 0) { *acc = 0.0; return 0; }
    *acc += rate * dt;
    UA_Int32 done = (UA_Int32)(*acc);
    if (done > *src_q) { done = *src_q; *acc = 0.0; }
    else *acc -= (double)done;
    *src_q -= done;
    return done;
}

static void update_warnings(UA_Server *s, double feed_r, double asm_r, double insp_r) {
    UA_Boolean feed_slow = (asm_r > feed_r + 1e-6);
    UA_Boolean asm_slow = (insp_r > asm_r + 1e-6);
    update_string(s, 3004, feed_slow ? "Inefficient: Increase delivery rate" : "None");
    update_string(s, 3106, feed_slow && asm_slow ? "Starved by Feed & slow for Insp"
        : (feed_slow ? "Starved: Conveyor delivery rate too slow"
        : (asm_slow ? "Inefficient: Decrease Assembly cycle time" : "None")));
    update_string(s, 3208, asm_slow ? "Starved: Assembly too slow" : "None");
    if (feed_slow && asm_slow)
        update_string(s, 2010, "Inefficient: Increase delivery rate & assembly speed");
    else if (feed_slow)
        update_string(s, 2010, "Inefficient: Delivery rate too slow for Assembly");
    else if (asm_slow)
        update_string(s, 2010, "Inefficient: Assembly too slow for Inspection");
    else
        update_string(s, 2010, "None");
}

void engine_step(UA_Server *s, EngineState *st, double dt_sec) {
    if (!st->is_running || st->in_fault) { update_double(s, 2006, 0.0); return; }
    UA_Int32 target = read_int32(s, 2002, 100);
    if (target <= 0 || st->total_produced >= target) { engine_stop(s, st); return; }

    double feed_r = clamp_dbl(read_double(s, 3003, 1.0), 0.1);
    double asm_r  = 1.0 / clamp_dbl(read_double(s, 3103, 1.0), 0.1);
    double insp_r = 1.0 / clamp_dbl(read_double(s, 3205, 1.0), 0.1);
    UA_Int32 asm_cap  = clamp_i32(read_int32(s, 3104, 10), 1);
    UA_Int32 insp_cap = clamp_i32(read_int32(s, 3206, 10), 1);
    st->asm_housing   = clamp_i32(read_int32(s, 3105, st->asm_housing), 0);
    st->insp_housing  = clamp_i32(read_int32(s, 3207, st->insp_housing), 0);
    

    UA_Int32 in_flight = st->total_produced + st->asm_housing + st->insp_housing;
    UA_Int32 rem = (target > in_flight) ? (target - in_flight) : 0;
    update_string(s, 3001, rem > 0 ? "RUNNING" : "IDLE");
    update_string(s, 3101, (rem > 0 || st->asm_housing > 0) ? "RUNNING" : "IDLE");
    UA_Int32 fed = advance_queue(&st->feed_acc, feed_r, dt_sec, &rem);
    st->feed_received += fed;
    st->asm_housing += fed;

    UA_Int32 assembled = advance_queue(&st->asm_acc, asm_r, dt_sec, &st->asm_housing);
    st->asm_processed += assembled;
    st->insp_housing += assembled;

    UA_Int32 inspected = advance_queue(&st->insp_acc, insp_r, dt_sec, &st->insp_housing);
    for (UA_Int32 i = 0; i < inspected; i++) {
        st->total_produced++;
        if (st->total_produced % 10 == 0) st->total_rejected++;
        else st->total_accepted++;
    }

    if (st->asm_housing > asm_cap) {
        st->asm_housing = asm_cap;
        trigger_fault(s, st, 2, "AssemblyStation Overload: housing full, conveyor too fast");
        return;
    }
    if (st->insp_housing > insp_cap) {
        st->insp_housing = insp_cap;
        trigger_fault(s, st, 3, "InspectionStation Overload: housing full, assembler too fast");
        return;
    }

    sync_counters(s, st);
    update_warnings(s, feed_r, asm_r, insp_r);
    if (st->total_produced >= target) { engine_stop(s, st); return; }

    double bottleneck = feed_r < asm_r ? feed_r : asm_r;
    if (insp_r < bottleneck) bottleneck = insp_r;
    update_double(s, 2006, bottleneck * 60.0);
}