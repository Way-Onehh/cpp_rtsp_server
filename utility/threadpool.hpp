#pragma once
#include <stdexcept>
#include <future>
#include <utility/threadpool_imp.h>

class threadpool {
public:
    threadpool(size_t threads_num) :  threadpool_imp(threads_num) {
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
 
        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(threadpool_imp.queue_mutex);
            if (threadpool_imp.stop) throw std::runtime_error("ThreadPool已停止");
            threadpool_imp.tasks.emplace([task]()  { (*task)(); });
        }
        threadpool_imp.cv.notify_one(); 
        return res;
    }
    void keep()
    {
        this->threadpool_imp.keep();
    }
private:
    threadpool_imp  threadpool_imp;
};
