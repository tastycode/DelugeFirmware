#include "sysex.h"





size_t recv_size_packed;
uint32_t recv_binary_crc;
uint32_t recv_binary_retry_count = 0;

uint32_t recv_binary_received = 0;
uint32_t recv_binary_segs;
uint8_t recv_binary_chunk_current[BINARY_CHUNK_SIZE];



#ifdef ENV_DEV
uint8_t* defaultAlloc(int32_t size) {
	void* addr = malloc(size);
	return (uint8_t*)addr;
}
void send_recv_sanity_check() {
	init_crc_table();
	bool failed = false;
	uint8_t* recv_buff = malloc(1024);
	uint8_t* send_buff = malloc(1024);
	uint8_t* source_buff = malloc(1024);


	// test packing / unpacking at 32bit int
	uint32_t random_real = rand();
	pack_8bit_to_7bit(send_buff, 5, (uint8_t*)&random_real, 4);
	uint32_t random_unpacked;
	unpack_7bit_to_8bit((uint8_t*)&random_unpacked,4,  &send_buff[0], 5);
	if (random_unpacked != random_real) {
		failed = true;
		D_PRINTLN("failed packing/unpacking random number: %d, %d", random_unpacked, random_real);
	}

	int packed_size = packed_size_7(random_real);
	int unpacked_size = unpacked_size_8(packed_size);
	if (unpacked_size != random_real) {
		failed = true;
		D_PRINTLN("failed pack/unpack 8>7>8 size: expected %d, got %d", random_real, unpacked_size);
	}

	unpacked_size = unpacked_size_8(random_real);
	packed_size = packed_size_7(unpacked_size);
	if (packed_size != random_real) {
		failed = true;
		D_PRINTLN("failed pack/unpack 7>8>7 size: expected %d, got %d", random_real, unpacked_size);
	}

	// test packing / unpacking a sequence with crc
	for (int i = 0; i < 8 ; i++) {
		random_real = rand();
		memcpy(source_buff + i * 4, &random_real, 4);
	}
	uint32_t crc_real = get_crc(source_buff, 8 * 4);
	uint32_t crc_unpacked = 0;
	pack_8bit_to_7bit(send_buff, 5, (uint8_t*)&crc_real, 4);
	pack_8bit_to_7bit(send_buff + 5 , packed_size_7(8 * 4), (uint8_t*)source_buff, 8*4);
	unpack_7bit_to_8bit((uint8_t*)&crc_unpacked,4,  &send_buff[0], 5);
	unpacked_size = unpack_7bit_to_8bit(recv_buff + 0, 8 * 4, send_buff + 5, packed_size_7(8 * 4));
	if (unpacked_size == 0)  {
		D_PRINTLN("failed unpacking");
	}

	crc_unpacked = get_crc(recv_buff, 8 * 4);
	if (crc_unpacked != crc_real) {
		failed = true;
		D_PRINTLN("failed packing/unpacking crc: %d, %d", crc_unpacked, crc_real);
	}

	free(source_buff);
	free(recv_buff);
	free(send_buff);
	if (failed) {
		exit(1);
	}
}
#endif


int codebuff_size_unpacked(uint8_t* bytes, int32_t len) {
	uint32_t user_code_start = *(uint32_t*)(&bytes[OFF_USER_CODE_START]);
	uint32_t user_code_end = *(uint32_t*)(&bytes[OFF_USER_CODE_END]);
	uint32_t user_code_delta = user_code_end - user_code_start;

	D_PRINTLN("codebuff_size_unpacked: start %d, end %d, size %d", user_code_start, user_code_end, user_code_delta);
	return user_code_delta;
}

