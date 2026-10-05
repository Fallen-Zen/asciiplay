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
    //
    // On POSIX "failure" only covers fork: a missing executable fails in the
    // child, after the fork has already succeeded, so the parent gets a valid
    // Proc whose pipe reads EOF.  Use haveExecutable() to tell "not installed"
    // apart from "ran but produced nothing".
    static Proc spawn(const std::vector<std::string>& argv, bool pipeStdout);

    bool        valid() const;
    bool        readExact(uint8_t* buf, std::size_t n);  // false at EOF/error
    std::string readAll();
    void        stop();

private:
    void* proc_ = nullptr;   // HANDLE on Windows, encoded pid on POSIX
    void* pipe_ = nullptr;   // HANDLE on Windows, encoded fd on POSIX
};

// Is this command runnable -- an executable of that name on PATH (or a path
// that is itself executable)?  Lets us say "ffmpeg is not installed" instead of
// blaming the input file.
bool haveExecutable(const std::string& name);

// When true, children keep our stderr instead of /dev/null, so ffmpeg and
// ffplay can report why they failed.  Off by default: their output would
// scribble over the alternate screen during playback.
void setChildStderrVisible(bool on);

// Terminal.  enter() switches to the alternate screen, hides the cursor, puts
// the input stream in raw/unbuffered mode and turns on mouse reporting;
// leave() undoes all of it and is safe to call twice.
void terminalEnter();
void terminalLeave();
bool terminalSize(int& cols, int& rows);

// One key or mouse event.  Mouse coordinates are 0-based terminal cells, and
// only the left button is reported -- the others are left to the terminal.
struct Input {
    enum Kind {
        None, Char, Esc,
        Left, Right, Up, Down, PageUp, PageDown, Home, End,
        MousePress, MouseDrag, MouseRelease, WheelUp, WheelDown,
    };
    Kind kind = None;
    int  ch = 0;            // Char: the byte typed
    int  x = 0, y = 0;      // mouse events
};

// Non-blocking read of the next event.  kind is None when nothing is pending;
// unrecognised sequences are swallowed rather than returned as stray bytes.
Input pollInput();

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
