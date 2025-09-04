#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>


class threadpool_imp {
public:
    threadpool_imp(size_t threads) ;
     ~threadpool_imp();
    void keep();
public:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;
};