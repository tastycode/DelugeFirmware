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

#pragma once

#include <stdint.h>
#include "definitions_cxx.hpp"
#include "util/cprint.h"
#include "gui/l10n/l10n.h"
#include "hid/display/oled.h"
#include "io/debug/print.h"
#include "io/midi/midi_device.h"
#include "io/midi/midi_engine.h"
#include "memory/general_memory_allocator.h"
#include "model/settings/runtime_feature_settings.h"
#include "util/chainload.h"
#include "util/functions.h"
#include "util/pack.h"
#include "model/song/song.h"
#include "extern.h"
#include "util/sysex.h"


// #include <cstdint>

class MIDIDevice;


namespace Debug {

void sysexReceived(MIDIDevice* device, uint8_t* data, int32_t len);
void sysexDebugPrint(const char* msg, bool nl);
void sysexSongSend(MIDIDevice* device);

#ifdef ENABLE_SYSEX_LOAD
void loadPacketReceived(uint8_t* data, int32_t len);
void loadCheckAndRun(uint8_t* data, int32_t len);
#endif

} // namespace Debug
