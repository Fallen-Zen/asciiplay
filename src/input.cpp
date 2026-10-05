//
//  input.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
#include "input.h"

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <string>

namespace plat {
namespace {

// How long to wait for the rest of an escape sequence once it has started.
// Long enough that a sequence split in transit -- a laggy SSH link, a
// multiplexer -- is still read as one, because a lone Esc quits.  100 ms is
// vim's ttimeoutlen default for the same reason.
constexpr int kSeqWaitMs = 100;

// Longer parameter strings are no key or mouse report we know; such a
// sequence is still read to its end, just not kept.
constexpr std::size_t kMaxParams = 32;

// b is the xterm button byte: low two bits the button (3 = released, in the
// legacy encoding), +32 motion, +64 wheel.  x and y arrive 1-based.
Input mouseEvent(int b, int x, int y, bool release) {
    Input in;
    in.x = x - 1;
    in.y = y - 1;
    if (b & 64) {
        if ((b & 3) <= 1) in.kind = (b & 1) ? Input::WheelDown : Input::WheelUp;
        return in;
    }
    const int button = b & 3;
    if (release || button == 3)  in.kind = Input::MouseRelease;
    else if (button != 0)        return in;           // middle/right: not ours
    else if (b & 32)             in.kind = Input::MouseDrag;
    else                         in.kind = Input::MousePress;
    return in;
}

Input keyFor(int final, int param) {
    Input in;
    switch (final) {
        case 'A': in.kind = Input::Up;    break;
        case 'B': in.kind = Input::Down;  break;
        case 'C': in.kind = Input::Right; break;
        case 'D': in.kind = Input::Left;  break;
        case 'H': in.kind = Input::Home;  break;
        case 'F': in.kind = Input::End;   break;
        case '~':
            switch (param) {
                case 1: case 7: in.kind = Input::Home;     break;
                case 4: case 8: in.kind = Input::End;      break;
                case 5:         in.kind = Input::PageUp;   break;
                case 6:         in.kind = Input::PageDown; break;
                default: break;
            }
            break;
        default: break;
    }
    return in;
}

} // namespace

Input decodeInput(const std::function<int(int ms)>& next) {
    Input in;
    const int c = next(0);
    if (c < 0) return in;
    if (c != 27) { in.kind = Input::Char; in.ch = c; return in; }

    // A bare Esc and the start of an escape sequence begin with the same byte;
    // a wait for a second byte tells them apart.
    const int c1 = next(kSeqWaitMs);
    if (c1 < 0) { in.kind = Input::Esc; return in; }
    if (c1 == 'O') return keyFor(next(kSeqWaitMs), 0);  // application cursor keys
    if (c1 != '[') return in;                           // Alt+key: ignore

    // CSI params, then a final byte.  Whatever happens, read up to that final
    // byte: bytes left behind would be decoded as keys, and a stray q, space
    // or digit there would quit, pause or seek.
    std::string params;
    bool overlong = false;
    int f;
    for (;;) {
        f = next(kSeqWaitMs);
        if (f < 0) return in;                           // cut short: nothing to act on
        if (f >= 0x40 && f <= 0x7e) break;
        if (params.size() < kMaxParams) params += (char)f;
        else                            overlong = true;
    }
    if (overlong) return in;

    if (f == 'M' && params.empty()) {                   // legacy mouse: 3 raw bytes
        const int b = next(kSeqWaitMs), x = next(kSeqWaitMs), y = next(kSeqWaitMs);
        if (b < 0 || x < 0 || y < 0) return in;
        return mouseEvent(b - 32, x - 32, y - 32, false);
    }
    if (!params.empty() && params[0] == '<' && (f == 'M' || f == 'm')) {
        int b = 0, x = 0, y = 0;                        // SGR mouse: <b;x;y
        if (std::sscanf(params.c_str() + 1, "%d;%d;%d", &b, &x, &y) != 3) return in;
        return mouseEvent(b, x, y, f == 'm');
    }
    return keyFor(f, std::atoi(params.c_str()));        // "5" of 5~, "1" of 1;5C
}

} // namespace plat
