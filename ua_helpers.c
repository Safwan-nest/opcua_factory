#include "ua_helpers.h"

void update_string(UA_Server *s, UA_UInt32 id, const char *txt) {
    UA_String str = UA_STRING_ALLOC(txt);
    UA_Variant v;
    UA_Variant_setScalar(&v, &str, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
    UA_String_clear(&str);
}

void update_int32(UA_Server *s, UA_UInt32 id, UA_Int32 num) {
    UA_Variant v;
    UA_Variant_setScalar(&v, &num, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

void update_double(UA_Server *s, UA_UInt32 id, UA_Double num) {
    UA_Variant v;
    UA_Variant_setScalar(&v, &num, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

void update_bool(UA_Server *s, UA_UInt32 id, UA_Boolean flag) {
    UA_Variant v;
    UA_Variant_setScalar(&v, &flag, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(s, UA_NODEID_NUMERIC(1, id), v);
}

UA_Int32 read_int32(UA_Server *s, UA_UInt32 id, UA_Int32 fallback) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_Int32 val = fallback;
    if (UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_INT32])) {
        val = *(UA_Int32 *)out.data;
    }
    UA_Variant_clear(&out);
    return val;
}

UA_Double read_double(UA_Server *s, UA_UInt32 id, UA_Double fallback) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_Double val = fallback;
    if (UA_Server_readValue(s, UA_NODEID_NUMERIC(1, id), &out) == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_DOUBLE])) {
        val = *(UA_Double *)out.data;
    }
    UA_Variant_clear(&out);
    return val;
}
