#ifndef LOGIC_H
#define LOGIC_H

#include <open62541/server.h>

// Initializes the start/stop methods and the 5-second timer
void initialize_production_logic(UA_Server *server);

#endif