int msg_send_binary_init(uint8_t* data, uint32_t handshake, uint32_t size, uint32_t crc, uint32_t segs) {
	int packedMessageSize = 0;
	D_PRINTLN("msg_send_binary_init handshake = %d, size = %d, crc = %x, segs = %d", handshake, size, crc, segs);
	data[0] = 0xf0;
	data[1] = 0x7d;
	data[2] = 3; // handled by debug.cpp
	data[3] = 4; // prepare binary payload
	pack_8bit_to_7bit(&data[4], 5, (uint8_t*)&handshake, 4);
	pack_8bit_to_7bit(&data[9], 5, (uint8_t*)&size, 4);
	pack_8bit_to_7bit(&data[14], 5, (uint8_t*)&crc, 4);
	pack_8bit_to_7bit(&data[19], 5, (uint8_t*)&segs, 4);
	data[24] = 0xf7;
	packedMessageSize = 25;

#ifdef ENV_DEV
	bool failed = false;
	recv_binary_init(data, 25, handshake, false, defaultAlloc, free);
	if (recv_binary_handshake != handshake) {
		failed = true;
		D_PRINTLN("failed handshake: expected %d, got %d", handshake, recv_binary_handshake);
	}
	if (recv_size_unpacked != size) {
		failed = true;
		D_PRINTLN("failed size: expected %d, got %d", size, recv_size_unpacked);
	}
	if (recv_binary_crc != crc) {
		failed = true;
		D_PRINTLN("failed crc: expected %x, got %x", crc, recv_binary_crc);
	}
	if (recv_binary_segs != segs) {
		failed = true;
		D_PRINTLN("failed segs: expected %d, got %d", segs, recv_binary_segs);
	}
	if (failed) {
		exit(1);
	}
#endif


	return packedMessageSize;
}

int msg_send_request_validation(uint8_t* data, uint32_t handshake, uint32_t size, uint32_t crc) {
	D_PRINTLN("msg_request_validation");
	data[0] = 0xf0;
	data[1] = 0x7d;
	data[2] = 3; // debug
	data[3] = 2; // run
	// TODO: Litle-endian mandated
	pack_8bit_to_7bit(&data[4], 5, (uint8_t*)&handshake, 4);
	pack_8bit_to_7bit(&data[9], 5, (uint8_t*)&size, 4);
	pack_8bit_to_7bit(&data[14], 5, (uint8_t*)&crc, 4);
	data[19] = 0xf7;
	return 20;
}

/*
struct msg_field_descriptor {
	uint8_t bytes,
	uint32_t size
}

struct msg_buffer_container {
	uint8_t* bytes,
	uint32_t size
}
void msg_bytes(msg_container, msg_descriptor) {
	memcpy(&msg_container.bytes, msg_descriptor.size, msg_descriptor.bytes)
	 msg_container.size += msg_descriptor.size)
}

int msg_send_binary_chunk(uint8_t* data, uint32_t crc, uint32_t pos_low, uint32_t pos_high, uint8_t* bytes, uint32_t chunk_size, uint32_t seg) {
	D_PRINTLN("msg_binary_chunk");
	// data is msg_buffer_container, MSG_DEVICE_HEADER is msg_field_descriptor
	uint8_t* dataContainer = msg_bytes(msg_empty(), MSG_DEVICE_HEADER);
	msg_bytes(dataContainer, MSG_BinarY_SEND)
	msg_bytes(dataContainer, MSG_binARY_cHUNK_OUT)
	msg_bytes(dataContainer, msg_int32(crc)),
*/

int msg_send_binary_chunk(uint8_t* data, uint32_t crc, uint32_t pos_low, uint32_t pos_high, uint8_t* bytes, uint32_t seg) {
	D_PRINTLN("msg_binary_chunk pos_low = %d, pos_high = %d, seg = %d", pos_low, pos_high, seg);
	int packedMessageSize = 0;
	int packChunkSize = packed_size_7(BINARY_CHUNK_SIZE);
	data[0] = 0xf0;
	data[1] = 0x7d;
	data[2] = 3; // debug
	data[3] = 1; // send packet
	pack_8bit_to_7bit(data + 4, 5, (uint8_t*)&seg, 4);
	data[9] = pos_low;
	data[10] = pos_high;
	int packedChunkSize = pack_8bit_to_7bit(&data[11], packChunkSize, (uint8_t*)bytes, BINARY_CHUNK_SIZE);
	pack_8bit_to_7bit(&data[11 + packedChunkSize], 5, (uint8_t*)&crc, 4);
	data[11 + packedChunkSize + 5] = 0xf7;
	packedMessageSize = 16 + packedChunkSize + 1;
#ifdef ENV_DEV
	recv_binary_failed_seg_count = 0;
	recv_binary(data, packedMessageSize, false);
	if (recv_binary_failed_seg_count > 0) {
		D_PRINTLN("Failed recv_binary");
		exit(1);
	}
#endif
	return packedMessageSize;
}

