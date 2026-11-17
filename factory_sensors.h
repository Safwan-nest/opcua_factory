#ifndef FACTORY_SENSORS_H
#define FACTORY_SENSORS_H

#include <open62541/server.h>

// Function declarations
void define_object_and_variables(UA_Server *server);
void update_sensor_data(UA_Server *server, void *data);

#endif
