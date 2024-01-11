#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cprint.h"
#include "definitions.h"
#include "util/pack.h"
#include "extern.h"

#define HANDSHAKE_SIZE 4
#define BINARY_CHUNK_SIZE  512
#define BINARY_PACKED_SIZE  586 // ceil((double)BINARY_CHUNK_SIZE + BINARY_CHUNK_SIZE / 7.0
#define VOID_HANDSHAKE 1111
#define MAXIMUM_RETRIES 3

#ifdef __cplusplus
extern "C" {
#endif

enum BinaryValidationResult {
	GENERIC_FAILED,
	HANDSHAKE_FAILED,
	SIZE_FAILED,
	CRC_FAILED,
	VALID
};

static uint32_t recv_binary_failed_segs[16];
static uint32_t recv_binary_failed_seg_count;
static size_t recv_size_unpacked;
static uint8_t* recv_binary_buf;
static uint32_t recv_binary_handshake = 0;

#ifdef __cplusplus
extern "C" {
#endif

enum BINARY_TYPE {
	kBinaryUsercode,
	kBinaryAnytype
};


__attribute__((visibility("default"))) void setConfig(int32_t codeStart, int32_t offUserCodeEnd);
#ifdef ENV_DEV
void send_recv_sanity_check();
#endif
void send_binary(uint32_t handshake, unsigned char* buffer, int size, enum BINARY_TYPE binaryType, void (*sendPtr)(uint8_t*, size_t));
void recv_binary_init(uint8_t* data, int32_t len, uint32_t handshake, bool shouldValidateHandshake, uint8_t* (*allocPtr)(int32_t), void (*freePtr)(void*));
int msg_send_request_validation(uint8_t* data, uint32_t handshake, uint32_t size, uint32_t crc);
int recv_binary_first(uint8_t* data, int32_t len, uint8_t* (*allocPtr)(int32_t), void (*freePtr)(void*));
void recv_binary_dump_reset(uint8_t* data, int32_t len);
void recv_binary(uint8_t* data, int32_t len, bool shouldCopyDecoded);
enum BinaryValidationResult recv_binary_validate(uint8_t* data, int32_t len, uint32_t handshake, bool shouldValidateHandshake, void (*sendPtr)(uint8_t*, size_t));

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
}
#endif