void setConfig(int32_t codeStart, int32_t offUserCodeEnd) {

}

void send_binary(uint32_t handshake, unsigned char* buffer, int size, enum BINARY_TYPE binaryType, void (*sendPtr)(uint8_t*, size_t)) {
	D_PRINTLN("sending binary of size %d with handshake %x \n", size, handshake);
	init_crc_table();
	uint32_t segs = ((size + (BINARY_CHUNK_SIZE-1)) & (~(BINARY_CHUNK_SIZE-1))) >> 9; // align to closest 512
	uint32_t total_bytes = segs * BINARY_CHUNK_SIZE;
	uint32_t crc = get_crc(buffer, size);
	uint8_t data[BINARY_CHUNK_SIZE * 2];
	uint32_t resolved_size = binaryType == kBinaryUsercode ? codebuff_size_unpacked((uint8_t*)buffer, size) : size;
	D_PRINTLN("resolved_handshake: %x, total_bytes: %d, segs: %d, crc: %x, size: %d, resolved_size: %d ", handshake, total_bytes, segs, crc, size, resolved_size );
	int initSize = msg_send_binary_init(data, handshake, resolved_size, crc, segs);
	if (recv_binary_failed_seg_count == 0) {
		sendPtr(data, initSize);
	}  else {
		D_PRINTLN("sending failed segs: %d\n", recv_binary_failed_seg_count);
	}

	for (int seg = 0; seg < segs; seg += 1) {
		int resolved_seg = recv_binary_failed_seg_count > 0 ? recv_binary_failed_segs[seg] : seg;
		int pos_low = resolved_seg & 0x7f;
		int pos_high = (resolved_seg >> 7) & 0x7f;
		int buf_pos = BINARY_CHUNK_SIZE * resolved_seg;
		unsigned char* bytes = buffer + buf_pos;
		uint32_t crc = get_crc(bytes, BINARY_CHUNK_SIZE);

		int chunkSize = msg_send_binary_chunk(data, crc, pos_low, pos_high, (uint8_t*)bytes, resolved_seg);
		if (recv_binary_failed_seg_count > 0 && seg > recv_binary_failed_seg_count) {
			break;
		}
		sendPtr(data, chunkSize);
	}
	int validateSize = msg_send_request_validation(data, handshake, size, crc);
	sendPtr(data, validateSize);
}


