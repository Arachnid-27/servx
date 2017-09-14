#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace servx {

class ThreadPool {
public:
    using thread_task_t = std::function<void()>;

    ThreadPool() = default;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool() = default;

    void init(size_t n);

    void push(thread_task_t task);

    static ThreadPool* instance() { return pool; }

private:
    std::mutex mtx;
    std::condition_variable cond;

    std::vector<std::thread> threads;
    std::queue<thread_task_t> tasks;

    static ThreadPool* pool;
};

}

#endif
