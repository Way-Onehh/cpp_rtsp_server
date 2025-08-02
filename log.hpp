#pragma once

#include <iostream>
#define LOG(ostream,bool,title,...)\
    do{ \
        time_t now = time(__null);\
        char time [20] = {};\
        char messages [64] = {};\
        strftime(time,20,"%Y-%m-%d %H:%M:%S",localtime(&now));\
        sprintf(messages,__VA_ARGS__);\
        if(bool)\
        ostream<<__FILE__<<" "<<__LINE__;\
        ostream<<" "<<time<<" --"<<#title<<"-- "<<messages<<std::endl;\
    } while (0);
#define CLOG(title,bool,...) LOG(std::cout,bool,title,__VA_ARGS__) 
#define DLOG(title,...) LOG(std::cout,0,title,__VA_ARGS__) 
