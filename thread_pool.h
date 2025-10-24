#ifndef UNTITLED_THREAD_POOL_H
#define UNTITLED_THREAD_POOL_H

#include <thread>              // std::thread
#include <queue>               // std::queue
#include <mutex>               // std::mutex
#include <condition_variable>  // std::condition_variable
#include <vector>              // std::vector
#include <functional>          // std::function
#include <string>              // std::string

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::pair<int, std::string>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;

public:
    ThreadPool(size_t num_threads) {
        stop = false;
        for (size_t i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                 // 1. Wait for work
                 // 2. Grab work from queue
                 // 3. Process it
                 // 4. Repeat
                }
            }

        }

    }

#endif //UNTITLED_THREAD_POOL_H
