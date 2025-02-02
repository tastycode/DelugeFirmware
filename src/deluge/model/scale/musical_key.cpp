#include "model/scale/musical_key.h"
#include <cstdint>

MusicalKey::MusicalKey() {
	modeNotes.add(0);
	rootNote = 0;
}

void MusicalKey::applyChanges(int8_t changes[12]) {
	modeNotes.applyChanges(changes);
	rootNote += changes[0];
}
