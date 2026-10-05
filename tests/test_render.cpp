//
//  test_render.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Renderer: frame diffing, and leaving the seek bar's row alone.
#include "check.h"
#include "asciiart.h"

namespace {

const GlyphSet& glyphs() {
    static const GlyphSet gs = GlyphSet::build("blocks", GLYPH_W, GLYPH_H);
    return gs;
}

// A cols x rows frame of one glyph.
std::vector<Cell> frame(int cols, int rows, int glyph) {
    std::vector<Cell> f((std::size_t)cols * rows);
    for (auto& c : f) c.g = (int16_t)glyph;
    return f;
}

void set(std::vector<Cell>& f, int cols, int x, int y, int glyph) {
    f[(std::size_t)y * cols + x].g = (int16_t)glyph;
}

bool movesTo(const std::string& buf, int row) {      // row is 0-based
    return buf.find("\x1b[" + std::to_string(row + 1) + ";") != std::string::npos;
}

Renderer renderer(int cols, int rows) {
    Renderer r;
    r.mode = ColorMode::None;
    r.reset(cols, rows);
    return r;
}

} // namespace

TEST(render_first_frame_paints_everything) {
    Renderer r = renderer(4, 3);
    CHECK(r.compose(frame(4, 3, glyphs().blankIdx), glyphs()));
    CHECK(movesTo(r.buf, 0));
    CHECK(movesTo(r.buf, 1));
    CHECK(movesTo(r.buf, 2));
}

TEST(render_unchanged_frame_writes_nothing) {
    Renderer r = renderer(4, 3);
    auto f = frame(4, 3, glyphs().blankIdx);
    r.compose(f, glyphs());
    CHECK(!r.compose(f, glyphs()));
    CHECK(r.buf.empty());
}

TEST(render_only_changed_cells) {
    Renderer r = renderer(4, 3);
    auto f = frame(4, 3, glyphs().blankIdx);
    r.compose(f, glyphs());
    set(f, 4, 2, 1, glyphs().fullIdx);
    CHECK(r.compose(f, glyphs()));
    CHECK_EQ(r.buf, "\x1b[2;3H" + glyphs().chars[glyphs().fullIdx]);
}

TEST(render_skips_the_bar_row) {
    Renderer r = renderer(4, 3);
    auto f = frame(4, 3, glyphs().blankIdx);
    r.compose(f, glyphs());
    r.skipRow = 2;
    set(f, 4, 0, 2, glyphs().fullIdx);           // under the bar
    set(f, 4, 0, 0, glyphs().fullIdx);
    CHECK(r.compose(f, glyphs()));
    CHECK(movesTo(r.buf, 0));
    CHECK(!movesTo(r.buf, 2));
}

TEST(render_skip_row_alone_writes_nothing) {
    Renderer r = renderer(4, 3);
    auto f = frame(4, 3, glyphs().blankIdx);
    r.compose(f, glyphs());
    r.skipRow = 1;
    set(f, 4, 3, 1, glyphs().fullIdx);
    CHECK(!r.compose(f, glyphs()));
}

TEST(render_invalidated_row_is_repainted_in_full) {
    Renderer r = renderer(4, 3);
    auto f = frame(4, 3, glyphs().blankIdx);
    r.compose(f, glyphs());
    r.invalidateRow(1);                           // the bar went away
    CHECK(r.compose(f, glyphs()));
    CHECK(!movesTo(r.buf, 0));
    CHECK(movesTo(r.buf, 1));
    CHECK(!movesTo(r.buf, 2));
    CHECK_EQ(r.buf, "\x1b[2;1H" + glyphs().chars[glyphs().blankIdx]
                    + glyphs().chars[glyphs().blankIdx]
                    + glyphs().chars[glyphs().blankIdx]
                    + glyphs().chars[glyphs().blankIdx]);
}

TEST(render_invalidate_out_of_range_is_harmless) {
    Renderer r = renderer(4, 3);
    auto f = frame(4, 3, glyphs().blankIdx);
    r.compose(f, glyphs());
    r.invalidateRow(-1);
    r.invalidateRow(3);
    CHECK(!r.compose(f, glyphs()));
}

TEST(render_reset_clears_skip_row) {
    Renderer r = renderer(4, 3);
    r.skipRow = 2;
    r.reset(4, 3);
    CHECK_EQ(r.skipRow, -1);
}
