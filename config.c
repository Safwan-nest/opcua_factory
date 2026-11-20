#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_app_config(const char *filepath, AppConfig *cfg) {
    // Safe fallback defaults if config.ini is missing or incomplete
    strncpy(cfg->hostname, "127.0.0.1", sizeof(cfg->hostname) - 1);
    cfg->hostname[sizeof(cfg->hostname) - 1] = '\0';
    cfg->port = 4840;
    cfg->cycle_interval_ms = 5000;
    cfg->default_target = 100;
    cfg->conveyor_speed = 1.5;
    cfg->cycle_time_sec = 5.0;

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "[CONFIG] '%s' not found; using defaults.\n", filepath);
        return;
    }

    char line[256], key[64], val[64];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;
        if (sscanf(line, " %63[^= ] = %63s", key, val) != 2) continue;

        if (strcmp(key, "hostname") == 0) {
            strncpy(cfg->hostname, val, sizeof(cfg->hostname) - 1);
            cfg->hostname[sizeof(cfg->hostname) - 1] = '\0';
        } else if (strcmp(key, "port") == 0) {
            int p = atoi(val);
            if (p > 0 && p <= 65535) cfg->port = (uint16_t)p;
        } else if (strcmp(key, "cycle_interval_ms") == 0) {
            int ms = atoi(val);
            if (ms >= 500) cfg->cycle_interval_ms = (uint32_t)ms;
        } else if (strcmp(key, "default_target") == 0) {
            int t = atoi(val);
            if (t > 0) cfg->default_target = t;
        } else if (strcmp(key, "conveyor_speed") == 0) {
            double s = atof(val);
            if (s > 0.0) cfg->conveyor_speed = s;
        } else if (strcmp(key, "cycle_time_sec") == 0) {
            double c = atof(val);
            if (c > 0.0) cfg->cycle_time_sec = c;
        }
    }
    fclose(fp);
}