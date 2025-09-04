#pragma  once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <utility/threadpool.hpp>
#include <utility>
#define  DEBUG 

// 任务基类
class task_base {
public:
    virtual void execute() = 0;
    virtual ~task_base() = default;
};
 
// 具体任务模板
template <typename ReturnType>
class task : public task_base {
public:
    std::shared_ptr<std::packaged_task<ReturnType()>> slot;
 
    explicit task(std::shared_ptr<std::packaged_task<ReturnType()>> t) 
        : slot(std::move(t)) {}
 
    void execute() override { (*slot)(); }
};
 
// 定时线程池
class timer_threadpool : public threadpool, public std::enable_shared_from_this<timer_threadpool> {
private:
    struct task_wrapper {
        int64_t time;
        std::shared_ptr<task_base> task_ptr;
        bool operator>(const task_wrapper& rhs) const { return time > rhs.time;  }
    };
    std::priority_queue<task_wrapper, std::vector<task_wrapper>, std::greater<task_wrapper>> min_heap_;
    std::mutex heap_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> is_scheduler_running_{false};

    void scheduler_loop() {
    while (true) {
        std::shared_ptr<task_base> task_to_execute;
        int64_t wait_time_ms = 0;
 
        // 阶段1: 获取任务信息（需要锁保护）
        {
            std::unique_lock<std::mutex> lock(heap_mutex_);
            if (min_heap_.empty()) {
                is_scheduler_running_.store(false);
                return;
            }

            const auto& next_task = min_heap_.top();
            auto now = std::chrono::steady_clock::now();
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count(); 
            if (now_ms >= next_task.time)  {
                task_to_execute = next_task.task_ptr; 
                min_heap_.pop();
            } else {
                wait_time_ms = next_task.time  - now_ms;
            }
        }
 
        // 阶段2: 执行任务或等待
        if (task_to_execute) {
                threadpool::submit([task = std::move(task_to_execute)] { 
                task->execute(); 
            });
        } else {
            std::unique_lock<std::mutex> lock(heap_mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(wait_time_ms));
        }
    }
}
public:
    explicit timer_threadpool(size_t threads,size_t  priority_queue_size = 1024) : threadpool(threads)
    {
        std::vector<task_wrapper> v;
        v.reserve(priority_queue_size);
        min_heap_=std::move(std::priority_queue<task_wrapper, std::vector<task_wrapper>, std::greater<task_wrapper>> (std::greater<task_wrapper>(),std::move(v)));
    }
    //注意循环任务要减去自身耗时
    template <class F, class... Args>
    void schedule(int64_t delay_ms, F&& f, Args&&... args) {
        if(delay_ms < 0) return submit(std::forward<F>(f), std::forward<Args>(args)...);

        using return_type = decltype(f(args...));  
        // 封装任务
        auto task_func = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto task_obj = std::make_shared<task<return_type>>(task_func);
        

        // 计算执行时间戳 
        auto exec_time = std::chrono::steady_clock::now() + 
                        std::chrono::milliseconds(delay_ms);
        // 插入优先级队列
        {
            std::lock_guard<std::mutex> lock(heap_mutex_);
            min_heap_.push(task_wrapper{std::chrono::duration_cast<std::chrono::milliseconds>(
            exec_time.time_since_epoch()).count(), task_obj});
        }
 
        // 唤醒调度线程 
        cv_.notify_one();
        if (!is_scheduler_running_.exchange(true)) {
            threadpool::submit([self = shared_from_this()] { 
                self->scheduler_loop(); 
            });
        }
    }

    template <class F, class... Args>
    auto schedule_return_future(int64_t delay_ms, F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        if(delay_ms < 0) return submit_return_future(std::forward<F>(f), std::forward<Args>(args)...);
        using return_type = decltype(f(args...));  
        // 封装任务
        auto task_func = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto task_obj = std::make_shared<task<return_type>>(task_func);
        
        // 计算执行时间戳 
        auto exec_time = std::chrono::steady_clock::now() + 
                        std::chrono::milliseconds(delay_ms);
        // 插入优先级队列
        {
            std::lock_guard<std::mutex> lock(heap_mutex_);
            min_heap_.push(task_wrapper{std::chrono::duration_cast<std::chrono::milliseconds>(
            exec_time.time_since_epoch()).count(), task_obj});
        }
 
        // 唤醒调度线程 
        cv_.notify_one();
        if (!is_scheduler_running_.exchange(true)) {
            threadpool::submit([self = shared_from_this()] { 
                self->scheduler_loop(); 
            });
        }
        return task_func->get_future();
    }
};