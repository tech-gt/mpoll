#include <iostream>
#include <string>
#include <memory>
#include <unistd.h>  // for sleep

struct MyObject {
    std::string name;
    int* data;
    
    MyObject(const std::string& n) : name(n) {
        data = new int[100];  // 分配内存但不释放
        std::cout << "创建对象: " << name << std::endl;
    }
    
    // 故意不实现析构函数，造成内存泄露
};

struct Node {
    int value;
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // 使用weak_ptr避免循环引用
    
    Node(int v) : value(v) {
        std::cout << "创建节点: " << value << std::endl;
    }
};

int main() {
    std::cout << "内存泄露演示程序开始..." << std::endl;
    std::cout << "程序将故意创建多种内存泄露场景" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 场景1: 简单内存泄露
    std::cout << "=== 场景1: 简单内存泄露 ===" << std::endl;
    int* leaked_memory = new int(42);
    std::cout << "分配了内存但没有释放: " << *leaked_memory << std::endl;
    // 故意不调用 delete leaked_memory;
    
    // 场景2: 数组内存泄露
    std::cout << "\n=== 场景2: 数组内存泄露 ===" << std::endl;
    for (int i = 1; i <= 10; ++i) {
        int* array = new int[1000];  // 分配大数组
        std::cout << "分配了第 " << i << " 个数组" << std::endl;
        // 故意不调用 delete[] array;
    }
    
    // 场景3: 对象内存泄露
    std::cout << "\n=== 场景3: 对象内存泄露 ===" << std::endl;
    for (int i = 0; i < 5; ++i) {
        MyObject* obj = new MyObject("Object_" + std::to_string(i));
        // 故意不调用 delete obj;
    }
    
    // 场景4: 部分释放的内存泄露
    std::cout << "\n=== 场景4: 部分释放的内存泄露 ===" << std::endl;
    int** pointers = new int*[20];
    for (int i = 0; i < 20; ++i) {
        pointers[i] = new int(i * 10);
    }
    // 只释放一半
    for (int i = 0; i < 10; ++i) {
        delete pointers[i];
    }
    std::cout << "释放了 10 个内存块，剩余 10 个未释放" << std::endl;
    delete[] pointers;  // 释放指针数组，但剩余的int内存仍然泄露
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "程序将休眠60秒，请在另一个终端使用以下命令检测内存泄露:" << std::endl;
    std::cout << "leaks " << getpid() << std::endl;
    std::cout << "或者使用: ps aux | grep leak_demo_sleep" << std::endl;
    std::cout << "然后: leaks <进程ID>" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 休眠60秒，给用户时间检测内存泄露
    sleep(60);
    
    std::cout << "程序即将结束，分配的内存没有被释放" << std::endl;
    return 0;
}