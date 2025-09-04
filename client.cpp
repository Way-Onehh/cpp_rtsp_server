#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>
#include <thread>
#include <chrono>
 
#define BUFFER_SIZE 4096
 
class RtspClient {
private:
    int sockfd;
    struct sockaddr_in server_addr;
    std::string session_id;
    std::string rtsp_url;
    int cseq = 1;
    bool keep_alive_active = false;
 
    // 解析URL获取主机和端口 
    void parseUrl(const std::string& url, std::string& host, int& port) {
        size_t start = url.find("//")  + 2;
        size_t colon = url.find(":",  start);
        size_t slash = url.find("/",  start);
        
        host = url.substr(start,  colon - start);
        port = (colon != std::string::npos) ? 
               std::stoi(url.substr(colon  + 1, slash - colon - 1)) : 554;
    }
 
    void createSocket() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
    }
 
    void setupServerAddress(const std::string& host, int port) {
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family  = AF_INET;
        server_addr.sin_port  = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr)  <= 0) {
            perror("Invalid address");
            exit(EXIT_FAILURE);
        }
    }
 
    void connectToServer() {
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }
        std::cout << "Connected to RTSP server at " << rtsp_url << std::endl;
    }
 
    std::string sendRtspRequest(const std::string& request) {
        if (send(sockfd, request.c_str(), request.length(),  0) < 0) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
 
        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
 
        return std::string(buffer, bytes_received);
    }
 
    void parseSessionId(const std::string& response) {
        std::regex session_regex("Session:\\s*(\\w+)(?:;\\s*timeout=(\\d+))?");
        std::smatch match;
        if (std::regex_search(response, match, session_regex)) {
            session_id = match[1];
            std::cout << "[Session] ID=" << session_id;
            if (match[2].matched) {
                std::cout << ", Timeout=" << match[2] << "s";
            }
            std::cout << std::endl;
        }
    }
 
    std::string buildRequest(const std::string& method, 
                            const std::string& uri,
                            const std::vector<std::string>& headers = {}) {
        std::string request = method + " " + uri + " RTSP/1.0\r\n";
        request += "CSeq: " + std::to_string(cseq++) + "\r\n";
        
        if (!session_id.empty())  {
            request += "Session: " + session_id + "\r\n";
        }
        
        for (const auto& header : headers) {
            request += header + "\r\n";
        }
        
        request += "User-Agent: RTSP-TestClient/1.0\r\n\r\n";
        return request;
    }
 
public:
    RtspClient(const std::string& url) : rtsp_url(url) {
        std::string host;
        int port;
        parseUrl(url, host, port);
 
        createSocket();
        setupServerAddress(host, port);
        connectToServer();
    }
 
    ~RtspClient() {
        if (keep_alive_active) {
            keep_alive_active = false;
        }
        teardown();
        close(sockfd);
    }
 
    void options() {
        std::string request = buildRequest("OPTIONS", rtsp_url);
        std::string response = sendRtspRequest(request);
        std::cout << "[OPTIONS Response]\n" << response << std::endl;
        parseSessionId(response);
    }
 
    void describe() {
        std::vector<std::string> headers = {"Accept: application/sdp"};
        std::string request = buildRequest("DESCRIBE", rtsp_url, headers);
        std::string response = sendRtspRequest(request);
        std::cout << "[DESCRIBE Response]\n" << response << std::endl;
        parseSessionId(response);
    }
 
    void setup() {
        std::vector<std::string> headers = {
            "Transport: RTP/AVP;unicast;client_port=8000-8001"
        };
        std::string request = buildRequest("SETUP", rtsp_url + "/trackID=0", headers);
        std::string response = sendRtspRequest(request);
        std::cout << "[SETUP Response]\n" << response << std::endl;
        parseSessionId(response);
 
        // 解析服务器返回的传输参数 
        std::regex transport_regex("Transport:.*server_port=(\\d+)-(\\d+)");
        std::smatch match;
        if (std::regex_search(response, match, transport_regex)) {
            std::cout << "[RTP] Server ports: " << match[1] << " (RTP), " 
                      << match[2] << " (RTCP)" << std::endl;
        }
    }
 
    void play() {
        std::vector<std::string> headers = {"Range: npt=0.000-"};
        std::string request = buildRequest("PLAY", rtsp_url, headers);
        std::string response = sendRtspRequest(request);
        std::cout << "[PLAY Response]\n" << response << std::endl;
 
        // 启动保活线程（间隔25秒）
        keep_alive_active = true;
        std::thread([this]() {
            while (keep_alive_active) {
                std::this_thread::sleep_for(std::chrono::seconds(25));
                this->options();
            }
        }).detach();
    }
 
    void teardown() {
        keep_alive_active = false;
        std::string request = buildRequest("TEARDOWN", rtsp_url);
        std::string response = sendRtspRequest(request);
        std::cout << "[TEARDOWN Response]\n" << response << std::endl;
    }

};
 
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <rtsp_url>" << std::endl;
        std::cerr << "Example: " << argv[0] << " rtsp://127.0.0.1:8554/record/test1" << std::endl;
        return 1;
    }
 
    RtspClient client(argv[1]);
 
    // RTSP标准交互流程
    client.options(); 
    client.describe(); 
    client.setup(); 
    client.play(); 
    
    while (true) {
    
    }
 
    return 0;
}