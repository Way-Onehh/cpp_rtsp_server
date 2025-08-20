#pragma once
#include <stdexcept>
#include <future>
#include <utility/threadpool_imp.h>

class threadpool {
public:
    threadpool(size_t threads_num) :  _imp(threads_num) {
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
 
        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(_imp.queue_mutex);
            if (_imp.stop) throw std::runtime_error("ThreadPool已停止");
            _imp.tasks.emplace([task]()  { (*task)(); });
        }
        _imp.cv.notify_one(); 
        return res;
    }
    void keep()
    {
        this->_imp.keep();
    }
private:
    threadpool_imp  _imp;
};
