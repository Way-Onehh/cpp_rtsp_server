#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <fstream>
class sdp {
public:
    // 从字符串解析 SDP 
    bool parse(const std::string& sdpText) {
        clear(); // 清除现有数据
        
        std::istringstream iss(sdpText);
        std::string line;
        int flag = -1;
        while (std::getline(iss, line)) {
            // 移除可能的回车符 
            if (!line.empty()  && line.back()  == '\r') {
                line.pop_back(); 
            }
            
            if (line.empty())  continue;
            
            // 检查 SDP 行格式 
            if (line.size()  < 3 || line[1] != '=') {
                return false; // 无效格式 
            }
            
            char type = line[0];
            std::string value = line.substr(2); 

            switch (type) {
                case 'v': // 协议版本
                    if (!isValidVersion(value)) return false;
                    version = value;
                    break;
                case 'o': // 源 
                    if (!parseOrigin(value)) return false;
                    break;
                case 's': // 会话名 
                    sessionName = value;
                    break;
                case 'i': // 会话信息
                    sessionInfo = value;
                    break;
                case 'u': // URI
                    uri = value;
                    break;
                case 'e': // 电子邮件 
                    emails.push_back(value); 
                    break;
                case 'p': // 电话号码 
                    phones.push_back(value); 
                    break;
                case 'c': // 连接数据 
                    if (!parseConnectionData(value)) return false;
                    break;
                case 'b': // 带宽信息 
                    if (!parseBandwidth(value)) return false;
                    break;
                case 't': // 时间描述 
                    if (!parseTiming(value)) return false;
                    break;
                case 'r': // 重复时间 
                    repeatTimes.push_back(value); 
                    break;
                case 'z': // 时区调整
                    timeZoneAdjustments = value;
                    break;
                case 'k': // 加密密钥 
                    encryptionKey = value;
                    break;
                case 'a': // 属性 
                    if(flag < 0)
                    attributes.push_back(value); 
                    else
                    mediaDescriptions[flag].attributes.push_back(value);
                    break;
                case 'm': // 媒体描述
                    flag ++;
                    if (!parseMediaDescription(value)) return false;
                    break;
                default:
                    // 未知类型，根据 RFC 4566 应该忽略 
                    break;
            }
        }
        
        // 验证必需字段
        if (version.empty()  || origin.empty()  || sessionName.empty())  {
            return false;
        }
        
        return true;
    }
    
    // 序列化为 SDP 文本 
    std::string serialize() const {
        std::ostringstream oss;
        
        // 必需字段
        oss << "v=" << version << "\r\n";
        oss << "o=" << origin << "\r\n";
        oss << "s=" << sessionName << "\r\n";
        
        // 可选字段 
        if (!sessionInfo.empty())  oss << "i=" << sessionInfo << "\r\n";
        if (!uri.empty())  oss << "u=" << uri << "\r\n";
        
        for (const auto& email : emails) {
            oss << "e=" << email << "\r\n";
        }
        
        for (const auto& phone : phones) {
            oss << "p=" << phone << "\r\n";
        }
        
        if (!connectionData.empty())  {
            oss << "c=" << connectionData << "\r\n";
        }
        
        for (const auto& bw : bandwidths) {
            oss << "b=" << bw.first  << ":" << bw.second  << "\r\n";
        }
        
        for (const auto& timing : timings) {
            oss << "t=" << timing.start  << " " << timing.stop  << "\r\n";
        }
        
        for (const auto& repeat : repeatTimes) {
            oss << "r=" << repeat << "\r\n";
        }
        
        if (!timeZoneAdjustments.empty())  {
            oss << "z=" << timeZoneAdjustments << "\r\n";
        }
        
        if (!encryptionKey.empty())  {
            oss << "k=" << encryptionKey << "\r\n";
        }
        
        for (const auto& attr : attributes) {
            oss << "a=" << attr << "\r\n";
        }
        
        // 媒体描述
        for (const auto& media : mediaDescriptions) {
            oss << "m=" << media.media  << " " << media.port  << " " 
                << media.proto  << " " << media.fmt  << "\r\n";
            
            if (!media.mediaInfo.empty())  {
                oss << "i=" << media.mediaInfo  << "\r\n";
            }
            
            if (!media.connectionData.empty())  {
                oss << "c=" << media.connectionData  << "\r\n";
            }
            
            for (const auto& bw : media.bandwidths)  {
                oss << "b=" << bw.first  << ":" << bw.second  << "\r\n";
            }
            
            if (!media.encryptionKey.empty())  {
                oss << "k=" << media.encryptionKey  << "\r\n";
            }
            
            for (const auto& attr : media.attributes)  {
                oss << "a=" << attr << "\r\n";
            }
        }
        
        return oss.str(); 
    }

