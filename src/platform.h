//
//  platform.h
//  asciiplay
//
//  Created by Piotr Panasewicz on 10/08/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Thin OS abstraction: child processes, terminal control, key polling.
// Everything else in asciiplay is portable C++17.
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace plat {

// A spawned child process, optionally with its stdout on a pipe we can read.
// Moving is allowed; copying is not.  The destructor terminates the child.
class Proc {
public:
    Proc() = default;
    ~Proc();
    Proc(const Proc&) = delete;
    Proc& operator=(const Proc&) = delete;
    Proc(Proc&& o) noexcept;
    Proc& operator=(Proc&& o) noexcept;

    // argv[0] is looked up on PATH.  Returns an invalid Proc on failure.
    static Proc spawn(const std::vector<std::string>& argv, bool pipeStdout);

    bool        valid() const;
    bool        readExact(uint8_t* buf, std::size_t n);  // false at EOF/error
    std::string readAll();
    void        stop();

private:
    void* proc_ = nullptr;   // HANDLE on Windows, encoded pid on POSIX
    void* pipe_ = nullptr;   // HANDLE on Windows, encoded fd on POSIX
};

// When true, children keep our stderr instead of /dev/null, so ffmpeg and
// ffplay can report why they failed.  Off by default: their output would
// scribble over the alternate screen during playback.
void setChildStderrVisible(bool on);

// Terminal.  enter() switches to the alternate screen, hides the cursor and
// puts the input stream in raw/unbuffered mode; leave() undoes all of it and
// is safe to call twice.
void terminalEnter();
void terminalLeave();
bool terminalSize(int& cols, int& rows);

// Non-blocking single-key read.  Returns -1 when nothing is pending.
int pollKey();

// Ctrl-C / console-close handling.  quitRequested() latches true once the user
// has asked to stop, so the main loop can unwind and restore the terminal.
void installQuitHandler();
bool quitRequested();
void requestQuit();

// ---- portable helpers -----------------------------------------------------

inline double nowSeconds() {
    using namespace std::chrono;
    static const auto t0 = steady_clock::now();
    return duration<double>(steady_clock::now() - t0).count();
}

inline void sleepMs(int ms) {
    if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void writeOut(const char* p, std::size_t n);
inline void writeOut(const std::string& s) { writeOut(s.data(), s.size()); }

} // namespace plat
