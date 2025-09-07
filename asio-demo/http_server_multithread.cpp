#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <sstream>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>

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
                    // 在工作线程中处理请求
                    std::cout << "[线程 " << std::this_thread::get_id() << "] 处理请求" << std::endl;
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
                    std::cout << "[线程 " << std::this_thread::get_id() << "] 响应发送成功" << std::endl;
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
            body = "<html><body><h1>多线程 Boost.Asio HTTP Server</h1><p>服务器运行正常!</p><p>当前处理线程: " + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "</p></body></html>";
        } else if (path == "/api/time") {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            body = "{\"time\":\"" + std::to_string(time_t) + "\",\"thread_id\":\"" + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "\"}";
        } else if (path == "/api/threads") {
            body = "{\"message\":\"多线程服务器\",\"current_thread\":\"" + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "\"}";
        } else if (path == "/api/compute") {
            // CPU密集型计算任务
            long long result = 0;
            for (int i = 0; i < 1000000; ++i) {
                result += i * i;
            }
            body = "{\"result\":" + std::to_string(result) + ",\"message\":\"CPU intensive computation completed\",\"thread_id\":\"" + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "\"}";
        } else {
            body = "<html><body><h1>404 Not Found</h1></body></html>";
        }

        string response = "HTTP/1.1 200 OK\r\n";
        if (path == "/api/time" || path == "/api/threads" || path == "/api/compute") {
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
    HttpServer(boost::asio::io_context& acceptor_io_context, 
               std::vector<std::shared_ptr<boost::asio::io_context>>& worker_io_contexts,
               short port)
        : acceptor_(acceptor_io_context, tcp::endpoint(tcp::v4(), port)),
          worker_io_contexts_(worker_io_contexts),
          next_worker_(0) {
        do_accept();
    }

private:
    void do_accept() {
        // 使用轮询方式选择工作线程的 io_context
        auto& worker_io_context = *worker_io_contexts_[next_worker_];
        next_worker_ = (next_worker_ + 1) % worker_io_contexts_.size();
        
        // 在选定的工作线程 io_context 中创建 socket
        auto socket = std::make_shared<tcp::socket>(worker_io_context);
        
        acceptor_.async_accept(*socket,
            [this, socket](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "[主线程] 新连接建立，分配给工作线程" << std::endl;
                    // 将连接处理任务投递到工作线程
                    boost::asio::post(socket->get_executor(), [socket]() {
                        std::make_shared<HttpConnection>(std::move(*socket))->start();
                    });
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<boost::asio::io_context>>& worker_io_contexts_;
    size_t next_worker_;
};

class ReactorHttpServer {
public:
    ReactorHttpServer(short port, size_t thread_count = std::thread::hardware_concurrency())
        : thread_count_(thread_count) {
        
        std::cout << "启动多线程 HTTP 服务器，使用 " << thread_count_ << " 个工作线程" << std::endl;
        
        // 创建主线程的 io_context (用于接受连接)
        acceptor_io_context_ = std::make_shared<boost::asio::io_context>();
        
        // 创建工作线程的 io_context 和 work guard
        for (size_t i = 0; i < thread_count_; ++i) {
            auto io_context = std::make_shared<boost::asio::io_context>();
            worker_io_contexts_.push_back(io_context);
            // 创建 work guard 防止 io_context 退出
            work_guards_.push_back(std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(io_context->get_executor()));
        }
        
        // 创建服务器
        server_ = std::make_unique<HttpServer>(*acceptor_io_context_, worker_io_contexts_, port);
    }
    
    void run() {
        // 启动工作线程
        std::vector<std::thread> worker_threads;
        for (size_t i = 0; i < thread_count_; ++i) {
            worker_threads.emplace_back([this, i]() {
                std::cout << "工作线程 " << i << " (ID: " << std::this_thread::get_id() << ") 启动" << std::endl;
                worker_io_contexts_[i]->run();
                std::cout << "工作线程 " << i << " 退出" << std::endl;
            });
        }
        
        std::cout << "主线程 (ID: " << std::this_thread::get_id() << ") 开始接受连接" << std::endl;
        
        // 主线程运行接受器
        acceptor_io_context_->run();
        
        // 等待所有工作线程结束
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    void stop() {
        // 先释放 work guards，让工作线程能够退出
        work_guards_.clear();
        
        acceptor_io_context_->stop();
        for (auto& io_context : worker_io_contexts_) {
            io_context->stop();
        }
    }

private:
    size_t thread_count_;
    std::shared_ptr<boost::asio::io_context> acceptor_io_context_;
    std::vector<std::shared_ptr<boost::asio::io_context>> worker_io_contexts_;
    std::vector<std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>> work_guards_;
    std::unique_ptr<HttpServer> server_;
};

int main() {
    try {
        // 创建多线程 Reactor 模式的 HTTP 服务器
        // 使用 CPU 核心数作为工作线程数，也可以手动指定
        ReactorHttpServer server(8080, 4); // 使用 4 个工作线程
        
        std::cout << "多线程 HTTP 服务器启动在端口 8080..." << std::endl;
        std::cout << "访问 http://localhost:8080 查看主页" << std::endl;
        std::cout << "访问 http://localhost:8080/api/time 查看时间 API" << std::endl;
        std::cout << "访问 http://localhost:8080/api/threads 查看线程信息" << std::endl;
        std::cout << "按 Ctrl+C 停止服务器" << std::endl;
        
        server.run();
    } catch (std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
    }
    return 0;
}