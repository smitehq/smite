#include "confetti.h"

#include <chrono>
#if defined(_WIN32)
#include <thread>
#endif

#if defined(_WIN32)
#include <io.h>
#include <stdio.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

namespace smite {
namespace {

constexpr int kFrames = 6;
constexpr int kRows = 2;
constexpr int kCols = 42;
constexpr auto kFrameDelay = std::chrono::milliseconds(40);

const char* kGlyphs = "*+.ox";
const char* kColors[] = {
    "\x1b[1;32m", // green, same family as the prompt
    "\x1b[1;33m", // yellow, same family as headers
    "\x1b[1;36m", // cyan, same family as info
    "\x1b[1;35m",
    "\x1b[1;31m",
};

bool stdout_is_tty() {
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

// Density rises and falls so the burst pops instead of sitting at full noise.
int frame_density(int frame) {
    static constexpr int kDensity[] = {1, 2, 3, 3, 2, 1};
    if (frame < 0 || frame >= kFrames) return 1;
    return kDensity[frame];
}

void write_frame(std::ostream& out, int frame) {
    const int density = frame_density(frame);
    for (int row = 0; row < kRows; ++row) {
        out << "  ";
        for (int col = 0; col < kCols; ++col) {
            unsigned mix = static_cast<unsigned>(col * 13 + row * 29 + frame * 17 + 3);
            if ((mix % 8) >= static_cast<unsigned>(density)) {
                out << ' ';
                continue;
            }
            const char* color = kColors[mix % 5];
            char glyph = kGlyphs[(mix / 5) % 5];
            out << color << glyph << "\x1b[0m";
        }
        out << '\n';
    }
}

// True when the player has already typed the next line. The byte stays
// in the kernel buffer so readline (or the telemetry prompt) still sees it.
bool stdin_has_data() {
#if defined(_WIN32)
    return false;
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{};
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
#endif
}

void wait_frame() {
#if defined(_WIN32)
    std::this_thread::sleep_for(kFrameDelay);
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{};
    tv.tv_usec = static_cast<long>(kFrameDelay.count()) * 1000;
    select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
#endif
}

} // namespace

void play_quest_confetti(std::ostream& out) {
    if (!stdout_is_tty() || !out) return;

    for (int frame = 0; frame < kFrames; ++frame) {
        if (frame > 0) {
            if (stdin_has_data()) break;
            out << "\x1b[" << kRows << "A";
        }
        write_frame(out, frame);
        out.flush();
        wait_frame();
        if (stdin_has_data()) break;
    }

    // Erase the burst so the next prompt sits under the quest text.
    out << "\x1b[" << kRows << "A\x1b[J" << std::flush;
}

} // namespace smite
