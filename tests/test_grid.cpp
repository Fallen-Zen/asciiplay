//
//  test_grid.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// capGrid: every grid, however it was derived, ends up within kMaxGrid.
#include "check.h"
#include "asciiart.h"

namespace {

struct Grid { int cols, rows; };

Grid cap(double cols, double rows) {
    Grid g{};
    capGrid(cols, rows, g.cols, g.rows);
    return g;
}

} // namespace

TEST(grid_within_limits_is_rounded_only) {
    Grid g = cap(100, 56.4);
    CHECK_EQ(g.cols, 100);
    CHECK_EQ(g.rows, 56);
    g = cap(kMaxGrid, kMaxGrid);
    CHECK_EQ(g.cols, kMaxGrid);
    CHECK_EQ(g.rows, kMaxGrid);
}

TEST(grid_derived_side_is_capped_keeping_shape) {
    // --cols 1000 on a 100x10000 source: rows derived as 1000 * 100 / 2.
    Grid g = cap(1000, 1000.0 * 100 / 2);
    CHECK_EQ(g.rows, kMaxGrid);
    CHECK_EQ(g.cols, 20);                    // same 1:50 shape
}

TEST(grid_wide_source_is_capped_too) {
    Grid g = cap(1000.0 * 50, 1000);
    CHECK_EQ(g.cols, kMaxGrid);
    CHECK_EQ(g.rows, 20);
}

TEST(grid_extreme_ratio_does_not_overflow) {
    Grid g = cap(1, 1e12);                   // far past INT_MAX before the cap
    CHECK_EQ(g.rows, kMaxGrid);
    CHECK_EQ(g.cols, 1);
    g = cap(1e12, 10);
    CHECK_EQ(g.cols, kMaxGrid);
    CHECK_EQ(g.rows, 1);
}

TEST(grid_never_below_one_cell) {
    Grid g = cap(0.2, 0);
    CHECK_EQ(g.cols, 1);
    CHECK_EQ(g.rows, 1);
}
