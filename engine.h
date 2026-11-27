#ifndef ENGINE_H
#define ENGINE_H

#include <open62541/server.h>
#include "config.h"

typedef struct {
    UA_Boolean is_running;
    UA_Boolean in_fault;
    UA_Int32 active_fault_station;
    UA_Int32 feed_received;
    UA_Int32 asm_processed;
    UA_Int32 asm_housing;
    UA_Int32 insp_housing;
    UA_Int32 total_produced;
    UA_Int32 total_accepted;
    UA_Int32 total_rejected;
    double feed_acc;
    double asm_acc;
    double insp_acc;
} EngineState;

void engine_init(EngineState *st, const AppConfig *cfg);

void engine_apply_fault(UA_Server *s, EngineState *st, UA_Int32 code);
UA_StatusCode engine_start(UA_Server *s, EngineState *st);
UA_StatusCode engine_stop(UA_Server *s, EngineState *st);
UA_StatusCode engine_reset_fault(UA_Server *s, EngineState *st);
UA_StatusCode engine_reset_counters(UA_Server *s, EngineState *st);
void engine_step(UA_Server *s, EngineState *st, double dt_sec);

#endif