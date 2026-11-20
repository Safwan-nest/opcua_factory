#ifndef LOGIC_H
#define LOGIC_H

#include <open62541/server.h>
#include "config.h"

void initialize_production_logic(UA_Server *server, const AppConfig *cfg);

#endif