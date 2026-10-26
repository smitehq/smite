#ifndef CORE_CONFETTI_H
#define CORE_CONFETTI_H

#include <ostream>

namespace smite {

// Short in-place burst for a successful quest. Returns after a handful of
// frames (well under half a second) or sooner if the player is already
// typing. Does nothing when stdout is not a terminal.
void play_quest_confetti(std::ostream& out);

} // namespace smite

#endif // CORE_CONFETTI_H
