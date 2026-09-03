//
//  platform_posix.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 10/08/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// POSIX implementation of platform.h (Linux, macOS, BSD).
#if !defined(_WIN32)

#include "platform.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>   // getenv/setenv
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace plat {
namespace {

// pid/fd are stored in the void* slots; +1 keeps "no value" as nullptr.
inline void*  enc(long v)   { return reinterpret_cast<void*>(v + 1); }
inline long   dec(void* p)  { return p ? reinterpret_cast<long>(p) - 1 : -1; }

volatile sig_atomic_t g_quit = 0;
bool g_childStderr = false;
void onSignal(int) { g_quit = 1; }

termios g_saved{};
bool    g_raw = false, g_alt = false;

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

bool Proc::valid() const { return dec(proc_) > 0; }

bool haveExecutable(const std::string& name) {
    auto runnable = [](const std::string& p) {
        struct stat st;
        return ::access(p.c_str(), X_OK) == 0 &&
               ::stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    };
    if (name.find('/') != std::string::npos) return runnable(name);

    const char* path = ::getenv("PATH");
    if (!path || !*path) path = "/usr/bin:/bin:/usr/local/bin";
    std::string dir;
    for (const char* c = path; ; ++c) {
        if (*c && *c != ':') { dir += *c; continue; }
        if (!dir.empty() && runnable(dir + "/" + name)) return true;
        dir.clear();
        if (!*c) break;
    }
    return false;
}

Proc Proc::spawn(const std::vector<std::string>& argv, bool pipeStdout) {
    Proc r;
    int p[2] = {-1, -1};
    if (pipeStdout) {
        if (::pipe(p) != 0) return r;
        // Mark both ends close-on-exec, or a *later* child (ffplay) inherits
        // this decoder's read end and holds it open.  ffmpeg would then never
        // see EOF after we exit, and would linger forever reparented to init.
        // dup2() clears the flag, so the child's own stdout still survives exec.
        ::fcntl(p[0], F_SETFD, FD_CLOEXEC);
        ::fcntl(p[1], F_SETFD, FD_CLOEXEC);
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        if (pipeStdout) { ::close(p[0]); ::close(p[1]); }
        return r;
    }

    if (pid == 0) {                                     // ---- child ----
        if (pipeStdout) {
            ::close(p[0]);
            ::dup2(p[1], STDOUT_FILENO);
            ::close(p[1]);
        }
        int null = ::open("/dev/null", O_WRONLY);
        if (null >= 0) {
            if (!g_childStderr) ::dup2(null, STDERR_FILENO);
            if (!pipeStdout)    ::dup2(null, STDOUT_FILENO);
            ::close(null);
        }

        // Over SSH, XDG_RUNTIME_DIR is often unset.  Without it ffplay cannot
        // find the PipeWire/PulseAudio socket, falls back to raw ALSA, fails to
        // open the device, and exits -- silently, from the user's point of view.
        // Point it at the session's runtime directory when one exists.
        if (!::getenv("XDG_RUNTIME_DIR")) {
            char rt[64];
            std::snprintf(rt, sizeof rt, "/run/user/%u", (unsigned)::getuid());
            struct stat st{};
            if (::stat(rt, &st) == 0 && S_ISDIR(st.st_mode) && st.st_uid == ::getuid())
                ::setenv("XDG_RUNTIME_DIR", rt, 1);
        }
        // Detach stdin from the terminal.  The child gets its own process group
        // below, so any read of the controlling terminal raises SIGTTIN and
        // stops it -- and ffmpeg/ffplay both poll stdin for interactive keys.
        // That would deadlock us waiting on a frame that never arrives.
        int nullIn = ::open("/dev/null", O_RDONLY);
        if (nullIn >= 0) { ::dup2(nullIn, STDIN_FILENO); ::close(nullIn); }

        ::setpgid(0, 0);        // own group: our Ctrl-C must not reach ffmpeg

#if defined(__linux__)
        // If we are killed outright (SIGKILL, crash), take the child with us.
        ::prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (::getppid() == 1) ::_exit(0);   // parent died during the race above
#endif

        std::vector<char*> cargv;
        cargv.reserve(argv.size() + 1);
        for (const auto& s : argv) cargv.push_back(const_cast<char*>(s.c_str()));
        cargv.push_back(nullptr);
        ::execvp(cargv[0], cargv.data());
        ::_exit(127);
    }

    r.proc_ = enc(pid);                                 // ---- parent ----
    if (pipeStdout) { ::close(p[1]); r.pipe_ = enc(p[0]); }
    return r;
}

bool Proc::readExact(uint8_t* buf, std::size_t n) {
    int fd = (int)dec(pipe_);
    if (fd < 0) return false;
    std::size_t got = 0;
    while (got < n) {
        ssize_t r = ::read(fd, buf + got, n - got);
        if (r > 0)                   { got += (std::size_t)r; continue; }
        if (r < 0 && errno == EINTR) { if (g_quit) return false; continue; }
        return false;                                   // EOF or hard error
    }
    return true;
}

std::string Proc::readAll() {
    int fd = (int)dec(pipe_);
    std::string out;
    if (fd < 0) return out;
    char buf[4096];
    for (;;) {
        ssize_t r = ::read(fd, buf, sizeof buf);
        if (r > 0) { out.append(buf, (std::size_t)r); continue; }
        if (r < 0 && errno == EINTR) continue;
        break;
    }
    return out;
}

void Proc::stop() {
    int fd = (int)dec(pipe_);
    if (fd >= 0) { ::close(fd); pipe_ = nullptr; }

    pid_t pid = (pid_t)dec(proc_);
    if (pid > 0) {
        ::kill(-pid, SIGTERM);      // the whole group ffmpeg may have started
        ::kill(pid, SIGTERM);
        int st = 0;
        while (::waitpid(pid, &st, 0) < 0 && errno == EINTR) {}
        proc_ = nullptr;
    }
}

// --------------------------------------------------------------- terminal --

void terminalEnter() {
    if (::isatty(STDIN_FILENO) && ::tcgetattr(STDIN_FILENO, &g_saved) == 0) {
        termios t = g_saved;
        t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 0;
        ::tcsetattr(STDIN_FILENO, TCSANOW, &t);
        g_raw = true;
    }
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
    if (g_raw) { ::tcsetattr(STDIN_FILENO, TCSANOW, &g_saved); g_raw = false; }
}

bool terminalSize(int& cols, int& rows) {
    winsize ws{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row) {
        cols = ws.ws_col; rows = ws.ws_row;
        return true;
    }
    cols = 80; rows = 24;
    return false;
}

int pollKey() {
    pollfd pfd{STDIN_FILENO, POLLIN, 0};
    if (::poll(&pfd, 1, 0) > 0) {
        char c;
        if (::read(STDIN_FILENO, &c, 1) == 1) return (unsigned char)c;
    }
    return -1;
}

void setChildStderrVisible(bool on) { g_childStderr = on; }

void installQuitHandler() {
    struct sigaction sa {};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
    ::signal(SIGPIPE, SIG_IGN);   // ffmpeg dying must not kill us
}

bool quitRequested() { return g_quit != 0; }
void requestQuit()   { g_quit = 1; }

void writeOut(const char* p, std::size_t n) {
    std::size_t off = 0;
    while (off < n) {
        ssize_t w = ::write(STDOUT_FILENO, p + off, n - off);
        if (w > 0) { off += (std::size_t)w; continue; }
        if (w < 0 && errno == EINTR) continue;
        break;
    }
}

} // namespace plat

#endif // !_WIN32