    // 清除所有数据 
    void clear() {
        version.clear(); 
        origin.clear(); 
        sessionName.clear(); 
        sessionInfo.clear(); 
        uri.clear(); 
        emails.clear(); 
        phones.clear(); 
        connectionData.clear(); 
        bandwidths.clear(); 
        timings.clear(); 
        repeatTimes.clear(); 
        timeZoneAdjustments.clear(); 
        encryptionKey.clear(); 
        attributes.clear(); 
        mediaDescriptions.clear(); 
    }
    
        // 主函数：生成SDP fmtp字符串 
    static std::string generate_fmtp_from_h264_file(const std::string& file_path) {
        // 打开文件并读取数据 
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open())  return "";
    
        const size_t max_read_size = 100000;  // 最大读取100KB
        std::vector<unsigned char> buffer(max_read_size);
        file.read(reinterpret_cast<char*>(buffer.data()),  max_read_size);
        size_t bytes_read = file.gcount(); 
        buffer.resize(bytes_read); 
    
        // NALU解析状态变量 
        int last_startcode_pos = -1;
        int last_startcode_len = 0;
        bool found_sps = false;
        bool found_pps = false;
        std::vector<unsigned char> sps_data;
        std::vector<unsigned char> pps_data;
        std::string profile_level_id;
    
        // 扫描缓冲区查找起始码 
        for (size_t i = 0; i < buffer.size();)  {
            // 检查4字节起始码：0x00000001
            if (i + 3 < buffer.size()  && 
                buffer[i] == 0x00 && buffer[i+1] == 0x00 && 
                buffer[i+2] == 0x00 && buffer[i+3] == 0x01) {
                
                if (last_startcode_pos != -1) {
                    process_nalu(buffer, last_startcode_pos + last_startcode_len, i,
                                found_sps, found_pps, sps_data, pps_data, profile_level_id);
                }
                last_startcode_pos = i;
                last_startcode_len = 4;
                i += 4;
                continue;
            }
            
            // 检查3字节起始码：0x000001
            if (i + 2 < buffer.size()  && 
                buffer[i] == 0x00 && buffer[i+1] == 0x00 && 
                buffer[i+2] == 0x01) {
                
                if (last_startcode_pos != -1) {
                    process_nalu(buffer, last_startcode_pos + last_startcode_len, i,
                                found_sps, found_pps, sps_data, pps_data, profile_level_id);
                }
                last_startcode_pos = i;
                last_startcode_len = 3;
                i += 3;
                continue;
            }
            
            i++;
        }
    
        // 处理最后一个NALU单元 
        if (last_startcode_pos != -1 && (!found_sps || !found_pps)) {
            process_nalu(buffer, last_startcode_pos + last_startcode_len, buffer.size(), 
                        found_sps, found_pps, sps_data, pps_data, profile_level_id);
        }
    
        // 生成最终结果
        if (found_sps && found_pps) {
            return "profile-level-id=" + profile_level_id + 
                "; sprop-parameter-sets=" + 
                base64_encode(sps_data.data(),  sps_data.size())  + "," +
                base64_encode(pps_data.data(),  pps_data.size()); 
        }
    
        return "";  // 未找到SPS/PPS时返回空字符串 
    }
    
public:
    // SDP 字段
    std::string version;              // v= (协议版本)
    std::string origin;               // o= (源)
    std::string sessionName;          // s= (会话名)
    std::string sessionInfo;          // i=* (会话信息)
    std::string uri;                  // u=* (URI)
    std::vector<std::string> emails;  // e=* (电子邮件地址)
    std::vector<std::string> phones;  // p=* (电话号码)
    std::string connectionData;       // c=* (连接数据)
    std::map<std::string, std::string> bandwidths; // b=* (带宽信息)
    
    struct Timing {
        std::string start;
        std::string stop;
    };

    std::vector<Timing> timings;      // t= (时间描述)
    std::vector<std::string> repeatTimes; // r=* (重复时间)
    std::string timeZoneAdjustments;  // z=* (时区调整)
    std::string encryptionKey;        // k=* (加密密钥)
    std::vector<std::string> attributes; // a=* (属性)

    struct MediaDescription {
        std::string media;            // m= (媒体类型)
        std::string port;             // m= (端口)
        std::string proto;            // m= (传输协议)
        std::string fmt;              // m= (格式列表)
        std::string mediaInfo;        // i=* (媒体标题)
        std::string connectionData;   // c=* (连接数据)
        std::map<std::string, std::string> bandwidths; // b=* (带宽信息)
        std::string encryptionKey;    // k=* (加密密钥)
        std::vector<std::string> attributes; // a=* (属性)
    };

    std::vector<MediaDescription> mediaDescriptions;
