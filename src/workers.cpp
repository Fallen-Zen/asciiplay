//
//  workers.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
#include "workers.h"

#include <algorithm>
#include <system_error>

WorkerPool::WorkerPool(int n) {
    n = std::min(std::max(n, 1), kMaxThreads);
    threads_.reserve((std::size_t)n - 1);
    for (int i = 1; i < n; ++i) {
        try {
            threads_.emplace_back([this, i] { serve(i); });
        } catch (const std::system_error&) {
            break;                      // out of threads: work with what we have
        }
    }
}

WorkerPool::~WorkerPool() {
    {
        std::lock_guard<std::mutex> lock(m_);
        stopping_ = true;
    }
    go_.notify_all();
    for (auto& t : threads_) t.join();
}

void WorkerPool::run(const std::function<void(int)>& part) {
    if (threads_.empty()) { part(0); return; }
    {
        std::lock_guard<std::mutex> lock(m_);
        job_ = &part;
        pending_ = (int)threads_.size();
        ++generation_;
    }
    go_.notify_all();
    part(0);

    std::unique_lock<std::mutex> lock(m_);
    done_.wait(lock, [this] { return pending_ == 0; });
    job_ = nullptr;
}

// Each thread owns one fixed part index, so a run never hands the same part
// out twice and a slow thread cannot pick up work from the next run.
void WorkerPool::serve(int index) {
    long seen = 0;
    for (;;) {
        const std::function<void(int)>* job;
        {
            std::unique_lock<std::mutex> lock(m_);
            go_.wait(lock, [&] { return stopping_ || generation_ != seen; });
            if (stopping_) return;
            seen = generation_;
            job = job_;
        }
        (*job)(index);
        {
            std::lock_guard<std::mutex> lock(m_);
            if (--pending_ == 0) done_.notify_one();
        }
    }
}
