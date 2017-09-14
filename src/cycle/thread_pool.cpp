#include "thread_pool.h"

namespace servx {

ThreadPool *ThreadPool::pool = new ThreadPool;

void ThreadPool::init(size_t n) {
    for (size_t i = 0; i != n; ++i) {
        threads.push_back(std::thread([this]{
            std::unique_lock<std::mutex> lock(mtx);
            cond.wait(lock, [this]{ return !tasks.empty(); });
            thread_task_t task = tasks.front();
            tasks.pop();
            lock.unlock();
            task();
        }));
    }
}

void ThreadPool::push(thread_task_t task) {
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(task);
    cond.notify_one();
}

}
