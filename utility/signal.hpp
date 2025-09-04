#pragma once
#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

template <typename... args_t>
class signal {
public:

    template<typename  T>
    int init(T slot)
    {
        return connect(slot);
    }
    
    template<typename  T>
    int connect(T &&slot)
    {
        std::unique_lock ul(mtx);
        current_index++;
        slot_infos.push_back(std::forward<solt_info>({current_index,slot}));
        return current_index;
    }
    
    void disconnect(int index)
    {
        std::unique_lock ul(mtx);
        auto pred = [index](const solt_info&  solt_info)
        {
            return solt_info.index == index;
        };
        auto it = std::find_if(slot_infos.begin(),slot_infos.end(),pred);
        if(it != slot_infos.end())
        slot_infos.erase(it);
    }

    void emit(args_t... args) {
        decltype(slot_infos) tmp;
        {
            std::unique_lock ul(mtx);
            tmp = slot_infos;  // 复制数据 
        }
        for (auto& slot_info : tmp) slot_info.slot(args...);  // 无锁执行回调
    }
    
    operator bool ()
    {
        std::unique_lock ul(mtx);
        return !slot_infos.empty();
    }
private:
    std::mutex mtx;

    struct solt_info 
    {
        int index = 0;
        std::function<void(args_t...)> slot;
    };

    std::vector<solt_info> slot_infos;
    int current_index = -1;
};


template <>
class signal<void> {
public:

    template<typename  T>
    int init(T slot)
    {
        return connect(slot);
    }
    
    template<typename  T>
    int connect(T &&slot)
    {
        std::unique_lock ul(mtx);
        current_index++;
        slot_infos.push_back(std::forward<solt_info>({current_index,slot}));
        return current_index;
    }
    
    void disconnect(int index)
    {
        std::unique_lock ul(mtx);
        auto pred = [index](const solt_info&  solt_info)
        {
            return solt_info.index == index;
        };
        auto it = std::find_if(slot_infos.begin(),slot_infos.end(),pred);
        if(it != slot_infos.end())
        slot_infos.erase(it);
    }

    void emit() {
        std::unique_lock ul(mtx);
        for (auto& slot_info : slot_infos) slot_info.slot(); 
    }
    
    operator bool ()
    {
        std::unique_lock ul(mtx);
        return !slot_infos.empty();
    }
private:
    std::mutex mtx;
    
    struct solt_info 
    {
        int index = 0;
        std::function<void()> slot;
    };

    std::vector<solt_info> slot_infos;
    int current_index = -1;
};