#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
 
class threadpool {
public:
    threadpool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this]  {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        //防止虚假唤醒
                        cv.wait(lock,  [this] { return stop || !tasks.empty();  });
 
                        if (stop && tasks.empty())  return;
                        if (tasks.empty())  continue;
 
                        task = std::move(tasks.front()); 
                        tasks.pop(); 
                    }
                    task();
                }
            });
        }
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
 
        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("ThreadPool已停止");
            tasks.emplace([task]()  { (*task)(); });
        }
        cv.notify_one(); 
        return res;
    }

    void keep()
    {
        while (1)
        {
            std::this_thread::yield();
        }
    }

    ~threadpool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all(); 
        for (auto& worker : workers) {
            if (worker.joinable())  worker.join(); 
        }
    }
    

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;
};
