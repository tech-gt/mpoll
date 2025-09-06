#include <cstddef>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <mutex>
#include <memory>
#include <cstring>

// 基础内存池接口
template<typename T>
class MemoryPoolBase {
public:
    virtual ~MemoryPoolBase() = default;
    virtual T* allocate() = 0;
    virtual void deallocate(T* ptr) = 0;
};

// T 必须是 trivially_destructible，因为我们不会调用析构函数
template<typename T>
class LockFreeMemoryPool : public MemoryPoolBase<T> {
private:
    // 内存块节点
    struct Node {
        Node* next;
    };

    // 解决 ABA 问题的标记指针
    struct TaggedPointer {
        Node* ptr;
        uintptr_t tag;

        // 需要默认构造函数以用于 atomic
        TaggedPointer() : ptr(nullptr), tag(0) {}
        TaggedPointer(Node* p, uintptr_t t) : ptr(p), tag(t) {}
    };

    // 底层的大块内存
    void* raw_memory_;
    // 空闲列表的栈顶
    std::atomic<TaggedPointer> head_;
    // 内存池中对象的总数
    const size_t capacity_;

public:
    // 构造函数：分配内存并构建初始的空闲列表
    explicit LockFreeMemoryPool(size_t count);

    // 析构函数：释放内存
    ~LockFreeMemoryPool();

    // 禁止拷贝和移动
    LockFreeMemoryPool(const LockFreeMemoryPool&) = delete;
    LockFreeMemoryPool& operator=(const LockFreeMemoryPool&) = delete;

    // 分配一个对象
    T* allocate();

    // 释放一个对象
    void deallocate(T* ptr);

    // 检查平台是否支持无锁的 TaggedPointer
    static bool is_lock_free() {
        std::atomic<TaggedPointer> dummy;
        return dummy.is_lock_free();
    }
};

// 基于互斥锁的备用实现
template<typename T>
class MutexMemoryPool : public MemoryPoolBase<T> {
private:
    struct Node {
        Node* next;
    };
    
    void* raw_memory_;
    Node* head_;
    std::mutex mutex_;
    const size_t capacity_;
    
public:
    explicit MutexMemoryPool(size_t count) : capacity_(count) {
        static_assert(sizeof(T) >= sizeof(Node), "T must be at least the size of a pointer.");
        
        raw_memory_ = new char[sizeof(T) * count];
        Node* current = reinterpret_cast<Node*>(raw_memory_);
        head_ = current;
        char* current_char = reinterpret_cast<char*>(raw_memory_);
        
        for(size_t i = 0; i < count - 1; i++) {
            Node* next = reinterpret_cast<Node*>(current_char + sizeof(T));
            current->next = next;
            current = next;
            current_char += sizeof(T);
        }
        current->next = nullptr;
    }
    
    ~MutexMemoryPool() {
        delete[] static_cast<char*>(raw_memory_);
    }
    
    T* allocate() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (head_ == nullptr) {
            return nullptr;
        }
        Node* result = head_;
        head_ = head_->next;
        return reinterpret_cast<T*>(result);
    }
    
    void deallocate(T* ptr) override {
        Node* node = reinterpret_cast<Node*>(ptr);
        std::lock_guard<std::mutex> lock(mutex_);
        node->next = head_;
        head_ = node;
    }
};

// 自适应内存池：自动选择最佳实现
template<typename T>
class AdaptiveMemoryPool {
private:
    std::unique_ptr<MemoryPoolBase<T>> pool_;
    
public:
    explicit AdaptiveMemoryPool(size_t count) {
        if (LockFreeMemoryPool<T>::is_lock_free()) {
            pool_ = std::make_unique<LockFreeMemoryPool<T>>(count);
            std::cout << "Using lock-free memory pool implementation." << std::endl;
        } else {
            pool_ = std::make_unique<MutexMemoryPool<T>>(count);
            std::cout << "Using mutex-based memory pool implementation." << std::endl;
        }
    }
    
    T* allocate() {
        return pool_->allocate();
    }
    
    void deallocate(T* ptr) {
        pool_->deallocate(ptr);
    }
};

// 构造函数实现
template<typename T>
LockFreeMemoryPool<T>::LockFreeMemoryPool(size_t count) : capacity_(count) {
    if (!is_lock_free()) {
        // 在不支持128位原子操作的平台（如32位系统）上，这会抛出异常
        throw std::runtime_error("Atomic TaggedPointer is not lock-free on this platform.");
    }

    // 我们需要确保 T 的大小至少是一个指针的大小，以存放 next 指针
    static_assert(sizeof(T) >= sizeof(Node), "T must be at least the size of a pointer.");
    // 确保 T 是 POD 类型或其析构无关紧要
//    static_assert(std::is_trivially_destructible<T>::value, "T must be trivially destructible.");

    // 分配一大块原始内存
    raw_memory_ = new char[sizeof(T) * count];
    Node* current = reinterpret_cast<Node*>(raw_memory_);
    Node* head = current;
    char* current_char = reinterpret_cast<char*>(raw_memory_);
    for(size_t i = 0; i < count -1; i++) {
        Node* next = reinterpret_cast<Node*>(current_char + sizeof(T));
        current->next = next;
        current = next;
        current_char += sizeof(T);
    }
    current->next = nullptr;
    // // 将大块内存链接成一个空闲列表
    // Node* current = reinterpret_cast<Node*>(raw_memory_);
    // Node* head_node = current;

    // for (size_t i = 0; i < count - 1; ++i) {
    //     Node* next_node = reinterpret_cast<Node*>(reinterpret_cast<char*>(current) + sizeof(T));
    //     current->next = next_node;
    //     current = next_node;
    // }
    // current->next = nullptr;

    // 初始化原子栈顶指针
    head_.store(TaggedPointer(head, 0));
}

