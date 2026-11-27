#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <open62541/server.h>

#include "config.h"
#include "hierarchy.h"
#include "variables.h"
#include "ua_helpers.h"
#include "engine.h"

static UA_Server *mk_srv(AppConfig *cfg, EngineState *st) {
    load_app_config("__missing_w2_cfg__.ini", cfg);
    UA_Server *s = UA_Server_new();
    build_factory_hierarchy(s);
    add_production_variables(s, cfg);
    engine_init(st, cfg);
    return s;
}

static int str_eq(UA_Server *s, UA_UInt32 id, const char *exp) {
    UA_Variant out;
    UA_Variant_init(&out);
    int ok = 0;
    if (UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_STRING])) {
        UA_String *v = (UA_String *)out.data;
        ok = (v->length == strlen(exp) && memcmp(v->data, exp, v->length) == 0);
    }
    UA_Variant_clear(&out);
    return ok;
}

static UA_Boolean rd_bool(UA_Server *s, UA_UInt32 id) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_Boolean val = false;
    if (UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        val = *(UA_Boolean *)out.data;
    }
    UA_Variant_clear(&out);
    return val;
}

static void t01_config_defaults(void) {
    AppConfig c;
    load_app_config("__missing_w2_cfg__.ini", &c);
    assert(c.conveyor_speed == 1.0 && c.cycle_time_sec == 1.0);
    assert(c.assembly_housing_capacity == 10 && c.inspection_cycle_time_sec == 1.0);
    assert(c.inspection_housing_capacity == 10);
}

static void t02_read_double_fallback(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    assert(read_double(s, 3003, 9.9) == 1.0);
    assert(read_double(s, 9999, 4.2) == 4.2);
    UA_Server_delete(s);
}

static void t03_start_stop_states(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    assert(str_eq(s, 2001, "RUNNING") && str_eq(s, 3001, "RUNNING"));
    assert(engine_stop(s, &st) == UA_STATUSCODE_GOOD);
    assert(str_eq(s, 2001, "STOPPED") && read_double(s, 2006, -1.0) == 0.0);
    UA_Server_delete(s);
}

static void t04_reject_invalid_target(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2002, 0);
    assert(engine_start(s, &st) == UA_STATUSCODE_BADINVALIDSTATE);
    update_int32(s, 2002, -10);
    assert(engine_start(s, &st) == UA_STATUSCODE_BADINVALIDSTATE);
    UA_Server_delete(s);
}

static void t05_balanced_pipeline_rate(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2002, 5);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 1.0);
    assert(st.feed_received == 1 && st.asm_processed == 1 && st.total_produced == 1);
    assert(read_double(s, 2006, 0.0) > 59.9 && read_double(s, 2006, 0.0) < 60.1);
    assert(str_eq(s, 2010, "None") && str_eq(s, 3004, "None"));
    UA_Server_delete(s);
}

static void t06_target_completion_and_reject(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2002, 10);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    for (int i = 0; i < 10; i++) engine_step(s, &st, 1.0);
    assert(st.total_produced == 10 && st.total_accepted == 9 && st.total_rejected == 1);
    assert(!st.is_running && str_eq(s, 2001, "STOPPED") && read_double(s, 2006, -1.0) == 0.0);
    UA_Server_delete(s);
}

static void t07_auto_reset_new_batch(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2002, 2);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 1.0); engine_step(s, &st, 1.0);
    assert(st.total_produced == 2 && !st.is_running);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    assert(st.total_produced == 0 && st.is_running);
    UA_Server_delete(s);
}

static void t08_reset_counters_guards(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 1.0);
    assert(engine_reset_counters(s, &st) == UA_STATUSCODE_BADINVALIDSTATE);
    assert(engine_stop(s, &st) == UA_STATUSCODE_GOOD);
    assert(engine_reset_counters(s, &st) == UA_STATUSCODE_GOOD);
    assert(st.total_produced == 0 && read_int32(s, 2003, -1) == 0);
    UA_Server_delete(s);
}

static void t09_feed_slow_warning(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_double(s, 3003, 1.0); update_double(s, 3103, 0.5); update_double(s, 3205, 1.0);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(str_eq(s, 2010, "Inefficient: Delivery rate too slow for Assembly"));
    assert(str_eq(s, 3004, "Inefficient: Increase delivery rate"));
    assert(str_eq(s, 3106, "Starved: Conveyor delivery rate too slow"));
    assert(str_eq(s, 3208, "None"));
    UA_Server_delete(s);
}

static void t10_asm_slow_warning(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_double(s, 3003, 2.0); update_double(s, 3103, 1.0); update_double(s, 3205, 0.5);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(str_eq(s, 2010, "Inefficient: Assembly too slow for Inspection"));
    assert(str_eq(s, 3106, "Inefficient: Decrease Assembly cycle time"));
    assert(str_eq(s, 3208, "Starved: Assembly too slow"));
    UA_Server_delete(s);
}

