#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <algorithm>

namespace miqu {

class ThreadPool {
public:
    static ThreadPool& get() {
        static ThreadPool s_instance;
        return s_instance;
    }

    explicit ThreadPool(size_t threads = 0) : m_stop(false) {
        if (threads == 0) {
            threads = std::max(2u, std::thread::hardware_concurrency());
        }
        for (size_t i = 0; i < threads; ++i) {
            m_workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                        this->m_cv.wait(lock, [this]() {
                            return this->m_stop || !this->m_tasks.empty();
                        });
                        if (this->m_stop && this->m_tasks.empty()) {
                            return;
                        }
                        task = std::move(this->m_tasks.front());
                        this->m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_stop) return;
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_cv;
    bool m_stop;
};

} // namespace miqu
