#ifndef SYXPACK_H
#define SYXPACK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <argp.h>
#include "lib/printf.h"
#include "util/pack.h"

// Structure to hold command line arguments
struct arguments {
    char *args[1];      // Input data
    bool unpack;        // -u flag
    bool binary;        // -b flag
    bool input_decimal; // -id flag
    bool output_decimal; // -od flag
};

// Function prototypes
int32_t pack_8bit_to_7bit(uint8_t* dst, int32_t dst_size, uint8_t* src, int32_t src_len);
int32_t unpack_7bit_to_8bit(uint8_t* dst, int32_t dst_size, uint8_t* src, int32_t src_len);
int32_t packed_size_7(int32_t numBytes);
int32_t unpacked_size_8(int32_t packedSizeBytes);

#endif // SYXPACK_H
