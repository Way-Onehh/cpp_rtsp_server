#pragma once
#include <sdp.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>

using namespace std;

class stream
{   
public:
    void append(char * buf,size_t n)
    {
        index_arr.push_back(make_pair(last_pos,last_pos+n));
        last_pos += n;
        data.append(buf,n);
    }
    //返回的是拷贝可以避免，扩容失效
    string at(size_t n)
    {
        return data.substr(index_arr[n].first,index_arr[n].second);
    }

    size_t size()
    {
        return index_arr.size();
    }
private:
    string data{};
    vector<pair<size_t,size_t>> index_arr;
    int last_pos = 0;
};

class Format
{
public:
    Format(Sdp sdp)
    {
        if(sdp.mediaDescriptions.empty()) throw runtime_error("no mediaDescriptions");
        for(auto & mdp :sdp.mediaDescriptions)
        {
            auto it = find_if(mdp.attributes.begin(),mdp.attributes.end(),[] (const string & str)->bool
            {
               auto ret = str.find("control:");
               return ret != std::string::npos;
            });
            auto streamname = it->substr(8);
            streams_by_name.emplace(streamname,new stream());
        }
    } 
public:
    Sdp sdp;
    map<string,unique_ptr<stream>> streams_by_name;
    map<uint32_t,unique_ptr<stream>> streams_by_ssrc;
};