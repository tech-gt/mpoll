# Boost.Asio HTTP 服务器学习项目

这是一个基于 Boost.Asio 的异步 HTTP 服务器示例，用于学习 C++ 异步网络编程。

## 项目特性

- ✅ 异步 I/O 处理
- ✅ 多连接支持
- ✅ HTTP 请求解析
- ✅ JSON API 响应
- ✅ 静态 HTML 页面服务
- ✅ 现代 C++17 语法

## 依赖要求

- **C++ 编译器**: 支持 C++17 标准（GCC 7+, Clang 5+, MSVC 2017+）
- **Boost 库**: 需要安装 Boost.Asio 和 Boost.System
- **CMake**: 版本 3.10 或更高

## 安装 Boost 库

### macOS (使用 Homebrew)
```bash
brew install boost
```

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libboost-all-dev
```

### CentOS/RHEL
```bash
sudo yum install boost-devel
# 或者在较新版本中
sudo dnf install boost-devel
```

## 编译和运行

1. **创建构建目录**
   ```bash
   mkdir build
   cd build
   ```

2. **配置项目**
   ```bash
   cmake ..
   ```

3. **编译**
   ```bash
   make
   ```

4. **运行服务器**
   ```bash
   ./http_server
   ```

5. **访问服务器**
   - 主页: http://localhost:8080/
   - 时间API: http://localhost:8080/api/time

## 项目结构

```
asio-demo/
├── CMakeLists.txt      # CMake 配置文件
├── http_server.cpp     # HTTP 服务器源代码
└── README.md          # 项目说明文档
```

## 代码架构说明

### HttpConnection 类
- 负责处理单个客户端连接
- 使用 `std::enable_shared_from_this` 确保对象生命周期管理
- 异步读取 HTTP 请求
- 解析请求并生成响应
- 异步发送响应

### HttpServer 类
- 负责监听端口和接受新连接
- 为每个新连接创建 `HttpConnection` 实例
- 使用异步接受模式处理多个并发连接

### 主要学习点

1. **异步编程模式**
   - 使用回调函数处理异步操作完成事件
   - `io_context.run()` 事件循环

2. **内存管理**
   - 使用 `shared_ptr` 管理连接对象生命周期
   - `shared_from_this()` 确保异步操作期间对象不被销毁

3. **网络编程**
   - TCP socket 操作
   - HTTP 协议基础实现
   - 缓冲区管理

## API 端点

### GET /
返回 HTML 主页，显示服务器基本信息。

### GET /api/time
返回 JSON 格式的当前服务器时间戳。

**响应示例:**
```json
{
  "time": "1640995200"
}
```

## 扩展建议

学习完基础版本后，可以尝试以下扩展：

1. **添加更多 HTTP 方法支持** (POST, PUT, DELETE)
2. **实现 HTTP 头部解析**
3. **添加静态文件服务**
4. **实现 WebSocket 支持**
5. **添加日志系统**
6. **实现连接池**
7. **添加 HTTPS 支持**

## 常见问题

### Q: 编译时提示找不到 Boost 库
A: 确保已正确安装 Boost 库，并且 CMake 能够找到它们。可以尝试设置 `BOOST_ROOT` 环境变量。

### Q: 服务器启动后无法访问
A: 检查防火墙设置，确保 8080 端口未被其他程序占用。

### Q: 如何修改监听端口
A: 在 `main()` 函数中修改 `HttpServer server(io_context, 8080);` 中的端口号。

## 学习资源

- [Boost.Asio 官方文档](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [Boost.Asio 教程](https://www.boost.org/doc/libs/release/doc/html/boost_asio/tutorial.html)
- [HTTP/1.1 协议规范](https://tools.ietf.org/html/rfc2616)

## 许可证

本项目仅用于学习目的，可自由使用和修改。