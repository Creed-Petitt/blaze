#ifndef HTTP_SERVER_THREAD_POOL_H
#define HTTP_SERVER_THREAD_POOL_H

#include <thread>              // std::thread
#include <queue>               // std::queue
#include <mutex>               // std::mutex
#include <condition_variable>  // std::condition_variable
#include <vector>              // std::vector
#include <functional>          // std::function
#include <string>              // std::string
#include <utility>             // std::move

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::condition_variable cv_space;
    bool stop;
    size_t max_queue_size_;

public:
    explicit ThreadPool(const size_t num_threads, size_t max_queue_size = 1024)
        : stop(false),
          max_queue_size_(max_queue_size ? max_queue_size : 1024) {

        for (size_t i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(queue_mutex);
                         cv.wait(lock, [this] {
                             return !tasks.empty() || stop; });

                        if (stop && tasks.empty()) {
                            cv_space.notify_all();
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                        cv_space.notify_one();
                    }
                task();
                }
            });

        }
    }

    void enqueue(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv_space.wait(lock, [this] {
            return stop || tasks.size() < max_queue_size_;
        });

        if (stop) {
            return;
        }

        tasks.push(std::move(task));
        cv.notify_one();
    }

    ~ThreadPool() {
        {
           std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
            cv.notify_all();
            cv_space.notify_all();
        }
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

};

#endif //HTTP_SERVER_THREAD_POOL_H
