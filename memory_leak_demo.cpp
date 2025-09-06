#include <iostream>
#include <vector>
#include <memory>
#include <string>

// 演示各种内存泄露场景
class LeakDemo {
public:
    // 场景1: 简单的内存泄露 - 分配但不释放
    static void simpleMemoryLeak() {
        std::cout << "=== 场景1: 简单内存泄露 ===" << std::endl;
        
        // 分配内存但不释放
        int* leaked_int = new int(42);
        char* leaked_array = new char[4096];
        std::string* leaked_string = new std::string("This will be leaked!");
        
        std::cout << "分配了内存但没有释放: " << *leaked_int << std::endl;
        // 注意：这里故意不调用 delete
    }
    
    // 场景2: 数组内存泄露
    static void arrayMemoryLeak() {
        std::cout << "=== 场景2: 数组内存泄露 ===" << std::endl;
        
        // 分配大量对象但不释放
        for (int i = 0; i < 10; ++i) {
            double* leaked_doubles = new double[100];
            // 填充一些数据
            for (int j = 0; j < 100; ++j) {
                leaked_doubles[j] = i * j * 3.14;
            }
            std::cout << "分配了第 " << i + 1 << " 个数组" << std::endl;
        }
        // 注意：这里故意不调用 delete[]
    }
    
    // 场景3: 对象内存泄露
    static void objectMemoryLeak() {
        std::cout << "=== 场景3: 对象内存泄露 ===" << std::endl;
        
        struct LeakedObject {
            std::vector<int> data;
            std::string name;
            
            LeakedObject(const std::string& n) : name(n) {
                data.resize(1000, 42);
                std::cout << "创建对象: " << name << std::endl;
            }
            
            ~LeakedObject() {
                std::cout << "销毁对象: " << name << std::endl;
            }
        };
        
        // 创建对象但不释放
        for (int i = 0; i < 5; ++i) {
            LeakedObject* obj = new LeakedObject("Object_" + std::to_string(i));
            // 故意不调用 delete
        }
    }
    
    // 场景4: 部分释放的内存泄露
    static void partialMemoryLeak() {
        std::cout << "=== 场景4: 部分释放的内存泄露 ===" << std::endl;
        
        std::vector<int*> pointers;
        
        // 分配多个内存块
        for (int i = 0; i < 20; ++i) {
            int* ptr = new int(i * i);
            pointers.push_back(ptr);
        }
        
        // 只释放一半的内存
        for (size_t i = 0; i < pointers.size() / 2; ++i) {
            delete pointers[i];
            pointers[i] = nullptr;
        }
        
        std::cout << "释放了 " << pointers.size() / 2 << " 个内存块，剩余 " 
                  << pointers.size() - pointers.size() / 2 << " 个未释放" << std::endl;
        
        // 注意：剩余的一半内存没有释放
    }
    
    // 场景5: 循环引用导致的内存泄露（使用原始指针模拟）
    static void circularReferenceleak() {
        std::cout << "=== 场景5: 循环引用内存泄露 ===" << std::endl;
        
        struct Node {
            int value;
            Node* next;
            Node* prev;
            
            Node(int v) : value(v), next(nullptr), prev(nullptr) {
                std::cout << "创建节点: " << value << std::endl;
            }
            
            ~Node() {
                std::cout << "销毁节点: " << value << std::endl;
            }
        };
        
        // 创建循环链表
        Node* node1 = new Node(1);
        Node* node2 = new Node(2);
        Node* node3 = new Node(3);
        
        // 建立循环引用
        node1->next = node2;
        node2->next = node3;
        node3->next = node1;  // 循环引用
        
        node1->prev = node3;
        node2->prev = node1;
        node3->prev = node2;
        
        std::cout << "创建了循环引用的链表" << std::endl;
        // 注意：由于循环引用，即使我们失去了对这些节点的引用，它们也不会被自动释放
    }
};

int main() {
    std::cout << "内存泄露演示程序开始..." << std::endl;
    std::cout << "程序将故意创建多种内存泄露场景" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 执行各种内存泄露场景
    LeakDemo::simpleMemoryLeak();
    std::cout << std::endl;
    
    // LeakDemo::arrayMemoryLeak();
    // std::cout << std::endl;
    
    // LeakDemo::objectMemoryLeak();
    // std::cout << std::endl;
    
    // LeakDemo::partialMemoryLeak();
    // std::cout << std::endl;
    
    // LeakDemo::circularReferenceleak();
    // std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "程序即将结束，但分配的内存没有被释放" << std::endl;
    std::cout << "使用 leaks 工具可以检测到这些内存泄露" << std::endl;
    
    return 0;
}