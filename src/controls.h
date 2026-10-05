//
//  controls.h
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Playback controls: what each key seeks to, and the on-demand progress bar.
// Nothing here touches the terminal -- the bar builds its escape sequences and
// the player writes them -- so all of it can be unit tested.
#pragma once

#include "asciiart.h"
#include "platform.h"

#include <string>

constexpr double kStep    = 5.0;    // Left/Right and the mouse wheel
constexpr double kBigStep = 60.0;   // Up/Down, PgUp/PgDn

// "4:05", or "1:04:05" once the file runs that long.
std::string clockText(double sec, bool hours);

// Keeps a seek target inside the file.  It stops a second short of the end,
// so End and Right land on something to show instead of finishing playback.
// With no known length (dur <= 0) the end could be anywhere, so forward seeks
// go no further than playback has already been (reached) -- past the real end
// ffmpeg would send nothing and one keypress would end playback.
double clampSeek(double t, double dur, double reached);

// Where a key or wheel event seeks to from `from`, clamped; -1 when the event
// is not a seek, or is one that cannot apply: a percentage or End with no
// known length, or a forward key that would not move forward.
double seekFor(const plat::Input& in, double from, double dur, double reached);

// The progress bar on the terminal's bottom row:  ▶ 1:23 ━━━━●──── 4:56
// It is drawn on demand -- while paused, while being dragged, and for a moment
// after a key or click -- so the rest of the time the picture has the screen.
struct SeekBar {
    ColorMode mode = ColorMode::True;
    double dur = 0;              // <= 0: length unknown, so no slider
    bool   hours = false;
    int    row = 0, width = 0;   // 0-based terminal row; width in columns
    int    label = 0;            // columns in each time label
    int    trackX = 0, trackW = 0;
    bool   shown = false;
    std::string buf;             // reused, so a redraw does not allocate

    // What is on screen.  render() runs every few milliseconds while the bar is
    // up but the picture only changes once a second or when the knob moves a
    // cell, so it compares these before building anything.
    long lastSec = -1;
    int  lastKnob = -1;
    bool lastPaused = false;

    // Lays the bar out along the bottom row of a tc x tr terminal.
    void place(int tc, int tr);

    // Only a bar the user can see takes clicks: the bottom row is picture the
    // rest of the time, and a stray click there must not seek.
    bool   hit(int y) const { return shown && trackW > 0 && y == row; }
    double timeAt(int x) const;  // media time under column x of the track

    // Builds the bar into buf and returns true when that differs from what is
    // on screen; the caller writes buf.  False also when there is no room.
    bool render(double pos, bool paused);

    // The sequence that blanks the bar's row, for the caller to write.
    const std::string& clear();
};

