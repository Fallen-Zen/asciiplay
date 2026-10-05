//
//  test_input.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// decodeInput: keys and xterm mouse reports, as terminals actually send them.
#include "check.h"
#include "input.h"

#include <deque>

using plat::Input;

namespace {

// Bytes as a terminal would deliver them.  An empty feed times out, as a
// quiet terminal does.
struct Feed {
    std::deque<int> bytes;
    std::vector<int> waits;          // the timeout asked for on each read

    explicit Feed(const std::string& s) {
        for (unsigned char c : s) bytes.push_back(c);
    }
    Input next() {
        return plat::decodeInput([this](int ms) {
            waits.push_back(ms);
            if (bytes.empty()) return -1;
            int c = bytes.front();
            bytes.pop_front();
            return c;
        });
    }
};

Input decode(const std::string& s) { return Feed(s).next(); }

std::string legacyMouse(int b, int x, int y) {   // x, y 1-based, as sent
    return std::string("\x1b[M") + (char)(b + 32) + (char)(x + 32) + (char)(y + 32);
}

} // namespace

TEST(input_nothing_pending_is_none) {
    Feed f("");
    CHECK_EQ(f.next().kind, Input::None);
    CHECK_EQ(f.waits.size(), 1u);
    CHECK_EQ(f.waits[0], 0);         // polling must never block
}

TEST(input_plain_bytes_are_chars) {
    Input in = decode("q");
    CHECK_EQ(in.kind, Input::Char);
    CHECK_EQ(in.ch, 'q');
    CHECK_EQ(decode(" ").ch, ' ');
    CHECK_EQ(decode("7").ch, '7');
}

TEST(input_bare_esc_is_esc_not_a_sequence) {
    Feed f("\x1b");
    CHECK_EQ(f.next().kind, Input::Esc);
    CHECK_EQ(f.waits.size(), 2u);
    CHECK(f.waits[1] > 0);           // waited a moment for the rest
}

TEST(input_arrow_keys) {
    CHECK_EQ(decode("\x1b[A").kind, Input::Up);
    CHECK_EQ(decode("\x1b[B").kind, Input::Down);
    CHECK_EQ(decode("\x1b[C").kind, Input::Right);
    CHECK_EQ(decode("\x1b[D").kind, Input::Left);
}

TEST(input_application_mode_keys) {
    CHECK_EQ(decode("\x1bOA").kind, Input::Up);
    CHECK_EQ(decode("\x1bOD").kind, Input::Left);
    CHECK_EQ(decode("\x1bOH").kind, Input::Home);
    CHECK_EQ(decode("\x1bOF").kind, Input::End);
}

TEST(input_home_end_and_paging) {
    CHECK_EQ(decode("\x1b[H").kind, Input::Home);
    CHECK_EQ(decode("\x1b[F").kind, Input::End);
    CHECK_EQ(decode("\x1b[1~").kind, Input::Home);
    CHECK_EQ(decode("\x1b[7~").kind, Input::Home);    // rxvt
    CHECK_EQ(decode("\x1b[4~").kind, Input::End);
    CHECK_EQ(decode("\x1b[8~").kind, Input::End);     // rxvt
    CHECK_EQ(decode("\x1b[5~").kind, Input::PageUp);
    CHECK_EQ(decode("\x1b[6~").kind, Input::PageDown);
}

TEST(input_modified_arrows_still_arrows) {
    CHECK_EQ(decode("\x1b[1;5C").kind, Input::Right);  // Ctrl
    CHECK_EQ(decode("\x1b[1;2D").kind, Input::Left);   // Shift
}

TEST(input_unknown_sequences_are_swallowed_whole) {
    Feed f("\x1b[15~x");             // F5, then a key
    CHECK_EQ(f.next().kind, Input::None);
    Input in = f.next();
    CHECK_EQ(in.kind, Input::Char);
    CHECK_EQ(in.ch, 'x');            // F5 left no stray bytes behind
}

