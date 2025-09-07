#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
public:
    // 构造函数，创建指定数量的工作线程
    ThreadPool(size_t threads);
    // 析构函数，停止并销毁线程池
    ~ThreadPool();

    // 提交任务到任务队列
    // F: 函数类型, Args: 函数参数类型
    // 返回一个 std::future 对象，用于获取任务的返回值
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    // 工作线程的容器
    std::vector<std::thread> workers;
    // 任务队列
    std::queue<std::function<void()>> tasks;

    // 同步机制
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    
    // 工作线程的主循环函数（从lambda提取出来）
    void worker_thread();
};

// 工作线程主循环函数实现
inline void ThreadPool::worker_thread() {
    while(true) {
        std::function<void()> task;
        {
            // 1. 加锁
            std::unique_lock<std::mutex> lock(this->queue_mutex);

            // 2. 等待条件满足：队列不为空 或 线程池停止
            this->condition.wait(lock, [this] {
                return this->stop || !this->tasks.empty();
            });

            // 3. 如果线程池停止且任务队列为空，则线程退出
            if(this->stop && this->tasks.empty()) {
                return;
            }

            // 4. 从队列中取出一个任务
            task = std::move(this->tasks.front());
            this->tasks.pop();
        } // 锁在这里被自动释放

        // 5. 执行任务
        task();
    }
}

// 构造函数实现
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i) {
        // 使用成员函数替代lambda表达式
        workers.emplace_back(&ThreadPool::worker_thread, this);
    }
}

// 析构函数实现
inline ThreadPool::~ThreadPool() {
    {
        // 1. 加锁，设置停止标志
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    // 2. 唤醒所有等待的线程
    condition.notify_all();

    // 3. 等待所有线程执行完毕
    for(std::thread &worker: workers) {
        worker.join();
    }
}

// 任务提交函数实现
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type> {

    // 使用 std::result_of 获取函数 f 的返回类型
    using return_type = typename std::result_of<F(Args...)>::type;

    // 使用 std::packaged_task 来打包任务，它能关联一个 future
    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    // 获取与 packaged_task 相关联的 future
    std::future<return_type> res = task->get_future();
    {
        // 1. 加锁
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 不允许在线程池停止后继续添加任务
        if(stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        // 2. 将任务（一个执行 packaged_task 的 lambda）放入队列
        tasks.emplace([task](){ (*task)(); });
    }

    // 3. 唤醒一个等待的线程
    condition.notify_one();
    return res;
}

#include <chrono>

// 一个简单的函数，返回计算结果
int multiply(int a, int b) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return a * b;
}

// 一个没有返回值的函数
void print_message(const std::string& msg) {
    std::cout << msg << std::endl;
}

int main() {
    // 创建一个包含 4 个工作线程的线程池
    ThreadPool pool(4);

    // 提交多个任务
    // 1. 提交有返回值的任务
    auto future1 = pool.enqueue(multiply, 5, 10);
    auto future2 = pool.enqueue([](int x) { return x * x; }, 8);

    // 2. 提交没有返回值的任务
    pool.enqueue(print_message, "Hello from thread pool!");
    pool.enqueue(print_message, "Another message.");

    // 等待并获取任务的结果
    // future.get() 会阻塞，直到任务完成并返回结果
    std::cout << "Result of 5 * 10 is " << future1.get() << std::endl;
    std::cout << "Result of 8 * 8 is " << future2.get() << std::endl;

    // 主线程可以继续做其他事情
    std::cout << "Main thread is doing other work." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 线程池对象在 main 函数结束时会自动调用析构函数，
    // 从而安全地停止和清理所有工作线程。
    return 0;
}