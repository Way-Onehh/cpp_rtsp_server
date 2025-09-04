#pragma once


#include "utility/log.hpp"
#include <cstddef>
#include <sstream>
#include <string>
#include <map>
#include <algorithm>
#include <regex>
#include <network/session.hpp>
//rtsp方法
enum Method
{
    OPTIONS,
    DESCRIBE,
    SETUP,
    PLAY,
    ANNOUNCE,
    RECORD,
    TEARDOWN,
    UNKNOWN
};

//rtsp请求
class request
{
    using Url = std::string;
    using Version = int;
    using Keys = std::map<std::string,std::string>;
    using string = std::string;
public:
    request(){}
    
    int parse(const char* raw_data){
        if (!raw_data) 
        {
            DLOG(RTSP,"%s","Null input data");
            return -1;
        }
 
        std::istringstream iss(raw_data);
        std::string line;
 
        //校验起始行格式
        if (!std::getline(iss, line)) 
        {
            DLOG(RTSP,"%s","Empty RTSP message");
            return -1;
        }

        if (!validate_start_line(line))
        {
            //DLOG(RTSP,"%s","Invalid RTSP start line");
            return -1;
        } 
 
        std::istringstream start_line(line);
        std::string method_str, rtsp_version;

        if (!(start_line >> method_str >> url >> rtsp_version)) {
            DLOG(RTSP,"%s","Malformed RTSP start line");
            return -1;
        }
 
        //校验方法和版本
        method = parse_method(method_str);
        if (method == Method::UNKNOWN) 
        {
            DLOG(RTSP,"%s","Unsupported RTSP method");
            return -1;
        }
        
        version = parse_version(rtsp_version);
 
        //校验头部字段 
        while (std::getline(iss, line) && line.find("v=") != 0) {
            if (line.empty())  continue;  // 跳过空行
            if (line == "\r"|| line == "\r\n") break;
            if (!validate_header_line(line)) 
            {
                DLOG(RTSP,"%s","Invalid header format");
                return -1;
            }
 
            size_t colon_pos = line.find(':'); 
            std::string key = line.substr(0,  colon_pos);
            std::string value = line.substr(colon_pos  + 1);
            
            trim_whitespace(key);
            trim_whitespace(value);
            if (key.empty())
            {
                DLOG(RTSP,"%s","Empty header key");
                return -1;
            } 

            if(key == "CSeq")   CSeq = std::stoi(value);
            else                keys[key] = value;
        }
        iss.seekg(-line.size()-1,std::ios_base::cur);
        std::getline(iss, line);
        //解析负载
        if(keys.count("Content-Length"))
        {
            size_t content_length = std::stoi(keys["Content-Length"]);
            auto remain = iss.rdbuf()->in_avail() ;
            if (remain >= content_length) 
            {
                payload.resize(content_length);                // 预分配空间
                iss.read(&payload[0],  content_length);        // 直接读取指定字节数 
            }else
            {
                return -1;
            }
        }

        return  iss.tellg();
    }

    //序列化为RTSP协议格式的字符串
    std::string serialize() const {
        std::ostringstream oss;
 
        //起始行
        oss << method_to_string(method) << " " << url << " RTSP/" << version << "\r\n";
        if(CSeq != -1)   oss <<"CSeq: "<< CSeq <<"\n";
        //头部键值对
        for (const auto& [key, value] : keys) {
            oss << key << ": " << value << "\r\n";
        }
        
        //空行结束头部 
        oss << "\r\n";

        oss << payload;
        return oss.str(); 
    }

    //没有返回""
    static string getroot(const Url &url)
    {
        return _get_url_(url,3);
    }
    //没有返回""
    static string getstream(const Url &url)
    {
        auto tail = url.substr(url.size()-10);
        if(tail == "/trackID=0" || tail == "/trackID=1"  )
        {   
            return tail;
        }

        tail = url.substr(url.size()-11);
        if(tail == "/streamid=0" ||  tail == "/streamid=1")
        {   
            return tail;
        }
        return "";
    }

    static bool get_port_or_channel(const std::string &Transport,int &p1 ,int &p2,session::type &type)
    {
        std::regex regex(R"(interleaved=(\d+)-(\d+))");
        for(size_t i = 0; i < 2 ;i++)
        {
            std::smatch smatch;
            bool ret = std::regex_search(Transport,smatch,regex);
            if(smatch[1].matched && smatch[2].matched)
            {
                p1 = std::stoi(smatch[1]);
                p2 = std::stoi(smatch[2]);
                type = static_cast<session::type>(i);
                return true;
            }
            regex = R"(client_port=(\d+)-(\d+))";
        }
        return false;
    }

private:
    Method parse_method(const std::string& method_str) {
        static const std::map<std::string, Method> method_map = {
            {"OPTIONS", Method::OPTIONS},
            {"DESCRIBE", Method::DESCRIBE},
            {"PLAY", Method::PLAY},
            {"SETUP", Method::SETUP},
            {"TEARDOWN",Method::TEARDOWN},
            {"ANNOUNCE",Method::ANNOUNCE},
            {"RECORD",Method::RECORD}
        };
        auto it = method_map.find(method_str); 
        return (it != method_map.end())  ? it->second : Method::UNKNOWN;
    }

      //辅助函数：将枚举方法转换为字符串 
    std::string method_to_string(Method m) const {
        static const std::map<Method, std::string> method_strings = {
            {Method::OPTIONS, "OPTIONS"},
            {Method::DESCRIBE, "DESCRIBE"},
            {Method::SETUP, "SETUP"},
            {Method::PLAY, "PLAY"},
            {Method::TEARDOWN, "TEARDOWN"}
        };
        auto it = method_strings.find(m); 
        return (it != method_strings.end())  ? it->second : "UNKNOWN";
    }

