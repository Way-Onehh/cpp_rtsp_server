#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <future>
#include <utility/threadpool_imp.h>

class threadpool {
public:
    threadpool(size_t threads_num) :  _impl(threads_num) {
    }
    virtual ~threadpool() = default;
    template <typename F, typename... Args>
    auto submit_return_future(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
 
        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(_impl.queue_mutex);
            if (_impl.stop) throw std::runtime_error("ThreadPool已停止");
            _impl.tasks.emplace([task]()  { (*task)(); });
        }
        _impl.cv.notify_one(); 
        return res;
    }
    
    void keep()
    {
        this->_impl.keep();
    }
    //不使用future版本 防止 future析构阻塞
    template <typename F, typename... Args>
    void submit(F&& f, Args&&... args){
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
 
        {
            std::unique_lock<std::mutex> lock(_impl.queue_mutex);
            if (_impl.stop) throw std::runtime_error("ThreadPool已停止");
            _impl.tasks.emplace([task]()  { (*task)(); });
        }
        _impl.cv.notify_one(); 
        return ;
    }

private:
    threadpool_imp  _impl;
};


