#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <open62541/server.h>

#include "config.h"
#include "hierarchy.h"
#include "variables.h"
#include "logic.h"

static UA_StatusCode call_method(UA_Server *s, UA_UInt32 methodId) {
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = UA_NODEID_NUMERIC(1, 1001);
    req.methodId = UA_NODEID_NUMERIC(1, methodId);
    UA_CallMethodResult res = UA_Server_call(s, &req);
    UA_StatusCode sc = res.statusCode;
    UA_CallMethodResult_clear(&res);
    return sc;
}

static void write_i32(UA_Server *s, UA_UInt32 id, UA_Int32 val) {
    UA_Variant v;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    assert(UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v) == UA_STATUSCODE_GOOD);
}

static UA_Int32 read_i32(UA_Server *s, UA_UInt32 id) {
    UA_Variant out;
    UA_Variant_init(&out);
    assert(UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD);
    assert(UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_INT32]));
    UA_Int32 val = *(UA_Int32 *)out.data;
    UA_Variant_clear(&out);
    return val;
}

static UA_Boolean read_bool(UA_Server *s, UA_UInt32 id) {
    UA_Variant out;
    UA_Variant_init(&out);
    assert(UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD);
    assert(UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_BOOLEAN]));
    UA_Boolean val = *(UA_Boolean *)out.data;
    UA_Variant_clear(&out);
    return val;
}

static UA_Double read_dbl(UA_Server *s, UA_UInt32 id) {
    UA_Variant out;
    UA_Variant_init(&out);
    assert(UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD);
    assert(UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_DOUBLE]));
    UA_Double val = *(UA_Double *)out.data;
    UA_Variant_clear(&out);
    return val;
}

static void assert_str(UA_Server *s, UA_UInt32 id, const char *expected) {
    UA_Variant out;
    UA_Variant_init(&out);
    assert(UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD);
    assert(UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_STRING]));
    UA_String *str = (UA_String *)out.data;
    assert(str->length == strlen(expected));
    assert(memcmp(str->data, expected, str->length) == 0);
    UA_Variant_clear(&out);
}

static void write_temp_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    assert(fp != NULL);
    fputs(content, fp);
    fclose(fp);
}

static void test_config_suite(void) {
    AppConfig cfg;

    /* Test 1: Missing config file falls back to safe defaults */
    load_app_config("/tmp/does_not_exist_opcua.ini", &cfg);
    assert(strcmp(cfg.hostname, "127.0.0.1") == 0 && cfg.port == 4840);
    assert(cfg.cycle_interval_ms == 5000 && cfg.default_target == 100);
    printf("[PASS] 01/12 Config missing file uses defaults\n");

    /* Test 2: Valid config file parses all fields */
    write_temp_file("/tmp/test_valid.ini",
        "hostname=10.0.2.15\nport=4841\ncycle_interval_ms=2000\n"
        "default_target=25\nconveyor_speed=2.0\ncycle_time_sec=4.5\n");
    load_app_config("/tmp/test_valid.ini", &cfg);
    assert(strcmp(cfg.hostname, "10.0.2.15") == 0 && cfg.port == 4841);
    assert(cfg.cycle_interval_ms == 2000 && cfg.default_target == 25);
    printf("[PASS] 02/12 Config valid file parsed correctly\n");

    /* Test 3 (Negative): Out-of-range config values rejected */
    write_temp_file("/tmp/test_bad.ini",
        "port=99999\ncycle_interval_ms=50\ndefault_target=-10\nconveyor_speed=-1.0\n");
    load_app_config("/tmp/test_bad.ini", &cfg);
    assert(cfg.port == 4840 && cfg.cycle_interval_ms == 5000 && cfg.default_target == 100);
    printf("[PASS] 03/12 Config invalid/negative values rejected\n");

    /* Test 4 (Boundary): Comments and blank lines ignored */
    write_temp_file("/tmp/test_comments.ini", "\n# header comment\ndefault_target=20\n\n");
    load_app_config("/tmp/test_comments.ini", &cfg);
    assert(cfg.default_target == 20 && cfg.port == 4840);
    printf("[PASS] 04/12 Config comments and blank lines ignored\n");
}

