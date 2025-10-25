#ifndef HTTP_SERVER_THREAD_POOL_H
#define HTTP_SERVER_THREAD_POOL_H

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
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;

public:
    explicit ThreadPool(const size_t num_threads) : stop(false) {

        for (size_t i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(queue_mutex);
                         cv.wait(lock, [this] {
                             return !tasks.empty() || stop; });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = tasks.front();
                        tasks.pop();
                    }
                task();
                }
            });

        }
    }

    void enqueue(const std::function<void()> &task) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(task);
        cv.notify_one();
    }

    ~ThreadPool() {
        {
           std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
            cv.notify_all();
        }
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

};

#endif //HTTP_SERVER_THREAD_POOL_H
