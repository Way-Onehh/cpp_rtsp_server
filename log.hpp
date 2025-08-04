#pragma once

#include <iostream>
#define LOG(ostream,bool,title,...)\
    do{ \
        time_t ____now = time(__null);\
        char ____time [20] = {};\
        char ____messages [64] = {};\
        strftime(____time,20,"%Y-%m-%d %H:%M:%S",localtime(&____now));\
        sprintf(____messages,__VA_ARGS__);\
        if(bool)\
        ostream<<__FILE__<<" "<<__LINE__;\
        ostream<<" "<<____time<<" --"<<#title<<"-- "<<____messages<<std::endl;\
    } while (0);
#define CLOG(title,bool,...) LOG(std::cout,bool,title,__VA_ARGS__) 
#define DLOG(title,...) LOG(std::cout,0,title,__VA_ARGS__) 
