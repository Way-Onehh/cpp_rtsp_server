#include <chrono>
#include <thread>
#include <utility/threadpool_imp.h>
threadpool_imp::threadpool_imp(size_t threads) : stop(false) {
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

void threadpool_imp::keep()
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

threadpool_imp::~threadpool_imp() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    cv.notify_all(); 
    for (auto& worker : workers) {
        if (worker.joinable())  worker.join(); 
    }
}