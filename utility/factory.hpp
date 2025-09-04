#pragma once
#include <cstddef>
#include <fcntl.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility/signal.hpp>

template<typename  key_t, typename obj_t, typename compare_t = std::less<key_t> >
class factory
{
    std::map<key_t, std::shared_ptr<obj_t>,compare_t>  obj_array;
    std::mutex mtx;
public:
    template<typename type = obj_t,typename... args_t>
    std::shared_ptr<obj_t> create(key_t key,args_t... args)
    {        
        auto ptr = std::make_shared<type>();
        ptr->init(args...);
        std::unique_lock lock(mtx);
        auto it =  obj_array.emplace(key,ptr);
        if(!it.second) throw  std::runtime_error("fail create"); 
        return it.first->second;
    }

    std::shared_ptr<obj_t> insert(key_t key,std::shared_ptr<obj_t> ptr)
    {
        std::unique_lock lock(mtx);
        return obj_array.emplace(key,ptr).first->second;
    }

    size_t count(key_t key)
    {
        std::unique_lock lock(mtx);
        return obj_array.count(key);
    }

    std::shared_ptr<obj_t> at(key_t key)
    {
        std::unique_lock lock(mtx);
        return obj_array.find(key)->second;
    }

    std::shared_ptr<obj_t> remove(key_t key) {
        std::unique_lock lock(mtx);
        auto it = obj_array.find(key); 
        if (it != obj_array.end())  {
            auto obj = it->second;
            obj_array.erase(it); 
            return obj;
        }
        return nullptr;
    }
public:

};