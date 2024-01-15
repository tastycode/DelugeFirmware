/*
 * Copyright © 2015-2023 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "io/debug/sysex.h"


static MIDIDevice* receivingDevice;

uint8_t* allocSharedMaxSpeed(int32_t size) {
	void* addr = GeneralMemoryAllocator::get().allocMaxSpeed(size);
	return static_cast<uint8_t*>(addr);
}


void sendSysex(uint8_t* data, size_t len) {
	D_PRINTLN("sendSysex: %d", len);
	if (receivingDevice == nullptr) {
		D_PRINTLN("sendSysex: no receiving device");
	} else {
		receivingDevice->sendSysex(data, len);
	}
}


void Debug::sysexReceived(MIDIDevice* device, uint8_t* data, int32_t len){
	if (len < 6) {
		return;
	}

	// first three bytes are already used, next is command
	switch (data[3]) {
	case 0:
		if (data[4] == 1) {
			midiDebugDevice = device;
		}
		else if (data[4] == 0) {
			midiDebugDevice = nullptr;
		}
		break;

	case 1:
	{
#ifdef ENABLE_SYSEX_LOAD
		recv_binary(data, len, true);
#endif
		break;
	}
	case 2:
	{
#ifdef ENABLE_SYSEX_LOAD
		uint32_t handshake = (uint32_t)runtimeFeatureSettings.get(RuntimeFeatureSettingType::DevSysexAllowed);
		BinaryValidationResult result = recv_binary_validate(data, len, handshake, true, sendSysex) ;
		switch (result) {
			case BinaryValidationResult::GENERIC_FAILED:
				display->displayPopup("general failed");
			break;
			case BinaryValidationResult::HANDSHAKE_FAILED:
				display->displayPopup("handshake failed");
			break;
			case BinaryValidationResult::SIZE_FAILED:
				display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_WRONG_SIZE));
			break;
			case BinaryValidationResult::CRC_FAILED:
				display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_CHECKSUM_FAIL));
			break;
			case BinaryValidationResult::VALID:
				display->displayPopup("general ok");
				chainload_from_buf(recv_binary_buf, recv_size_unpacked);
			break;
		}
#endif
			break;
		}
		case 3:
		{
			receivingDevice = device;
			display->displayPopup("about to sysex song send");
			sysexSongSend(device);
			break;
		}
		case 4:
		{
			D_PRINTLN("recv_binary_init followsc");
			uint32_t handshake = (uint32_t)runtimeFeatureSettings.get(RuntimeFeatureSettingType::DevSysexAllowed);
			recv_binary_init(data, len, handshake, true, allocSharedMaxSpeed, delugeDealloc);
			break;
		}
	}
}

void Debug::sysexDebugPrint(const char* msg, bool nl) {

	if (!midiDebugDevice || midiDebugDevice == nullptr || !SYSEX_LOGGING_ENABLED || !msg) {
		return;
	}
	uint8_t reply_hdr[5] = {0xf0, 0x7d, 0x03, 0x40, 0x00};
	uint8_t* reply = midiEngine.sysex_fmt_buffer;
	memcpy(reply, reply_hdr, 5);
	size_t len = strlen(msg);
	len = std::min(len, sizeof(midiEngine.sysex_fmt_buffer) - 7);
	memcpy(reply + 5, msg, len);
	for (int32_t i = 0; i < len; i++) {
		reply[5 + i] &= 0x7F; // only ascii debug messages
	}

	if (nl) {
		reply[5 + len] = '\n';
		len++;
	}

	reply[5 + len] = 0xf7;

	if (midiDebugDevice != nullptr) { // rare race condition
		midiDebugDevice->sendSysex(reply, 5 + len + 1);
	}
}


void applyMask(unsigned long long *buffer, const int c, const int o) {
    unsigned long long result = 0;
    for (int i = 0; i < 8; ++i) {
        unsigned long long temp = (*buffer >> (c * i)) & 0x7F;  // Extract 7 bits
        result |= (temp << (o * i));  // Shift and combine
    }
    *buffer = result;
}

void encodeBuffered(uint8_t *output, const uint8_t *input, int size, int fromSize, int toSize) {
    int c = fromSize;
    int o = toSize;

    for (int i = 0; i < size; i += c) {
        unsigned long long buffer = 0;
        int readSize = (size - i < c) ? size - i : c;  // Calculate the size to read

        // Copy data into the buffer
        memcpy(&buffer, input + i, readSize);

        // Apply the mask
        applyMask(&buffer, c, o);

        // Copy data to output, ensuring we don't exceed the bounds
        int writeSize = (size - i < o) ? size - i : o;
        memcpy(output, &buffer, writeSize);
        output += writeSize;
    }
}
int encodedSize(int size, int fromBits, int toBits) {
    return (int)ceil((double)size * fromBits / toBits);
}

void Debug::sysexSongSend(MIDIDevice* device) {
	// len 1449
	D_PRINTLN("sysexSongSend");
	display->displayPopup("sysexSongSend");
	String* xml = currentSong->toXML();
	const char* charXml = xml->get();
	int32_t len = xml->getLength();

	display->displayPopup("sendingBinary");
	receivingDevice = device;
	send_binary(VOID_HANDSHAKE, (unsigned char*)charXml, len, kBinaryAnytype, sendSysex);
	display->displayPopup("sent");

	/*
	free(xmlBytes);
	free(xml);
	*/
}


