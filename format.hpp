#include <sdp.hpp>
#include <string>
#include <map>
using namespace std;
class format
{
public:
    format(SDP sdp)
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
            data.emplace(streamname,string());
        }
    }  
public:
    SDP sdp;
    map<string,string> data;
};