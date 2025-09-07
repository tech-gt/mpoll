cmake_minimum_required(VERSION 3.10)
project(AsioHttpServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost 库 (使用新的方式)
find_package(Boost REQUIRED)

# 查找 Threads 库
find_package(Threads REQUIRED)

# 创建可执行文件
add_executable(http_server http_server.cpp)
add_executable(http_server_multithread http_server_multithread.cpp)

# 链接库
target_link_libraries(http_server 
    Boost::boost
    Threads::Threads
)

target_link_libraries(http_server_multithread 
    Boost::boost
    Threads::Threads
)

# 设置编译选项
target_compile_options(http_server PRIVATE -Wall -Wextra)
target_compile_options(http_server_multithread PRIVATE -Wall -Wextra)

# 如果是 Debug 模式，添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(http_server PRIVATE -g -O0)
    target_compile_options(http_server_multithread PRIVATE -g -O0)
else()
    target_compile_options(http_server PRIVATE -O2)
    target_compile_options(http_server_multithread PRIVATE -O2)
endif()