#ifndef MIDIPORT_H
#define MIDIPORT_H

#include <QString>

namespace qlogue {

/// One MIDI port as reported by `logue-cli probe -l`.
struct MidiPort {
    enum Direction { In, Out };
    Direction direction = In;
    int       index     = -1;
    QString   name;    // e.g. "minilogue xd:minilogue xd _ SOUND 28:1"
};

} // namespace qlogue

#endif // MIDIPORT_H
