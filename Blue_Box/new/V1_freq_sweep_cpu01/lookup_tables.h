// mux_lookup_table.h
#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#define NUM_CONFIGS 192

// Structure to hold the lookup table configuration
typedef struct {
    unsigned short exc_out;
    unsigned short sns_ap_out;
    unsigned short sns_an_out;
    unsigned short ret_out;
} MuxConfig;

// Declare the lookup table
extern const MuxConfig tomo_mux_lookup_table[NUM_CONFIGS];

#endif // MUX_LOOKUP_TABLE_H
