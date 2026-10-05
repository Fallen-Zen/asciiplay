//
//  test_controls.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Seek targets, clamping, and the progress bar's layout and output.
#include "check.h"
#include "controls.h"

using plat::Input;

namespace {

Input key(Input::Kind k)  { Input in; in.kind = k; return in; }
Input chr(char c)         { Input in; in.kind = Input::Char; in.ch = c; return in; }

// What the terminal shows: escapes removed, one entry per UTF-8 code point
// (every glyph the bar uses is one column wide).
std::vector<std::string> visible(const std::string& s) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < s.size(); ) {
        if (s[i] == '\x1b') {                       // CSI: up to its final byte
            i += 2;
            while (i < s.size() && !(s[i] >= 0x40 && s[i] <= 0x7e)) ++i;
            ++i;
            continue;
        }
        std::size_t n = 1;
        const unsigned char c = (unsigned char)s[i];
        if (c >= 0xf0) n = 4; else if (c >= 0xe0) n = 3; else if (c >= 0xc0) n = 2;
        out.push_back(s.substr(i, n));
        i += n;
    }
    return out;
}

int count(const std::vector<std::string>& cells, const char* glyph) {
    int n = 0;
    for (const auto& c : cells) n += c == glyph;
    return n;
}

int indexOf(const std::vector<std::string>& cells, const char* glyph) {
    for (std::size_t i = 0; i < cells.size(); ++i)
        if (cells[i] == glyph) return (int)i;
    return -1;
}

const char* kFill = "━", *kKnob = "●", *kRest = "─";

SeekBar placed(double dur, int cols, int rows, ColorMode mode = ColorMode::True) {
    SeekBar b;
    b.mode = mode;
    b.dur = dur;
    b.place(cols, rows);
    return b;
}

} // namespace

// ---- clockText ---------------------------------------------------------------

TEST(clock_minutes_and_seconds) {
    CHECK_EQ(clockText(0, false), "0:00");
    CHECK_EQ(clockText(65, false), "1:05");
    CHECK_EQ(clockText(59.99, false), "0:59");     // truncates, never rounds up
    CHECK_EQ(clockText(3599, false), "59:59");
}

TEST(clock_hours) {
    CHECK_EQ(clockText(3665, true), "1:01:05");
    CHECK_EQ(clockText(5, true), "0:00:05");
}

TEST(clock_negative_is_zero) {
    CHECK_EQ(clockText(-3, false), "0:00");
}

// ---- clampSeek ---------------------------------------------------------------

TEST(clamp_known_length) {
    CHECK_EQ(clampSeek(-5, 120, 0), 0.0);
    CHECK_EQ(clampSeek(50, 120, 0), 50.0);
    CHECK_EQ(clampSeek(200, 120, 0), 119.0);       // a second short of the end
    CHECK_EQ(clampSeek(119.5, 120, 0), 119.0);
}

TEST(clamp_clip_shorter_than_a_second) {
    CHECK_EQ(clampSeek(10, 0.5, 0), 0.0);
}

TEST(clamp_unknown_length_stops_at_furthest_played) {
    CHECK_EQ(clampSeek(40, 0, 30), 30.0);
    CHECK_EQ(clampSeek(10, 0, 30), 10.0);
    CHECK_EQ(clampSeek(-1, 0, 30), 0.0);
    CHECK_EQ(clampSeek(5, -1, 0), 0.0);            // nothing played yet
}

// ---- seekFor -----------------------------------------------------------------

TEST(seek_steps) {
    CHECK_EQ(seekFor(key(Input::Left), 30, 120, 0), 25.0);
    CHECK_EQ(seekFor(key(Input::Right), 30, 120, 0), 35.0);
    CHECK_EQ(seekFor(key(Input::Down), 90, 120, 0), 30.0);
    CHECK_EQ(seekFor(key(Input::Up), 30, 120, 0), 90.0);
    CHECK_EQ(seekFor(key(Input::PageDown), 90, 120, 0), 30.0);
    CHECK_EQ(seekFor(key(Input::PageUp), 30, 120, 0), 90.0);
}

TEST(seek_wheel_matches_arrows) {
    CHECK_EQ(seekFor(key(Input::WheelUp), 30, 120, 0), 35.0);
    CHECK_EQ(seekFor(key(Input::WheelDown), 30, 120, 0), 25.0);
}