// 析构函数实现
template<typename T>
LockFreeMemoryPool<T>::~LockFreeMemoryPool() {
    delete[] static_cast<char*>(raw_memory_);
}

// 分配操作 (Lock-Free Pop)
template<typename T>
T* LockFreeMemoryPool<T>::allocate() {
    TaggedPointer old_head;
    TaggedPointer new_head;

    // 使用循环和 CAS 保证原子性
    while (true) {
        // 1. 原子地读取当前 head
        old_head = head_.load();

        // 2. 如果栈为空，则分配失败
        if (old_head.ptr == nullptr) {
            return nullptr;
        }

        // 3. 准备新的 head
        new_head.ptr = old_head.ptr->next;
        new_head.tag = old_head.tag + 1;

        // 4. 尝试用 CAS 更新 head
        // 如果 head_ 的值仍等于 old_head，就将其更新为 new_head 并返回 true
        // 否则，说明有其他线程修改了 head_，此时 CAS 失败，
        // old_head 会被自动更新为 head_ 的最新值，然后循环重试。
        if (head_.compare_exchange_weak(old_head, new_head)) {
            // 成功！返回取出的节点
            return reinterpret_cast<T*>(old_head.ptr);
        }
    }
}

// 释放操作 (Lock-Free Push)
template<typename T>
void LockFreeMemoryPool<T>::deallocate(T* ptr) {
    // 将要释放的内存块转为 Node*
    Node* new_node = reinterpret_cast<Node*>(ptr);
    TaggedPointer old_head;
    TaggedPointer new_head;

    new_head.ptr = new_node;

    while (true) {
        // 1. 原子地读取当前 head
        old_head = head_.load();

        // 2. 将新节点的 next 指向旧的 head
        new_node->next = old_head.ptr;

        // 3. 准备新的 head
        new_head.tag = old_head.tag + 1;

        // 4. 尝试用 CAS 将新节点设为 head
        if (head_.compare_exchange_weak(old_head, new_head)) {
            // 成功！
            return;
        }
    }
}

#include <vector>
#include <cassert>

// 测试用的数据结构
struct MyObject {
    int data;
//   std::string *name = new std::string("hello world nihao sdfafsdfsdffssdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfssdfsdfasdadasdasdsadsadsadasdsadsadasdasdasdadsadssadasdadsasdasdadsadasdasdasda");
    char padding[60]; // 凑够64字节，避免伪共享
    // std::string name;
};

const int THREAD_COUNT = 8;
const int ALLOCATIONS_PER_THREAD = 100000;

void test_worker(AdaptiveMemoryPool<MyObject>& pool) {
    std::vector<MyObject*> allocated_objects;
    allocated_objects.reserve(ALLOCATIONS_PER_THREAD);

    // Phase 1: Allocation
    for (int i = 0; i < ALLOCATIONS_PER_THREAD; ++i) {
        MyObject* obj = pool.allocate();
        if (obj) {
            obj->data = i;
            strncpy(obj->padding, "hello world nihao sdfafsdfsdffssdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfs", sizeof(obj->padding) - 1);
            obj->padding[sizeof(obj->padding) - 1] = '\0'; // 确保字符串以null结尾
            allocated_objects.push_back(obj);
        } else {
            // 如果池耗尽，可以等待或处理
        }
    }

    // Phase 2: Deallocation
    for (MyObject* obj : allocated_objects) {
        assert(obj != nullptr);
        pool.deallocate(obj);
    }
}

using namespace std;
int main() {
    std::cout << "sizeof(int) is " << sizeof(int) << std::endl;
    std::cout << "sizeof(std::string) is " << sizeof(std::string) << std::endl;
    std::cout << "sizeof(MyObject) is " << sizeof(MyObject) << std::endl;

    // 创建自适应内存池（自动选择最佳实现）
    AdaptiveMemoryPool<MyObject> pool(THREAD_COUNT * ALLOCATIONS_PER_THREAD);
    // getchar();
    std::vector<std::thread> threads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back(test_worker, std::ref(pool));
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Test completed successfully." << std::endl;

    // 验证内存池是否已满（所有对象都已释放回来）
    std::vector<MyObject*> final_check;
    for(int i = 0; i < THREAD_COUNT * ALLOCATIONS_PER_THREAD; ++i) {
        final_check.push_back(pool.allocate());
    }
    assert(pool.allocate() == nullptr); // 应该分配完了
    // getchar();

    return 0;
}