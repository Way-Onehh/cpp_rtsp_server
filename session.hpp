#pragma once
#include <log.hpp>
#include <cstddef>
#include <memory>
#include <format.hpp>
#include <rtp.hpp>

template <int buffer_size = 1024>
class Session
{
public:
    enum Status
    {
        INIT,
        SETUP,
        PLAYING,
        RECODE,
        TEARDOWN
    };
    Session(Format &format):status(INIT),format(&format){}

    void handle_RECORD(char * buf , size_t n)
    {
       

    }

public:
    Status status;
    std::shared_ptr<Format> format;
private:
};