# HTTP 服务器对比：单线程 vs 多线程 Reactor 模式

本项目包含两个 HTTP 服务器实现，展示了不同的并发处理模式。

## 文件说明

- `http_server.cpp` - 单线程异步服务器
- `http_server_multithread.cpp` - 多线程 Reactor 模式服务器

## 编译和运行

```bash
# 进入构建目录
cd build

# 编译两个版本
make http_server
make http_server_multithread

# 运行单线程版本
./http_server

# 运行多线程版本
./http_server_multithread
```

## 架构对比

### 单线程版本 (`http_server.cpp`)

**特点：**
- 使用 1 个线程（主线程）
- 单个 `io_context` 处理所有 I/O 操作
- 异步 I/O 模型，事件驱动

**优势：**
- 简单的架构，无线程同步问题
- 内存占用较低
- 适合 I/O 密集型应用

**适用场景：**
- 轻量级服务
- I/O 密集型任务
- 对内存使用敏感的环境

### 多线程 Reactor 模式 (`http_server_multithread.cpp`)

**特点：**
- 使用 5 个线程（1 个主线程 + 4 个工作线程）
- 主线程负责接受连接
- 工作线程负责处理 I/O 操作
- 使用轮询方式分配连接到工作线程

**优势：**
- 更好的并发性能
- 充分利用多核 CPU
- 请求处理可以并行进行

**适用场景：**
- 高并发服务
- CPU 密集型任务
- 多核服务器环境

## 测试端点

两个服务器都支持以下端点：

- `GET /` - 主页，显示服务器信息
- `GET /api/time` - 返回当前时间戳（JSON 格式）
- `GET /api/threads` - 返回线程信息（仅多线程版本）

## 性能测试

可以使用以下命令进行简单的性能测试：

```bash
# 使用 curl 测试
curl http://localhost:8080
curl http://localhost:8080/api/time
curl http://localhost:8080/api/threads

# 使用 ab 进行压力测试
ab -n 1000 -c 10 http://localhost:8080/

# 使用 wrk 进行压力测试
wrk -t4 -c100 -d30s http://localhost:8080/
```

## 线程验证

查看服务器运行时的线程数：

```bash
# 查看单线程版本（应该显示 2 行，包含标题）
ps -M $(pgrep -f "http_server$") | wc -l

# 查看多线程版本（应该显示 6 行，包含标题）
ps -M $(pgrep -f http_server_multithread) | wc -l
```

## 实现细节

### Reactor 模式核心组件

1. **事件分离器（Event Demultiplexer）**: `boost::asio::io_context`
2. **事件处理器（Event Handler）**: `HttpConnection` 类
3. **反应器（Reactor）**: `HttpServer` 和 `ReactorHttpServer` 类
4. **工作线程池**: 多个工作线程运行独立的 `io_context`

### 关键技术点

- **Work Guard**: 使用 `executor_work_guard` 保持工作线程运行
- **负载均衡**: 轮询方式分配连接到不同工作线程
- **异步 I/O**: 所有网络操作都是非阻塞的
- **RAII**: 使用智能指针管理资源生命周期

## 注意事项

- 多线程版本需要更多内存和系统资源
- 在低并发情况下，单线程版本可能性能更好
- 线程数量可以根据 CPU 核心数调整
- 生产环境建议添加错误处理和日志记录