/* will deprecate recv_binary_first since its tightly coupled to only firmware transfer */
void recv_binary_init(uint8_t* data, int32_t len, uint32_t handshake, bool shouldValidateHandshake, uint8_t* (*allocPtr)(int32_t), void (*freePtr)(void*)) {
	D_PRINTLN("recv_binary_init: len=%d, handshake=%d, shouldValidateHandshake=%d", len,handshake,shouldValidateHandshake);
	if (len < 20) {
		D_PRINTLN("recv_binary_init: len < 20");
	}

	printHex("recv_binary_init", data, len);

	memset(recv_binary_failed_segs, 0, recv_binary_failed_seg_count);
	recv_size_unpacked = 0;
	recv_size_packed = 0;
	recv_binary_received = 0;
	recv_binary_failed_seg_count = 0;
	recv_binary_segs = 0;

	unpack_7bit_to_8bit((uint8_t*)&recv_binary_handshake,4,  &data[4], 5);
	unpack_7bit_to_8bit((uint8_t*)&recv_size_unpacked, 4, &data[9], 5);
	unpack_7bit_to_8bit((uint8_t*)&recv_binary_crc,4,  &data[14] , 5);
	unpack_7bit_to_8bit((uint8_t*)&recv_binary_segs,4,  &data[19], 5);
	printHex("recv_binary_init:recv_size_unpacked", &data[9], 5);
	printHex("recv_binary_init:recv_binary_segs", &data[19], 5);
	D_PRINTLN("recv_size_unpacked = %d, recv_binary_segs = %d", recv_size_unpacked, recv_binary_segs);


	if (recv_binary_buf != NULL) {
		D_PRINTLN("recv_binary_init: freeing old buffer at %p", recv_binary_buf);
		freePtr(recv_binary_buf);
	}

	if (!shouldValidateHandshake || handshake == recv_binary_handshake) {
		D_PRINTLN("recv_binary_init: handshake %x, size %d, crc %x", recv_binary_handshake, recv_size_unpacked, recv_binary_crc);

		recv_size_packed = recv_size_unpacked + ((BINARY_CHUNK_SIZE - 1) - ((recv_size_unpacked - 1) & (BINARY_CHUNK_SIZE - 1)));

		D_PRINTLN("recv_binary_init: handshake %x, unpacked %d, packed %d, crc %x, allocPtr %p", recv_binary_handshake, recv_size_unpacked, recv_size_packed, recv_binary_crc, allocPtr);
		recv_binary_buf = (uint8_t*)allocPtr(recv_size_unpacked);
		if (recv_binary_buf == NULL) {
			// fail :(
			D_PRINTLN("recv_binary_init: unable to allocate %d bytes", recv_size_unpacked);
		}
		D_PRINTLN("Allocated %d bytes for binary buffer at %p", recv_size_unpacked, recv_binary_buf);
	} else {
		D_PRINTLN("recv_binary_init: handshake received %x does not match stored %x", recv_binary_handshake, handshake);
	}
}

// invoked many times as chunked binary packets arrive
void recv_binary(uint8_t* data, int32_t len, bool shouldCopyDecoded) {
	memset(recv_binary_chunk_current, 0, BINARY_CHUNK_SIZE);
	const int packed_size = len - 17;
	bool failed = false;
	uint32_t msg_crc;
	uint32_t msg_seg;
	unpack_7bit_to_8bit((uint8_t*)&msg_seg,4,  &data[4], 5);
	// calculate position of pos based on segment
	uint8_t pos_low = data[9];
	uint8_t pos_high = data[10];
	int pos = BINARY_CHUNK_SIZE * (pos_low + 0x80 * pos_high);
	D_PRINTLN("recv_binary: len %d, recv_binary_received %d/%d, msg_unpacked_length %d, packed_size %d, handshake: %d", len, recv_binary_received,recv_binary_segs, packed_size, recv_binary_handshake);
	unpack_7bit_to_8bit(recv_binary_chunk_current, BINARY_CHUNK_SIZE, &data[11], packed_size_7(BINARY_CHUNK_SIZE));
	uint32_t msg_real_crc = get_crc(recv_binary_chunk_current, BINARY_CHUNK_SIZE);
	unpack_7bit_to_8bit((uint8_t*)&msg_crc,4,  data + 11 + packed_size, 5);
	recv_binary_received = recv_binary_received + 1;
	if (packed_size != BINARY_PACKED_SIZE) {
		D_PRINTLN("recv_binary: packed_size %d != BINARY_PACKED_SIZE %d", packed_size, BINARY_PACKED_SIZE);
		failed = true;
	}
	if (msg_seg != (recv_binary_received - 1)) {
		D_PRINTLN("recv_binary: seg mismatch: msg_seg %x, expected %x", msg_seg, recv_binary_received);
		failed = true;
	}
	if (msg_real_crc != msg_crc) {
		D_PRINTLN("recv_binary: CRC mismatch: msg_real_crc %x, msg_crc %x", msg_real_crc, msg_crc);
		failed = true;
	}
	if (failed) {
		recv_binary_failed_seg_count+=1;
		recv_binary_failed_segs[recv_binary_failed_seg_count] = recv_binary_received;
	} else {
		if (shouldCopyDecoded) {
			memcpy(&recv_binary_buf[pos], recv_binary_chunk_current, BINARY_CHUNK_SIZE);
		}
	}
}

