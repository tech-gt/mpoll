#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <sstream>
#include <ctime>
#include <chrono>

using boost::asio::ip::tcp;
using std::string;

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
    HttpConnection(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    string request(data_, length);
//                    std::cout << "收到请求:\n" << request << std::endl;
                    string response = process_request(request);
                    do_write(response);
                }
            });
    }

    void do_write(const string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    std::cout << "响应发送成功" << std::endl;
                }
                socket_.close();
            });
    }

    string process_request(const string& request) {
        string method, path, version;
        std::istringstream iss(request);
        iss >> method >> path >> version;

        string body;
        if (path == "/") {
            body = "<html><body><h1>Boost.Asio HTTP Server</h1><p>服务器运行正常!</p></body></html>";
        } else if (path == "/api/time") {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            body = "{\"time\":\"" + std::to_string(time_t) + "\"}";
        } else if (path == "/api/compute") {
            // CPU密集型计算任务
            long long result = 0;
            for (int i = 0; i < 1000000; ++i) {
                result += i * i;
            }
            body = "{\"result\":" + std::to_string(result) + ",\"message\":\"CPU intensive computation completed\"}";
        } else {
            body = "<html><body><h1>404 Not Found</h1></body></html>";
        }

        string response = "HTTP/1.1 200 OK\r\n";
        if (path == "/api/time" || path == "/api/compute") {
            response += "Content-Type: application/json; charset=utf-8\r\n";
        } else {
            response += "Content-Type: text/html; charset=utf-8\r\n";
        }
        response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += body;
        return response;
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class HttpServer {
public:
    HttpServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "新连接建立" << std::endl;
                    std::make_shared<HttpConnection>(std::move(socket))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        HttpServer server(io_context, 8080);
        
        std::cout << "HTTP 服务器启动在端口 8080..." << std::endl;
        std::cout << "访问 http://localhost:8080 查看主页" << std::endl;
        
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
    }
    return 0;
}