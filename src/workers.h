//
//  workers.h
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// A fixed set of threads that splits one job at a time between them.  Started
// once and reused for every frame, rather than creating threads per frame.
#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// More threads than this buys nothing for a terminal-sized grid and only
// costs memory and scheduling.
constexpr int kMaxThreads = 256;

class WorkerPool {
public:
    // Up to n - 1 threads; the caller of run() is the n-th worker.  A thread
    // the system will not start just leaves the pool smaller.
    explicit WorkerPool(int n);
    ~WorkerPool();
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    int size() const { return (int)threads_.size() + 1; }

    // Calls part(i) once for every i in [0, size()), spread over the pool and
    // the calling thread, and returns when all have finished.
    void run(const std::function<void(int)>& part);

private:
    void serve(int index);

    std::vector<std::thread> threads_;
    std::mutex m_;
    std::condition_variable go_, done_;
    const std::function<void(int)>* job_ = nullptr;
    long generation_ = 0;      // one per run(): what wakes the threads
    int  pending_ = 0;         // threads yet to finish this run's part
    bool stopping_ = false;
};
