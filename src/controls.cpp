//
//  controls.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
#include "controls.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// "4:05", or "1:04:05" once the file runs that long.
std::string clockText(double sec, bool hours) {
    const long t = std::max(0L, (long)sec);
    char b[32];
    if (hours) std::snprintf(b, sizeof b, "%ld:%02ld:%02ld", t / 3600, t / 60 % 60, t % 60);
    else       std::snprintf(b, sizeof b, "%ld:%02ld", t / 60, t % 60);
    return b;
}

double clampSeek(double t, double dur, double reached) {
    if (dur > 0) t = std::min(t, std::max(0.0, dur - 1.0));
    else         t = std::min(t, reached);
    return std::max(0.0, t);
}

double seekFor(const plat::Input& in, double from, double dur, double reached) {
    using I = plat::Input;
    switch (in.kind) {
    case I::Left:  case I::WheelDown: return clampSeek(from - kStep, dur, reached);
    case I::Right: case I::WheelUp:   return clampSeek(from + kStep, dur, reached);
    case I::Down:  case I::PageDown:  return clampSeek(from - kBigStep, dur, reached);
    case I::Up:    case I::PageUp:    return clampSeek(from + kBigStep, dur, reached);
    case I::Home:                     return 0;
    case I::End:   return dur > 0 ? clampSeek(dur, dur, reached) : -1;
    case I::Char:
        if (in.ch >= '0' && in.ch <= '9' && dur > 0)
            return clampSeek(dur * (in.ch - '0') / 10.0, dur, reached);
        return -1;
    default:
        return -1;
    }
}

// ---------------------------------------------------------------- SeekBar --

void SeekBar::place(int tc, int tr) {
    row = tr - 1;
    width = tc;
    hours = dur >= 3600;
    label = (int)clockText(std::max(dur, 0.0), hours).size();
    trackX = label + 4;                         // " ▶ " label " "
    trackW = width - trackX - (label + 2);      // " " label " "
    if (dur <= 0 || trackW < 8) trackW = 0;
    shown = false;
}

double SeekBar::timeAt(int x) const {
    const double f = (double)(x - trackX) / std::max(1, trackW - 1);
    return std::min(1.0, std::max(0.0, f)) * dur;
}

bool SeekBar::render(double pos, bool paused) {
    const long sec  = (long)std::max(0.0, pos);
    const int  knob = trackW > 0
        ? (int)std::lround(std::min(1.0, std::max(0.0, pos / dur)) * (trackW - 1))
        : 0;
    if (shown && sec == lastSec && knob == lastKnob && paused == lastPaused) return false;

    std::string now = clockText(pos, hours || pos >= 3600);
    if ((int)now.size() < label) now.insert(0, label - now.size(), ' ');
    if (width < (int)now.size() + 3) return false;  // no room for anything

    const bool col = mode != ColorMode::None;
    char t[32];
    std::snprintf(t, sizeof t, "\x1b[%d;1H", row + 1);
    buf = t;
    if (col) buf += "\x1b[0;48;5;236;38;5;252m";
    buf += paused ? " \u2016 " : " \u25B6 ";      // ‖ or ▶
    buf += now;
    if (trackW > 0) {
        buf += ' ';
        if (col) buf += "\x1b[38;5;45m";
        for (int i = 0; i < knob; ++i) buf += "\u2501";            // ━
        if (col) buf += "\x1b[38;5;231m";
        buf += "\u25CF";                                          // ●
        if (col) buf += "\x1b[38;5;240m";
        for (int i = knob + 1; i < trackW; ++i) buf += "\u2500";   // ─
        if (col) buf += "\x1b[38;5;252m";
        buf += ' ';
        buf += clockText(dur, hours);
        buf += ' ';
    } else {
        buf.append(width - 3 - now.size(), ' ');
    }
    buf += "\x1b[0m";
    shown = true;
    lastSec = sec;
    lastKnob = knob;
    lastPaused = paused;
    return true;
}

const std::string& SeekBar::clear() {
    char t[48];
    std::snprintf(t, sizeof t, "\x1b[%d;1H\x1b[0m\x1b[2K", row + 1);
    buf = t;
    shown = false;
    return buf;
}