TEST(seek_steps_are_clamped) {
    CHECK_EQ(seekFor(key(Input::Left), 2, 120, 0), 0.0);
    CHECK_EQ(seekFor(key(Input::Up), 100, 120, 0), 119.0);
}

TEST(seek_home_end) {
    CHECK_EQ(seekFor(key(Input::Home), 50, 120, 0), 0.0);
    CHECK_EQ(seekFor(key(Input::End), 50, 120, 0), 119.0);
}

TEST(seek_digits_are_tenths) {
    CHECK_EQ(seekFor(chr('0'), 50, 120, 0), 0.0);
    CHECK_EQ(seekFor(chr('5'), 0, 120, 0), 60.0);
    CHECK_EQ(seekFor(chr('9'), 0, 120, 0), 108.0);
}

TEST(seek_unknown_length) {
    CHECK_EQ(seekFor(key(Input::Right), 30, 0, 30), 30.0);   // already at the edge
    CHECK_EQ(seekFor(key(Input::Left), 30, 0, 30), 25.0);
    CHECK_EQ(seekFor(key(Input::Right), 25, 0, 30), 30.0);
    CHECK_EQ(seekFor(key(Input::Home), 30, 0, 30), 0.0);
    CHECK_EQ(seekFor(key(Input::End), 30, 0, 30), -1.0);     // nowhere to go
    CHECK_EQ(seekFor(chr('5'), 30, 0, 30), -1.0);
}

TEST(seek_non_seek_events) {
    CHECK_EQ(seekFor(chr(' '), 30, 120, 0), -1.0);
    CHECK_EQ(seekFor(chr('q'), 30, 120, 0), -1.0);
    CHECK_EQ(seekFor(key(Input::Esc), 30, 120, 0), -1.0);
    CHECK_EQ(seekFor(key(Input::MousePress), 30, 120, 0), -1.0);
    CHECK_EQ(seekFor(key(Input::None), 30, 120, 0), -1.0);
}

// ---- SeekBar layout ----------------------------------------------------------

TEST(bar_layout) {
    SeekBar b = placed(120, 100, 30);
    CHECK_EQ(b.row, 29);                          // bottom row, 0-based
    CHECK_EQ(b.label, 4);                         // "2:00"
    CHECK_EQ(b.trackX, 8);                        // " ▶ 0:00 "
    CHECK_EQ(b.trackW, 100 - 8 - 6);              // less " 2:00 "
    CHECK(!b.hours);
}

TEST(bar_layout_with_hours) {
    SeekBar b = placed(2 * 3600, 100, 30);
    CHECK(b.hours);
    CHECK_EQ(b.label, 7);                         // "2:00:00"
    CHECK_EQ(b.trackX, 11);
}

TEST(bar_has_no_track_without_length_or_room) {
    CHECK_EQ(placed(0, 100, 30).trackW, 0);
    CHECK_EQ(placed(120, 21, 30).trackW, 0);      // 7 cells left: too few
    CHECK_EQ(placed(120, 22, 30).trackW, 8);
}

TEST(bar_time_at_column) {
    SeekBar b = placed(120, 100, 30);
    CHECK_EQ(b.timeAt(b.trackX), 0.0);
    CHECK_EQ(b.timeAt(b.trackX + b.trackW - 1), 120.0);
    CHECK_NEAR(b.timeAt(b.trackX + (b.trackW - 1) / 2), 60.0, 1.0);
    CHECK_EQ(b.timeAt(0), 0.0);                   // on the left label
    CHECK_EQ(b.timeAt(99), 120.0);                // on the right label
}

TEST(bar_takes_clicks_only_while_shown) {
    SeekBar b = placed(120, 100, 30);
    CHECK(!b.hit(29));                            // hidden: picture underneath
    b.render(10, false);
    CHECK(b.hit(29));
    CHECK(!b.hit(28));
    b.clear();
    CHECK(!b.hit(29));
}

TEST(bar_without_track_takes_no_clicks) {
    SeekBar b = placed(0, 100, 30);
    b.render(10, false);
    CHECK(!b.hit(29));
}

// ---- SeekBar output ----------------------------------------------------------

