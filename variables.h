#ifndef VARIABLES_H
#define VARIABLES_H

#include <open62541/server.h>
#include "config.h"

void add_production_variables(UA_Server *server, const AppConfig *cfg);

#endif