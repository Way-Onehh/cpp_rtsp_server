#include <chrono>
class timer_recoder
{
    ulong * time_;
    std::chrono::steady_clock::time_point start_time;
public:
    timer_recoder()
    {
        reset();
    }

    timer_recoder(ulong & time)
    {
        time_ = &time;
        reset();
    }

    ~timer_recoder()
    {
        finish();
    } 
    
    void bind(ulong & time)
    {
        time_ = &time;
    }

    void reset()
    {
        start_time = std::chrono::steady_clock::now();
    }
    
    void finish()
    {
        *this->time_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
    }

};