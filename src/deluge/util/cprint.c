#include "./cprint.h"

void byteToBinary(uint8_t byte, char *binaryStr) {
    for (int i = 7; i >= 0; i--) {
        binaryStr[7 - i] = (byte & (1 << i)) ? '1' : '0';
    }
    binaryStr[8] = '\0'; // Null-terminate the string
}

// print xxd-like hex dump
void printHex(const char* prefix, uint8_t* data, int size) {
    const int bytesPerLine = 16;

    for (int i = 0; i < size; i++) {
        // Print the address at the start of each line
        if (i % bytesPerLine == 0) {
            if (i > 0) {
                D_PRINT_RAW("\n");  // Newline for previous line
            }
            D_PRINT("%s: %04x: ", prefix, i);  // Print the address/offset
        }

        D_PRINT_RAW("%02X ", data[i]);  // Print the current byte in hex

        // Optional: Add extra spacing after 8 bytes for readability
        if (i % bytesPerLine == 7) {
            D_PRINT_RAW(" ");
        }
    }

    D_PRINT_RAW("\n");  // Newline at the end
}
