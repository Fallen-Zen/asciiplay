//
//  platform_win.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 10/08/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Windows implementation of platform.h.
//
// Needs Windows 10 1511 or newer for ENABLE_VIRTUAL_TERMINAL_PROCESSING, which
// is what makes the ANSI colour output work.  Windows Terminal is strongly
// preferred over the legacy console host: it is far faster at the volume of
// escape sequences video playback produces, and it renders the block-element
// glyphs correctly.
#if defined(_WIN32)

#include "platform.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <fcntl.h>   // _O_BINARY; <io.h> declares _setmode but not the modes
#include <io.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

namespace plat {
namespace {

volatile LONG g_quit = 0;
bool g_childStderr = false;

DWORD g_outModeSaved = 0, g_inModeSaved = 0;
UINT  g_cpSaved = 0;
bool  g_modeSaved = false, g_alt = false;
bool  g_leftDown = false;       // a release only counts after our own press

BOOL WINAPI ctrlHandler(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            InterlockedExchange(&g_quit, 1);
            return TRUE;
        default:
            return FALSE;
    }
}

// Quote one argument the way CommandLineToArgvW parses it back.
void appendArg(std::string& out, const std::string& arg) {
    if (!out.empty()) out += ' ';
    bool needQuote = arg.empty() ||
                     arg.find_first_of(" \t\n\v\"") != std::string::npos;
    if (!needQuote) { out += arg; return; }

    out += '"';
    for (std::size_t i = 0; i < arg.size(); ++i) {
        std::size_t slashes = 0;
        while (i < arg.size() && arg[i] == '\\') { ++slashes; ++i; }
        if (i == arg.size()) {
            out.append(slashes * 2, '\\');   // before the closing quote
            break;
        }
        if (arg[i] == '"') {
            out.append(slashes * 2 + 1, '\\');
            out += '"';
        } else {
            out.append(slashes, '\\');
            out += arg[i];
        }
    }
    out += '"';
}

} // namespace

// ------------------------------------------------------------------- Proc --

Proc::~Proc() { stop(); }

Proc::Proc(Proc&& o) noexcept : proc_(o.proc_), pipe_(o.pipe_) {
    o.proc_ = nullptr; o.pipe_ = nullptr;
}

Proc& Proc::operator=(Proc&& o) noexcept {
    if (this != &o) {
        stop();
        proc_ = o.proc_; pipe_ = o.pipe_;
        o.proc_ = nullptr; o.pipe_ = nullptr;
    }
    return *this;
}

bool Proc::valid() const { return proc_ != nullptr; }

bool haveExecutable(const std::string& name) {
    // SearchPath applies the usual rules, including PATHEXT via the extension
    // hint, so "ffmpeg" finds ffmpeg.exe.
    char found[MAX_PATH];
    char* filePart = nullptr;
    DWORD n = ::SearchPathA(nullptr, name.c_str(), ".exe",
                            (DWORD)sizeof found, found, &filePart);
    return n > 0 && n < sizeof found;
}

Proc Proc::spawn(const std::vector<std::string>& argv, bool pipeStdout) {
    Proc r;
    if (argv.empty()) return r;

    HANDLE rd = nullptr, wr = nullptr;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;

    if (pipeStdout) {
        if (!CreatePipe(&rd, &wr, &sa, 1 << 20)) return r;
        // Our end must not leak into the child, or EOF would never arrive.
        SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    }

    HANDLE nul = CreateFileA("NUL", GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                             OPEN_EXISTING, 0, nullptr);
    // Never hand the child our console input: ffmpeg and ffplay both poll
    // stdin for interactive keys and would swallow keystrokes meant for us.
    HANDLE nulIn = CreateFileA("NUL", GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                               OPEN_EXISTING, 0, nullptr);

    STARTUPINFOA si{};
    si.cb = sizeof si;
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = nulIn;
    si.hStdOutput = pipeStdout ? wr : nul;
    si.hStdError  = g_childStderr ? GetStdHandle(STD_ERROR_HANDLE) : nul;

    std::string cmd;
    for (const auto& a : argv) appendArg(cmd, a);

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                             TRUE, CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                             nullptr, nullptr, &si, &pi);

    if (wr)    CloseHandle(wr);        // child owns it now
    if (nul)   CloseHandle(nul);
    if (nulIn) CloseHandle(nulIn);

    if (!ok) { if (rd) CloseHandle(rd); return r; }

    CloseHandle(pi.hThread);
    r.proc_ = pi.hProcess;
    r.pipe_ = rd;
    return r;
}

bool Proc::readExact(uint8_t* buf, std::size_t n) {
    if (!pipe_) return false;
    std::size_t got = 0;
    while (got < n) {
        DWORD want = (DWORD)((n - got) > (1u << 30) ? (1u << 30) : (n - got));
        DWORD rd = 0;
        if (!ReadFile((HANDLE)pipe_, buf + got, want, &rd, nullptr) || rd == 0)
            return false;                                   // EOF or error
        got += rd;
    }
    return true;
}

std::string Proc::readAll() {
    std::string out;
    if (!pipe_) return out;
    char buf[4096];
    DWORD rd = 0;
    while (ReadFile((HANDLE)pipe_, buf, sizeof buf, &rd, nullptr) && rd > 0)
        out.append(buf, rd);
    return out;
}