private:
    // 验证方法 
    bool isValidVersion(const std::string& v) const {
        try {
            int versionNum = std::stoi(v);
            return versionNum == 0; // SDP 版本目前只能是 0 
        } catch (...) {
            return false;
        }
    }
    
    bool parseOrigin(const std::string& value) {
        // o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
        std::istringstream iss(value);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, ' ')) {
            if (!token.empty())  {
                tokens.push_back(token); 
            }
        }
        
        if (tokens.size()  != 6) {
            return false;
        }
        
        origin = value;
        return true;
    }
    
    bool parseConnectionData(const std::string& value) {
        // c=<nettype> <addrtype> <connection-address>
        std::istringstream iss(value);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, ' ')) {
            if (!token.empty())  {
                tokens.push_back(token); 
            }
        }
        
        if (tokens.size()  < 3) {
            return false;
        }
        
        connectionData = value;
        return true;
    }
    
    bool parseBandwidth(const std::string& value) {
        // b=<bwtype>:<bandwidth>
        size_t colonPos = value.find(':'); 
        if (colonPos == std::string::npos || colonPos == 0 || colonPos == value.length()  - 1) {
            return false;
        }
        
        std::string bwType = value.substr(0,  colonPos);
        std::string bwValue = value.substr(colonPos  + 1);
        
        // 验证带宽值是否为数字 
        if (bwValue.empty()  || !std::all_of(bwValue.begin(),  bwValue.end(),  ::isdigit)) {
            return false;
        }
        
        bandwidths[bwType] = bwValue;
        return true;
    }
    
    bool parseTiming(const std::string& value) {
        // t=<start-time> <stop-time>
        std::istringstream iss(value);
        std::string start, stop;
        
        if (!(iss >> start >> stop)) {
            return false;
        }
        
        // 验证时间是否为数字
        if (start.empty()  || !std::all_of(start.begin(),  start.end(),  ::isdigit) ||
            stop.empty()  || !std::all_of(stop.begin(),  stop.end(),  ::isdigit)) {
            return false;
        }
        
        timings.push_back({start,  stop});
        return true;
    }
    
    bool parseMediaDescription(const std::string& value) {
        // m=<media> <port> <proto> <fmt> ...
        std::istringstream iss(value);
        MediaDescription md;

        if (!(iss >> md.media >> md.port >> md.proto)) {
            return false;
        }

        std::string fmt;
        while (iss >> fmt) {
            if (!md.fmt.empty()) md.fmt += " ";
            md.fmt += fmt;
        }
        
        mediaDescriptions.push_back(md);
        return true;
    }

    // Base64编码函数
    static std::string base64_encode(const unsigned char* data, size_t length) {
        const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
    
        while (length--) {
            char_array_3[i++] = *(data++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
    
                for (i = 0; i < 4; i++) {
                    result += base64_chars[char_array_4[i]];
                }
                i = 0;
            }
        }
    
        if (i) {
            for (j = i; j < 3; j++) {
                char_array_3[j] = 0;
            }
    
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
    
            for (j = 0; j < i + 1; j++) {
                result += base64_chars[char_array_4[j]];
            }
    
            while (i++ < 3) {
                result += '=';
            }
        }
    
        return result;
    }
    
    // 处理NALU单元
    static void process_nalu(const std::vector<unsigned char>& buffer, int start, int end,
                    bool& found_sps, bool& found_pps,
                    std::vector<unsigned char>& sps_data,
                    std::vector<unsigned char>& pps_data,
                    std::string& profile_level_id) {
        if (start >= end || end - start < 2) return;
    
        // 解析NALU头
        unsigned char nalu_header = buffer[start];
        int nal_unit_type = nalu_header & 0x1F;  // 提取低5位 
    
        // 计算负载起始位置和大小 
        int payload_start = start + 1;
        int payload_size = end - payload_start;
    
        if (nal_unit_type == 7 && !found_sps && payload_size >= 3) {
            // 提取SPS数据并生成profile-level-id
            sps_data.assign(buffer.begin()  + payload_start, buffer.begin()  + end);
            char hex_str[7];
            snprintf(hex_str, sizeof(hex_str), "%02X%02X%02X",
                    sps_data[0], sps_data[1], sps_data[2]);
            profile_level_id = hex_str;
            found_sps = true;
        } else if (nal_unit_type == 8 && !found_pps) {
            // 提取PPS数据 
            pps_data.assign(buffer.begin()  + payload_start, buffer.begin()  + end);
            found_pps = true;
        }
    }
    
};
 
 
