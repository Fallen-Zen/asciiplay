//
//  test_workers.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// WorkerPool, and the matcher giving the same picture on any thread count.
#include "check.h"
#include "asciiart.h"
#include "workers.h"

#include <atomic>
#include <cstdlib>

TEST(workers_single_runs_inline) {
    WorkerPool pool(1);
    CHECK_EQ(pool.size(), 1);
    int calls = 0, part = -1;
    pool.run([&](int i) { ++calls; part = i; });
    CHECK_EQ(calls, 1);
    CHECK_EQ(part, 0);
}

TEST(workers_every_part_runs_once) {
    WorkerPool pool(4);
    CHECK_EQ(pool.size(), 4);
    std::vector<std::atomic<int>> hits(4);
    pool.run([&](int i) { hits[(std::size_t)i]++; });
    for (auto& h : hits) CHECK_EQ(h.load(), 1);
}

TEST(workers_survive_many_runs) {
    // Back-to-back runs, as frames are: no part lost, none run twice.
    WorkerPool pool(8);
    std::atomic<long> total{0};
    for (int run = 0; run < 2000; ++run)
        pool.run([&](int i) { total += i + 1; });
    CHECK_EQ(total.load(), 2000L * (8 * 9 / 2));
}

TEST(workers_size_is_clamped) {
    CHECK_EQ(WorkerPool(0).size(), 1);
    CHECK_EQ(WorkerPool(-3).size(), 1);
    CHECK(WorkerPool(kMaxThreads + 100).size() <= kMaxThreads);
}

TEST(matcher_same_picture_on_any_thread_count) {
    const GlyphSet gs = GlyphSet::build("ascii+blocks", 4, 8);
    std::srand(7);
    std::vector<uint8_t> frame;
    std::vector<std::vector<Cell>> results;
    for (int threads : {1, 4, 7}) {
        Options opt;
        opt.threads = threads;
        Engine eng(opt, gs);
        eng.resize(120, 40);                    // 4800 cells: the threaded path
        if (frame.empty()) {
            frame.resize(eng.rgb.size());
            for (auto& b : frame) b = (uint8_t)(std::rand() & 0xff);
        }
        eng.rgb = frame;
        eng.process();
        results.push_back(eng.cells);
    }
    for (std::size_t r = 1; r < results.size(); ++r) {
        bool same = results[r].size() == results[0].size();
        for (std::size_t i = 0; same && i < results[0].size(); ++i)
            same = results[r][i].g == results[0][i].g &&
                   results[r][i].sameStyle(results[0][i], ColorMode::True);
        CHECK(same);
    }
}
