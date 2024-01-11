#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>     // For uint8_t and other fixed-width integer types
#include <stddef.h>     // For size_t
#include <stdio.h>
#include "definitions.h"
// Function declarations
#ifdef __cplusplus
extern "C" {
#endif
void printHex(const char* prefix, uint8_t* data, int size);
#ifdef __cplusplus
}
#endif

#endif // PRINT_H
