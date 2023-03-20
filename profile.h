#pragma once
#include <assert.h>
#include <hdr/hdr_histogram.h>
#include <stdio.h>

#include "cpu_cycles.h"
// #include "memcached.h"

#define MAX_LAT_US 1000

static struct hdr_histogram *hist_lat_cycle[conn_max_state];

void init_hdr_histogram(void);
void close_hdr_histogram(void);
void profile_output(void);

void init_hdr_histogram(void) {
    puts("init_hdr_histogram");
    for (int i = 0; i < conn_max_state; i++) {
        hdr_init(1, MAX_LAT_US * RMGRID_CPU_FREQ, 1, &hist_lat_cycle[i]);
        assert(hist_lat_cycle[i]);
    }
}

void close_hdr_histogram(void) {
    for (int i = 0; i < conn_max_state; i++) {
        hdr_close(hist_lat_cycle[i]);
    }
}

#define START_PROFILE(STATE)                                      \
    int profile_state = STATE;                                    \
    assert(profile_state >= 0 && profile_state < conn_max_state); \
    uint64_t profile_start_cycle = get_cycles_start();

#define END_RPOFILE                                        \
    uint64_t profile_end_cycle = get_cycles_end();         \
    hdr_record_value_atomic(hist_lat_cycle[profile_state], \
                            profile_end_cycle - profile_start_cycle);

void profile_output(void) {
    static const char *output_file_name[] = {
        "/tmp/conn_listening.log", "/tmp/conn_new_cmd.log",
        "/tmp/conn_waiting.log",   "/tmp/conn_read.log",
        "/tmp/conn_parse_cmd.log", "/tmp/conn_write.log",
        "/tmp/conn_nread.log",     "/tmp/conn_swallow.log",
        "/tmp/conn_closing.log",   "/tmp/conn_mwrite.log",
        "/tmp/conn_closed.log",    "/tmp/conn_watch.log",
        "/tmp/conn_io_queue.log",
    };
    for (int i = 0; i < conn_max_state; i++) {
        FILE *file = fopen(output_file_name[i], "w");
        if (!file) {
            printf("Failed to open file %s\n", output_file_name[i]);
            exit(-1);
        }
        assert(hist_lat_cycle[i]);
        hdr_percentiles_print(hist_lat_cycle[i], file, 100, RMGRID_CPU_FREQ,
                              CLASSIC);
        fclose(file);
    }
    puts("Output to /tmp/conn_*.log");
}