TEST(input_alt_key_is_ignored) {
    Feed f("\x1bxq");
    CHECK_EQ(f.next().kind, Input::None);
    CHECK_EQ(f.next().ch, 'q');
}

TEST(input_truncated_sequence_does_not_hang) {
    CHECK_EQ(decode("\x1b[").kind, Input::None);
    CHECK_EQ(decode("\x1b[1;").kind, Input::None);
    CHECK_EQ(decode("\x1bO").kind, Input::None);
}

TEST(input_overlong_sequence_is_dropped) {
    CHECK_EQ(decode("\x1b[" + std::string(64, '1') + "~").kind, Input::None);
}

TEST(input_sgr_mouse_press_release_drag) {
    Input in = decode("\x1b[<0;10;5M");
    CHECK_EQ(in.kind, Input::MousePress);
    CHECK_EQ(in.x, 9);               // reported 1-based, returned 0-based
    CHECK_EQ(in.y, 4);

    in = decode("\x1b[<0;10;5m");
    CHECK_EQ(in.kind, Input::MouseRelease);

    in = decode("\x1b[<32;42;30M");
    CHECK_EQ(in.kind, Input::MouseDrag);
    CHECK_EQ(in.x, 41);
    CHECK_EQ(in.y, 29);
}

TEST(input_sgr_mouse_beyond_column_223) {
    Input in = decode("\x1b[<0;300;80M");  // the reason for SGR over legacy
    CHECK_EQ(in.kind, Input::MousePress);
    CHECK_EQ(in.x, 299);
    CHECK_EQ(in.y, 79);
}

TEST(input_sgr_modified_click_is_still_a_click) {
    CHECK_EQ(decode("\x1b[<4;1;1M").kind, Input::MousePress);    // Shift
    CHECK_EQ(decode("\x1b[<16;1;1M").kind, Input::MousePress);   // Ctrl
}

TEST(input_wheel) {
    Input in = decode("\x1b[<64;7;3M");
    CHECK_EQ(in.kind, Input::WheelUp);
    CHECK_EQ(in.x, 6);
    CHECK_EQ(decode("\x1b[<65;7;3M").kind, Input::WheelDown);
}

TEST(input_other_buttons_are_left_to_the_terminal) {
    CHECK_EQ(decode("\x1b[<1;5;5M").kind, Input::None);   // middle
    CHECK_EQ(decode("\x1b[<2;5;5M").kind, Input::None);   // right
    CHECK_EQ(decode("\x1b[<34;5;5M").kind, Input::None);  // right-drag
    CHECK_EQ(decode("\x1b[<66;5;5M").kind, Input::None);  // wheel left/right
}

TEST(input_malformed_sgr_mouse) {
    CHECK_EQ(decode("\x1b[<0;5M").kind, Input::None);
    CHECK_EQ(decode("\x1b[<a;b;cM").kind, Input::None);
}

TEST(input_legacy_mouse) {
    Input in = decode(legacyMouse(0, 10, 5));
    CHECK_EQ(in.kind, Input::MousePress);
    CHECK_EQ(in.x, 9);
    CHECK_EQ(in.y, 4);
    CHECK_EQ(decode(legacyMouse(3, 10, 5)).kind, Input::MouseRelease);
    CHECK_EQ(decode(legacyMouse(32, 12, 5)).kind, Input::MouseDrag);
    CHECK_EQ(decode(legacyMouse(64, 1, 1)).kind, Input::WheelUp);
    CHECK_EQ(decode("\x1b[M!").kind, Input::None);   // cut short
}

TEST(input_events_back_to_back) {
    Feed f("\x1b[C\x1b[<0;2;3M\x1b[<0;2;3m q");
    CHECK_EQ(f.next().kind, Input::Right);
    CHECK_EQ(f.next().kind, Input::MousePress);
    CHECK_EQ(f.next().kind, Input::MouseRelease);
    CHECK_EQ(f.next().ch, ' ');
    CHECK_EQ(f.next().ch, 'q');
    CHECK_EQ(f.next().kind, Input::None);
}