static void t11_both_slow_warning(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_double(s, 3003, 0.5); update_double(s, 3103, 1.0); update_double(s, 3205, 0.5);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(str_eq(s, 2010, "Inefficient: Increase delivery rate & assembly speed"));
    assert(str_eq(s, 3106, "Starved by Feed & slow for Insp"));
    UA_Server_delete(s);
}

static void t12_assembly_overload_fault(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_double(s, 3003, 10.0); update_double(s, 3103, 2.0); update_int32(s, 3104, 3);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(st.in_fault && st.active_fault_station == 2 && rd_bool(s, 2007) == true);
    assert(str_eq(s, 3101, "FAULT") && str_eq(s, 3001, "STOPPED") && read_int32(s, 3105, -1) == 3);
    assert(str_eq(s, 2008, "AssemblyStation Overload: housing full, conveyor too fast"));
    UA_Server_delete(s);
}

static void t13_inspection_overload_fault(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_double(s, 3003, 10.0); update_double(s, 3103, 0.1);
    update_double(s, 3205, 2.0); update_int32(s, 3104, 20); update_int32(s, 3206, 3);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(st.in_fault && st.active_fault_station == 3 && rd_bool(s, 2007) == true);
    assert(str_eq(s, 3201, "FAULT") && read_int32(s, 3207, -1) == 3);
    assert(str_eq(s, 2008, "InspectionStation Overload: housing full, assembler too fast"));
    UA_Server_delete(s);
}

static void t14_overload_recovery_no_stall(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2002, 5); update_double(s, 3003, 10.0);
    update_double(s, 3103, 2.0); update_int32(s, 3104, 2);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(st.in_fault && engine_reset_fault(s, &st) == UA_STATUSCODE_GOOD);
    assert(st.asm_housing == 0 && !st.in_fault && rd_bool(s, 2007) == false);
    update_double(s, 3003, 1.0); update_double(s, 3103, 1.0);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    for (int i = 0; i < 6; i++) engine_step(s, &st, 1.0);
    assert(st.total_produced == 5 && !st.is_running);
    UA_Server_delete(s);
}

static void t15_manual_fault_lock_and_clear(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2009, 1); engine_apply_fault(s, &st, 1);
    assert(str_eq(s, 3001, "FAULT") && engine_start(s, &st) == UA_STATUSCODE_BADINVALIDSTATE);
    assert(engine_reset_fault(s, &st) == UA_STATUSCODE_BADINVALIDSTATE);
    update_int32(s, 2009, 0);
    assert(engine_reset_fault(s, &st) == UA_STATUSCODE_GOOD && str_eq(s, 2001, "STOPPED"));
    UA_Server_delete(s);
}

static void t16_manual_fault_messages(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    engine_apply_fault(s, &st, 2); assert(str_eq(s, 2008, "AssemblyStation Fault"));
    engine_apply_fault(s, &st, 3); assert(str_eq(s, 2008, "InspectionStation Fault"));
    engine_apply_fault(s, &st, 9); assert(str_eq(s, 2008, "Unknown Station Fault"));
    UA_Server_delete(s);
}

static void t17_boundary_clamping(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_double(s, 3003, -5.0); update_double(s, 3103, 0.0); update_double(s, 3205, -1.0);
    update_int32(s, 3104, 0); update_int32(s, 3206, -2);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5);
    assert(read_double(s, 2006, 0.0) > 5.9 && read_double(s, 2006, 0.0) < 6.1);
    UA_Server_delete(s);
}

static void t18_feeding_idle_while_draining(void) {
    AppConfig c; EngineState st; UA_Server *s = mk_srv(&c, &st);
    update_int32(s, 2002, 2); update_double(s, 3003, 4.0);
    assert(engine_start(s, &st) == UA_STATUSCODE_GOOD);
    engine_step(s, &st, 0.5); engine_step(s, &st, 0.5);
    assert(str_eq(s, 3001, "IDLE") && st.is_running);
    UA_Server_delete(s);
}

int main(void) {
    void (*tests[])(void) = {
        t01_config_defaults, t02_read_double_fallback, t03_start_stop_states,
        t04_reject_invalid_target, t05_balanced_pipeline_rate, t06_target_completion_and_reject,
        t07_auto_reset_new_batch, t08_reset_counters_guards, t09_feed_slow_warning,
        t10_asm_slow_warning, t11_both_slow_warning, t12_assembly_overload_fault,
        t13_inspection_overload_fault, t14_overload_recovery_no_stall,
        t15_manual_fault_lock_and_clear, t16_manual_fault_messages,
        t17_boundary_clamping, t18_feeding_idle_while_draining
    };
    for (int i = 0; i < 18; i++) { tests[i](); printf("[PASS] Week2 Test %02d/18\n", i + 1); }
    return 0;
}