     //校验起始行格式（如 "DESCRIBE rtsp://example.com  RTSP/1.0"）
    bool validate_start_line(const std::string& line) const {
        static const std::regex rtsp_start_regex(
            R"((OPTIONS|DESCRIBE|PLAY|SETUP|TEARDOWN|ANNOUNCE|RECORD)\s\S+\s\S+\r$)");
        return std::regex_match(line, rtsp_start_regex);
    }
 
    //校验头部行格式（如 "CSeq: 1"）
    bool validate_header_line(const std::string& line) const {
        return line.find(':')  != std::string::npos;
    }
 
    //解析版本号
    int parse_version(const std::string& ver_str) const {
        if (ver_str.size()  < 8 || ver_str.substr(0,7) != "RTSP/1.") {
            DLOG(RTSP,"%d","Invalid RTSP version format");
            return false;     
        }
        return ver_str.back()  - '0';
    }
 
    //去除首尾空白字符
    void trim_whitespace(std::string& s) const {
        s.erase(s.begin(),  std::find_if(s.begin(),  s.end(),  [](int ch) { 
            return !std::isspace(ch); 
        }));
        s.erase(std::find_if(s.rbegin(),  s.rend(),  [](int ch) { 
            return !std::isspace(ch); 
        }).base(), s.end()); 
    }

static string _get_url_(const string &url, size_t n) 
{
    if (url.empty()  || n == 0) return "";
 
    auto find_slash_pos = [&](size_t target) -> size_t {
        size_t count = 0;
        for (size_t i = 0; i < url.size();  ++i) {
            if (url[i] == '/') {
                if (++count == target) return i;
            }
        }
        return string::npos; // 未找到时返回npos
    };
    
    const size_t start_pos = find_slash_pos(n);
    if (start_pos == string::npos) return ""; // 第n个'/'不存在 
    
    auto tail = url.substr(url.size()-10);
    if(tail == "/trackID=0" || tail == "/trackID=1"  )
    {   
        return url.substr(start_pos  ,url.size() - start_pos  -10);
    }
    tail = url.substr(url.size()-11);
    if(tail == "/streamid=0" ||  tail == "/streamid=1")
    {   
        return url.substr(start_pos ,url.size() - start_pos  -11 );
    }
    return url.substr(start_pos );
}



public:
    Method method;
    Url url;
    Version version;
    Keys keys;
    int CSeq = -1;
    string payload {};
};

class response {
    using Version = int;
    using Code = int;
    using Keys = std::map<std::string, std::string>;
    using string = std::string;
    using Reason = std::string;
 
public:
    response (){}
    //构造函数：从原始RTSP响应数据解析 
    explicit response(const char* raw_data) {
        if (!raw_data) throw std::invalid_argument("Null input data");
 
        std::istringstream iss(raw_data);
        std::string line;
 
        //解析起始行
        if (!std::getline(iss, line)) throw std::runtime_error("Empty RTSP response");
        parse_start_line(line);
 
        //解析头部字段 
        while (std::getline(iss, line) && line != "\r") {
            if (line.empty())  continue;
            parse_header_line(line);
        }
        std::getline(iss, line);
        //解析负载
        if (iss.rdbuf()->in_avail()  > 0) {
            payload.assign(std::istreambuf_iterator<char>(iss),  {});
        }
    }
 
    //序列化为RTSP协议格式字符串
    std::string serialize() const {
        std::ostringstream oss;
 
        //起始行
        oss << "RTSP/1." << version << " " << code << " " << reason << "\r\n";
 
        //头部字段
        if (CSeq != -1) oss << "CSeq: " << CSeq << "\r\n";
        for (const auto& [key, value] : keys) {
            oss << key << ": " << value << "\r\n";
        }
 
        //负载处理 
        oss << "\r\n";
        if (!payload.empty())  oss << payload;
        return oss.str(); 
    }
 
private:
    //解析起始行
    void parse_start_line(const std::string& line) {
        static const std::regex rtsp_response_regex(
            R"(^RTSP/(\d\.\d)\s+(\d+)\s+([^\r]+)\r?$)");
 
        std::smatch matches;
        if (!std::regex_match(line, matches, rtsp_response_regex)) {
            throw std::runtime_error("Invalid RTSP response line format");
        }
 
        version = parse_version(matches[1].str());
        code = std::stoi(matches[2].str());
        reason = matches[3].str();
    }
 
    //解析头部行
    void parse_header_line(const std::string& line) {
        size_t colon_pos = line.find(':'); 
        if (colon_pos == std::string::npos) {
            throw std::runtime_error("Invalid header format: missing colon");
        }
 
        std::string key = line.substr(0,  colon_pos);
        std::string value = line.substr(colon_pos  + 1);
 
        trim_whitespace(key);
        trim_whitespace(value);
 
        if (key.empty())  throw std::runtime_error("Empty header key");
        if (key == "CSeq") {
            CSeq = std::stoi(value);
        } else {
            keys[key] = value;
        }
    }
 
    //解析版本号
    int parse_version(const std::string& ver_str) const {
        if (ver_str.size()  < 3 || ver_str.substr(0,2) != "1.") {
            throw std::runtime_error("Invalid RTSP version format");
        }
        return ver_str.back()  - '0';
    }
 
    // 字符串修剪
    void trim_whitespace(std::string& s) const {
        s.erase(s.begin(),  std::find_if(s.begin(),  s.end(),  [](int ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(),  s.rend(),  [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end()); 
    }

public:
    Version version = 0;  
    Code code = -1;
    Reason reason;
    Keys keys;
    int CSeq = -1;
    string payload;
};