TEST(bar_fills_exactly_the_row) {
    for (int width : {18, 22, 40, 80, 100, 213}) {
        SeekBar b = placed(4000, width, 30);      // hour labels: the widest
        CHECK(b.render(1234, false));
        CHECK_EQ(visible(b.buf).size(), (std::size_t)width);
    }
    SeekBar b = placed(0, 50, 30);                // no track: padded instead
    CHECK(b.render(5, true));
    CHECK_EQ(visible(b.buf).size(), 50u);
}

TEST(bar_stays_in_its_row_when_playback_overruns_the_length) {
    // A length reported just short of an hour, and playback past it: the time
    // label must not grow to "1:00:00" and push the bar off the row.
    SeekBar b = placed(3599.9, 80, 24);
    CHECK(b.render(3700, false));
    CHECK_EQ(visible(b.buf).size(), 80u);
    CHECK_EQ(indexOf(visible(b.buf), kKnob), b.trackX + b.trackW - 1);
}

TEST(bar_content) {
    SeekBar b = placed(120, 100, 30);
    CHECK(b.render(60, false));
    CHECK_EQ(b.buf.rfind("\x1b[30;1H", 0), 0u);   // starts on the bottom row
    const auto cells = visible(b.buf);
    CHECK_EQ(cells[1], "▶");                 // ▶ playing
    CHECK_EQ(std::string(cells[3] + cells[4] + cells[5] + cells[6]), "1:00");
    CHECK_EQ(count(cells, kKnob), 1);
    CHECK_EQ(count(cells, kFill) + count(cells, kKnob) + count(cells, kRest), b.trackW);
    CHECK_EQ(b.buf.substr(b.buf.size() - 4), "\x1b[0m");
}

TEST(bar_knob_tracks_position) {
    SeekBar b = placed(120, 100, 30);
    b.render(0, false);
    CHECK_EQ(indexOf(visible(b.buf), kKnob), b.trackX);
    b.render(120, false);
    CHECK_EQ(indexOf(visible(b.buf), kKnob), b.trackX + b.trackW - 1);
    b.render(60, false);
    CHECK_NEAR(indexOf(visible(b.buf), kKnob), b.trackX + (b.trackW - 1) / 2.0, 1.0);
}

TEST(bar_time_label_is_right_aligned) {
    SeekBar b = placed(700, 100, 30);             // "11:40": five columns
    b.render(5, false);
    const auto cells = visible(b.buf);
    CHECK_EQ(std::string(cells[3] + cells[4] + cells[5] + cells[6] + cells[7]), " 0:05");
}

TEST(bar_paused_glyph) {
    SeekBar b = placed(120, 100, 30);
    b.render(10, true);
    CHECK_EQ(visible(b.buf)[1], "‖");        // ‖
}

TEST(bar_redraws_only_on_change) {
    SeekBar b = placed(120, 100, 30);
    CHECK(b.render(10.0, false));
    CHECK(!b.render(10.4, false));                // same second, same knob
    CHECK(b.render(11.0, false));                 // new second
    CHECK(b.render(11.0, true));                  // paused
    CHECK(!b.render(11.0, true));
    b.clear();
    CHECK(b.render(11.0, true));                  // cleared: draw it again
}

TEST(bar_redraws_after_place) {
    SeekBar b = placed(120, 100, 30);
    CHECK(b.render(10, false));
    b.place(80, 24);
    CHECK(b.render(10, false));
    CHECK_EQ(b.buf.rfind("\x1b[24;1H", 0), 0u);
}

TEST(bar_monochrome_has_no_colour) {
    SeekBar b = placed(120, 100, 30, ColorMode::None);
    b.render(10, false);
    CHECK(b.buf.find("\x1b[38") == std::string::npos);
    CHECK(b.buf.find("\x1b[48") == std::string::npos);
    CHECK(b.buf.find("\x1b[0;48") == std::string::npos);
}

TEST(bar_too_narrow_draws_nothing) {
    SeekBar b = placed(120, 5, 30);
    CHECK(!b.render(10, false));
    CHECK(!b.shown);
}

TEST(bar_clear_blanks_its_row) {
    SeekBar b = placed(120, 100, 30);
    b.render(10, false);
    CHECK_EQ(b.clear(), std::string("\x1b[30;1H\x1b[0m\x1b[2K"));
    CHECK(!b.shown);
}
