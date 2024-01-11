#include "syxsrv.h"

#define MAX_MESSAGE_SIZE 624
#define END_SYSEX 0xF7
#define FILE_PATH "output.syx"
#define DEFAULT_HANDSHAKE 8575309


void _putchar(char character) {
	putchar(character);
}
time_t current_xmit_start = 0;
int resolvedHandshake = DEFAULT_HANDSHAKE;

time_t current_time_sec() {
	return time(NULL);
}

uint8_t* allocSharedMaxSpeed(int32_t size) {
	void* addr = malloc(size);
	return (uint8_t*)addr;
}

void freeShared(void* addr) {
	free(addr);
}


void sendSysex(uint8_t* data, size_t len) {
	D_PRINTLN("sendsysex: %d", len);
}

void handle_incoming_sysex(char* buffer, int32_t len) {
	uint8_t *data = (uint8_t*)buffer;
	switch (data[3]) {
		case 1:
			if (current_xmit_start == 0) {
				D_PRINTLN("received in-process message before recv_binary_init");
			} else {
				current_xmit_start = current_time_sec();
				recv_binary(data, len, true);
			}
			break;
		case 2:
			// todo
			if (current_xmit_start == 0) {
				D_PRINTLN("received validate without init");
			} else {
				recv_binary_validate(data, len, resolvedHandshake, 0, sendSysex);
			}
			break;
		case 3:
			D_PRINTLN("case 3:");
			// todo
			break;
		case 4:
			D_PRINTLN("recv_binary_init");
			if (current_xmit_start == 0) {
				current_xmit_start = current_time_sec();
				recv_binary_init(data, len, resolvedHandshake, 0, allocSharedMaxSpeed, freeShared);
			} else {
				D_PRINTLN("received recv_binary_init before finalizing previous message");
			}
			break;
		default:
			// todo
			break;
	}
}
void handle_message(uint8_t *buffer, int length) {
    handle_incoming_sysex(buffer, length);  // Defined in sysex.c
}


int main(int argc, char **argv) {
	init_crc_table();
	send_recv_sanity_check();
    FILE *file;
    uint8_t buff[MAX_MESSAGE_SIZE];
    int bytes_received = 0;
    int byte;

	if (argv[1] && strlen(argv[1]) > 0) {
		D_PRINTLN("Using handshake: %s", argv[1]);
		resolvedHandshake = atoi(argv[1]);
	}

    // Open the SYX file
    file = fopen(FILE_PATH, "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    while ((byte = fgetc(file)) != EOF) {
        if (bytes_received > MAX_MESSAGE_SIZE) {
            fprintf(stderr, "Warning: Maximum message size exceeded %d>%d. Message discarded. \n", bytes_received, MAX_MESSAGE_SIZE);
            bytes_received = 0;  // Reset for the next message
        }

        buff[bytes_received++] = (uint8_t)byte;

        if (byte == END_SYSEX) {
            // Process complete message
            handle_message(buff, bytes_received);
            bytes_received = 0;  // Reset for the next message
        }
    }

    fclose(file);
    return 0;
}
