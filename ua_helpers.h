#ifndef UA_HELPERS_H
#define UA_HELPERS_H

#include <open62541/server.h>

void update_string(UA_Server *s, UA_UInt32 id, const char *txt);
void update_int32(UA_Server *s, UA_UInt32 id, UA_Int32 num);
void update_double(UA_Server *s, UA_UInt32 id, UA_Double num);
void update_bool(UA_Server *s, UA_UInt32 id, UA_Boolean flag);
UA_Int32 read_int32(UA_Server *s, UA_UInt32 id, UA_Int32 fallback);
UA_Double read_double(UA_Server *s, UA_UInt32 id, UA_Double fallback);

#endif