int main(void) {
    test_config_suite();

    AppConfig cfg;
    load_app_config("/tmp/does_not_exist_opcua.ini", &cfg);
    UA_Server *s = UA_Server_new();
    build_factory_hierarchy(s);
    add_production_variables(s, &cfg);
    initialize_production_logic(s, &cfg);

    /* Test 5: All 5 hierarchy objects exist */
    UA_UInt32 objs[5] = {1000, 1001, 1002, 1003, 1004};
    for (int i = 0; i < 5; i++) {
        UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
        assert(UA_Server_readNodeClass(s, UA_NODEID_NUMERIC(1, objs[i]), &nc) == UA_STATUSCODE_GOOD);
        assert(nc == UA_NODECLASS_OBJECT);
    }
    printf("[PASS] 05/12 Factory01, ProductionLine01, and 3 stations exist\n");

    /* Test 6: Initial line and station states are STOPPED (O08) */
    assert_str(s, 2001, "STOPPED");
    assert_str(s, 3001, "STOPPED");
    assert_str(s, 3101, "STOPPED");
    assert_str(s, 3201, "STOPPED");
    assert(read_i32(s, 2002) == 100 && read_i32(s, 2003) == 0 && read_bool(s, 2007) == false);
    printf("[PASS] 06/12 Initial states STOPPED, counters 0, alarm false\n");

    /* Test 7: StartLine sets line and all stations to RUNNING (O09) */
    assert(call_method(s, 4001) == UA_STATUSCODE_GOOD);
    assert_str(s, 2001, "RUNNING");
    assert_str(s, 3001, "RUNNING");
    assert_str(s, 3101, "RUNNING");
    assert_str(s, 3201, "RUNNING");
    printf("[PASS] 07/12 StartLine starts line and all 3 stations\n");

    /* Test 8: StopLine returns line and all stations to STOPPED (O10) */
    assert(call_method(s, 4002) == UA_STATUSCODE_GOOD);
    assert_str(s, 2001, "STOPPED");
    assert_str(s, 3001, "STOPPED");
    assert_str(s, 3101, "STOPPED");
    assert_str(s, 3201, "STOPPED");
    assert(read_dbl(s, 2006) == 0.0);
    printf("[PASS] 08/12 StopLine stops line, all stations, and rate=0\n");

    /* Test 9 (Negative): StartLine rejected while FaultInjection=2 active */
    write_i32(s, 2009, 2);
    assert(call_method(s, 4001) == UA_STATUSCODE_BADINVALIDSTATE);
    printf("[PASS] 09/12 StartLine blocked when FaultInjection is active\n");

    /* Test 10 (Negative): ResetFault rejected while FaultInjection!=0 (O24) */
    assert(call_method(s, 4003) == UA_STATUSCODE_BADINVALIDSTATE);
    printf("[PASS] 10/12 ResetFault blocked until FaultInjection is cleared\n");

    /* Test 11: ResetFault succeeds once FaultInjection cleared to 0 */
    write_i32(s, 2009, 0);
    assert(call_method(s, 4003) == UA_STATUSCODE_GOOD);
    assert_str(s, 2001, "STOPPED");
    assert_str(s, 2008, "None");
    assert(read_bool(s, 2007) == false);
    printf("[PASS] 11/12 ResetFault clears fault and restores STOPPED\n");

    /* Test 12 (Boundary): ProductionTarget=0 blocks StartLine */
    write_i32(s, 2002, 0);
    assert(call_method(s, 4001) == UA_STATUSCODE_BADINVALIDSTATE);
    printf("[PASS] 12/12 Boundary target=0 rejects StartLine\n");

    UA_Server_delete(s);
    printf("ALL 12 WEEK-1 TESTS PASSED.\n");
    return 0;
}