#ifdef ENABLE_SYSEX_LOAD
static size_t load_bufsize;
static uint8_t* load_buf;
static size_t load_codesize;


static void firstPacket(uint8_t* data, int32_t len) {
	D_PRINTLN("sysex.cpp/firstPacket");
	uint8_t tmpbuf[0x40] __attribute__((aligned(CACHE_LINE_SIZE)));
	unpack_7bit_to_8bit(tmpbuf, 0x40, data + 11, 0x4a);
	uint32_t user_code_start = *(uint32_t*)(tmpbuf + OFF_USER_CODE_START);
	uint32_t user_code_end = *(uint32_t*)(tmpbuf + OFF_USER_CODE_END);
	load_codesize = (int32_t)(user_code_end - user_code_start);
	if (load_bufsize < load_codesize) {
		if (load_buf != nullptr) {
			delugeDealloc(load_buf);
		}
		load_bufsize = load_codesize + (511 - ((load_codesize - 1) & 511));

		load_buf = (uint8_t*)GeneralMemoryAllocator::get().allocMaxSpeed(load_bufsize);
		if (load_buf == nullptr) {
			// fail :(
			return;
		}
	}
}

void Debug::loadPacketReceived(uint8_t* data, int32_t len) {
	uint32_t handshake = runtimeFeatureSettings.get(RuntimeFeatureSettingType::DevSysexAllowed);
	if (handshake == 0) {
		return; // not allowed
	}

	const int size = 512;
	const int packed_size = 586; // ceil(512+512/7)
	if (len < packed_size + 12) {
		return;
	}

	uint32_t handshake_received;
	unpack_7bit_to_8bit((uint8_t*)&handshake_received, 4, data + 4, 5);
	if (handshake != handshake_received) {
		return;
	}

	int pos = 512 * (data[9] + 0x80 * data[10]);

	if (pos == 0) {
		firstPacket(data, len);
	}

	if (load_buf == nullptr || pos + 512 > load_bufsize) {
		return;
	}

	unpack_7bit_to_8bit(load_buf + pos, size, data + 11, packed_size);
}

void Debug::loadCheckAndRun(uint8_t* data, int32_t len) {
	uint32_t handshake = runtimeFeatureSettings.get(RuntimeFeatureSettingType::DevSysexAllowed);
	if (handshake == 0) {
		return; // not allowed
	}

	if (len < 19 || load_buf == nullptr) {
		return; // cannot do that
	}

	uint32_t fields[3];
	unpack_7bit_to_8bit((uint8_t*)fields, sizeof(fields), data + 4, 14);

	if (handshake != fields[0]) {
		return;
	}

	if (load_codesize != fields[1]) {
		display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_WRONG_SIZE));
		return;
	}

	uint32_t checksum = get_crc(load_buf, load_codesize);

	if (checksum != fields[2]) {
		display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_CHECKSUM_FAIL));
		return;
	}
	chainload_from_buf(load_buf, load_codesize);
}
#endif
