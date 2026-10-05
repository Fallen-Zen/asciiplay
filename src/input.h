//
//  input.h
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Decoding of terminal input -- keys and xterm mouse reports -- from a byte
// stream.  Portable, so the POSIX terminal and the unit tests share it; the
// Windows console delivers ready-made events and does not need it.
#pragma once

#include "platform.h"

#include <functional>

namespace plat {

// next(ms) returns the next byte, waiting up to ms for it, or -1 when none
// arrives.  Reads one event; kind is None when no byte is pending, and also
// when a sequence was read but is not one we act on (Alt+key, F-keys, middle
// and right clicks), so its bytes never leak out as stray characters.
Input decodeInput(const std::function<int(int ms)>& next);

} // namespace plat