void reply_failed_segs(void (*sendPtr)(uint8_t*, size_t)) {
	D_PRINTLN("reply_failed_segs: recv_binary_failed_seg_count = %d", recv_binary_failed_seg_count);
	int failed_seg_size = recv_binary_failed_seg_count * 5;
	int header_size = 5;
	uint8_t data[header_size + failed_seg_size];
	data[0] = 0xf0;
	data[1] = 0x7d;
	data[2] = 3; // debug
	data[3] = 6; // run
	data[4] = (uint8_t)recv_binary_failed_seg_count;
	for (int i = 0; i < recv_binary_failed_seg_count; i++) {
		D_PRINTLN("reply_failed_segs: failed seg %d = %d", i, recv_binary_failed_segs[i]);
		pack_8bit_to_7bit(&data[5 + (i*5)], 5, (uint8_t*)(recv_binary_failed_segs + i),4);
	}
	data[5 + failed_seg_size] = 0xf7;
	printHex("reply_failed_segs", data, 5 + failed_seg_size + 1);
	sendPtr(data, 5 + failed_seg_size + 1);
}

enum BinaryValidationResult recv_binary_validate(uint8_t* data, int32_t len, uint32_t handshake, bool shouldValidateHandshake, void (*sendPtr)(uint8_t*, size_t)) {
	if (len < 20) {
		D_PRINTLN("recv_binary_validate/GENERIC_FAILED: len %d, recv_binary_buf %p", len, recv_binary_buf);
		return GENERIC_FAILED; // cannot do that
	}

	uint32_t msg_handshake;
	uint32_t msg_size_unpacked;
	uint32_t msg_crc;
	unpack_7bit_to_8bit((uint8_t*)&msg_handshake,4,  data + 4, 5);
	unpack_7bit_to_8bit((uint8_t*)&msg_size_unpacked,4,  data + 9, 5);
	unpack_7bit_to_8bit((uint8_t*)&msg_crc,4,  data + 14, 5);

	for (int i = 0; i < recv_binary_failed_seg_count; i++) {
		uint32_t seg = recv_binary_failed_segs[i];
		D_PRINTLN("recv_binary_validate/CRC_FAILED: seg %d", seg);
	}
	uint32_t crc = get_crc((unsigned char*)recv_binary_buf, recv_size_unpacked);
	if ((crc != msg_crc || msg_crc != recv_binary_crc) && recv_binary_failed_seg_count > 0) {
		D_PRINTLN("recv_binary_validate/CRC_FAILED: checksum %x, recv_binary_crc %x, msg_crc %x", crc, recv_binary_crc, msg_crc);
		if (recv_binary_retry_count < MAXIMUM_RETRIES && recv_binary_failed_seg_count < 16) {
			reply_failed_segs(sendPtr);
			recv_binary_retry_count+=1;
		} else {
			return CRC_FAILED;
		}
	}
	D_PRINTLN("recv_binary_validate/recv_binary_received=%d, fails=%x", recv_binary_received, recv_binary_failed_seg_count);
	if (recv_binary_received != recv_binary_segs) {
		D_PRINTLN("recv_binary_validate/SIZE_FAILED: received %d, recv_binary_segs %d", recv_binary_received, recv_binary_segs);
		return SIZE_FAILED;
	}
	if (shouldValidateHandshake) {
		if (handshake != msg_handshake || msg_handshake != recv_binary_handshake) {
			D_PRINTLN("recv_binary_validate/HANDSHAKE_FAILED: handshake %d, recv_binary_handshake %d, msg_handshake %d", handshake, recv_binary_handshake, msg_handshake);
			return HANDSHAKE_FAILED;
		}
	}

	return VALID;
}

void recv_binary_dump_reset(uint8_t* data, int32_t len) {
	memcpy(data, recv_binary_buf, recv_size_unpacked);
	recv_binary_buf = NULL;
	recv_size_packed = 0;
	recv_size_unpacked = 0;
}

