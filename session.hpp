#pragma once

#include <vector>
#include <channel.hpp>
#include <memory>
#include <format.hpp>
#include <sstream>
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
    Session(int id,Format &format) :id(id),status(INIT),format(format){}
    void addchannel(channel * ch)
    {   
        channels.emplace_back(ch);
    }

    void handle_RECODE(int i,string streamname)
    {
        if(format.streams.size() != channels.size()) std::runtime_error("Channel count does not match the number of format streams");
        char buf[buffer_size] ={};
        int bytes = channels[i]->read(buf,buffer_size);
        auto stream =  format.streams[streamname];
        stream.append(buf,bytes);
    }

    void handle_PLAY(int i,string streamname)
    {
        if(format.streams.size() != channels.size()) std::runtime_error("Channel count does not match the number of format streams");
        auto data =  format.streams[streamname].at(index);
        channels[i]->write(data.data(),data.size());
    }

public:
    Status status;
    int id;
    Format & format;
private:
    std::vector<std::shared_ptr<channel>> channels;
    int index = 0; 
};