void Proc::stop() {
    if (pipe_) { CloseHandle((HANDLE)pipe_); pipe_ = nullptr; }
    if (proc_) {
        TerminateProcess((HANDLE)proc_, 0);
        WaitForSingleObject((HANDLE)proc_, 2000);
        CloseHandle((HANDLE)proc_);
        proc_ = nullptr;
    }
}

// --------------------------------------------------------------- terminal --

void terminalEnter() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE in  = GetStdHandle(STD_INPUT_HANDLE);

    g_cpSaved = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);                    // block glyphs are UTF-8

    if (GetConsoleMode(out, &g_outModeSaved) && GetConsoleMode(in, &g_inModeSaved)) {
        SetConsoleMode(out, g_outModeSaved | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        // Mouse events as input records.  Quick-edit has to go, or a click
        // starts a text selection (and freezes our output) instead.  VT input
        // goes too, so keys arrive with virtual-key codes, not escape bytes.
        DWORD m = g_inModeSaved & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                                           ENABLE_QUICK_EDIT_MODE |
                                           ENABLE_VIRTUAL_TERMINAL_INPUT);
        SetConsoleMode(in, m | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT);
        g_modeSaved = true;
    }
    _setmode(_fileno(stdout), _O_BINARY);           // do not translate \n
    std::fputs("\x1b[?1049h\x1b[?25l", stdout);
    std::fflush(stdout);
    g_alt = true;
}

void terminalLeave() {
    if (g_alt) {
        std::fputs("\x1b[0m\x1b[?25h\x1b[?1049l", stdout);
        std::fflush(stdout);
        g_alt = false;
    }
    if (g_modeSaved) {
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_outModeSaved);
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),  g_inModeSaved);
        g_modeSaved = false;
    }
    if (g_cpSaved) { SetConsoleOutputCP(g_cpSaved); g_cpSaved = 0; }
}

bool terminalSize(int& cols, int& rows) {
    CONSOLE_SCREEN_BUFFER_INFO ci{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci)) {
        cols = ci.srWindow.Right  - ci.srWindow.Left + 1;
        rows = ci.srWindow.Bottom - ci.srWindow.Top  + 1;
        if (cols > 0 && rows > 0) return true;
    }
    cols = 80; rows = 24;
    return false;
}

Input pollInput() {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD pending = 0;
    while (GetNumberOfConsoleInputEvents(h, &pending) && pending > 0) {
        INPUT_RECORD r{};
        DWORD got = 0;
        if (!ReadConsoleInputW(h, &r, 1, &got) || got == 0) break;
        Input in;

        if (r.EventType == KEY_EVENT) {
            const KEY_EVENT_RECORD& k = r.Event.KeyEvent;
            if (!k.bKeyDown) continue;
            switch (k.wVirtualKeyCode) {
                case VK_LEFT:   in.kind = Input::Left;     return in;
                case VK_RIGHT:  in.kind = Input::Right;    return in;
                case VK_UP:     in.kind = Input::Up;       return in;
                case VK_DOWN:   in.kind = Input::Down;     return in;
                case VK_PRIOR:  in.kind = Input::PageUp;   return in;
                case VK_NEXT:   in.kind = Input::PageDown; return in;
                case VK_HOME:   in.kind = Input::Home;     return in;
                case VK_END:    in.kind = Input::End;      return in;
                case VK_ESCAPE: in.kind = Input::Esc;      return in;
                default: break;
            }
            const WCHAR ch = k.uChar.UnicodeChar;
            if (ch > 0 && ch < 128) { in.kind = Input::Char; in.ch = ch; return in; }
            continue;
        }

        if (r.EventType == MOUSE_EVENT) {
            const MOUSE_EVENT_RECORD& m = r.Event.MouseEvent;
            CONSOLE_SCREEN_BUFFER_INFO ci{};
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
            in.x = m.dwMousePosition.X - ci.srWindow.Left;   // buffer -> window
            in.y = m.dwMousePosition.Y - ci.srWindow.Top;

            if (m.dwEventFlags & MOUSE_WHEELED) {
                in.kind = (short)HIWORD(m.dwButtonState) > 0 ? Input::WheelUp
                                                             : Input::WheelDown;
                return in;
            }
            const bool left = (m.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
            if (m.dwEventFlags & MOUSE_MOVED) {
                if (!left || !g_leftDown) continue;
                in.kind = Input::MouseDrag;
                return in;
            }
            if (left && !g_leftDown) {
                g_leftDown = true;
                in.kind = Input::MousePress;
                return in;
            }
            if (!left && g_leftDown) {
                g_leftDown = false;
                in.kind = Input::MouseRelease;
                return in;
            }
        }
    }
    return Input{};
}

void setChildStderrVisible(bool on) { g_childStderr = on; }

void installQuitHandler() { SetConsoleCtrlHandler(ctrlHandler, TRUE); }

bool quitRequested() { return InterlockedCompareExchange(&g_quit, 0, 0) != 0; }
void requestQuit()   { InterlockedExchange(&g_quit, 1); }

void writeOut(const char* p, std::size_t n) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    std::size_t off = 0;
    while (off < n) {
        DWORD want = (DWORD)((n - off) > (1u << 30) ? (1u << 30) : (n - off));
        DWORD wrote = 0;
        if (!WriteFile(h, p + off, want, &wrote, nullptr) || wrote == 0) break;
        off += wrote;
    }
}

} // namespace plat

#endif // _WIN32
