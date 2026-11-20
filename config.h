#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

typedef struct {
    char hostname[64];
    uint16_t port;
    uint32_t cycle_interval_ms;
    int32_t default_target;
    double conveyor_speed;
    double cycle_time_sec;
} AppConfig;

void load_app_config(const char *filepath, AppConfig *cfg);

#endif