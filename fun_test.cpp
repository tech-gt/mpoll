#include <iostream>
#include <functional> // for std::function, std::bind
#include <vector>
#include <string>

// 1. 普通函数 (Plain Function)
int plain_multiply(int a, int b) {
    std::cout << "--- [普通函数] plain_multiply ---\n";
    return a * b;
}

// 定义一个类，包含多种函数
class Calculator {
public:
    Calculator(const std::string& name) : instance_name(name) {}

    // 2. 普通成员函数 (Regular Member Function)
    // 它有一个隐藏的 'this' 指针作为第一个参数
    int member_multiply(int a, int b) {
        std::cout << "--- [普通成员函数 on '" << this->instance_name << "'] member_multiply ---\n";
        return a * b;
    }

    // 3. 类静态成员函数 (Static Member Function)
    // 它没有 'this' 指针，行为像一个普通函数
    static int static_multiply(int a, int b) {
        std::cout << "--- [类静态成员函数] static_multiply ---\n";
        return a * b;
    }

private:
    std::string instance_name;


};


struct President
{
    std::string name;
    std::string country;
    int year;

    President(std::string p_name, std::string p_country, int p_year)
            : name(std::move(p_name)), country(std::move(p_country)), year(p_year)
    {
        std::cout << "I am being constructed.\n";
    }

    President(President&& other)
            : name(std::move(other.name)), country(std::move(other.country)), year(other.year)
    {
        std::cout << "I am being moved.\n";
    }
    President(const President& other)
            : name(other.name), country(other.country), year(other.year)
    {
        std::cout << "I am being copied.\n";
    }
    President() = default; // 或者自定义默认构造函数
    President& operator=(const President& other) = default;
};

int main() {
    std::vector<President> reElections;
    reElections.reserve(10);
    std::cout << reElections.capacity() << std::endl;
    std::cout << reElections.size() << std::endl;
    std::cout << "\npush_back:\n";
    reElections.push_back(President("Franklin Delano Roosevelt", "the USA", 1936));
    std::cout << reElections.capacity() << std::endl;
    std::cout << "\npush_back2:\n";
    President p2("hello","beijing", 1991);
    reElections.push_back(p2);
    return 0;
}

int main2() {
    // 我们的目标是把所有可调用实体都统一成这个签名
    using TaskType = std::function<int(int, int)>;

    // 创建一个可以存放这些统一任务的容器
    std::vector<TaskType> tasks;

    std::cout << ">>> 开始准备各种可调用实体...\n\n";

    // 准备阶段 =========================================================

    // a. 直接添加普通函数
    tasks.push_back(plain_multiply);

    // b. 直接添加类静态成员函数 (用法和普通函数一样)
    tasks.push_back(Calculator::static_multiply);
    std::cout << sizeof(&Calculator::member_multiply) << std::endl;
    std::cout << sizeof(&Calculator::static_multiply) << std::endl;
    std::cout << sizeof(&plain_multiply) << std::endl;
    std::cout << sizeof(TaskType) << std::endl;
    // c. 准备 Lambda 表达式
    auto lambda_multiply = [](int a, int b) {
        std::cout << "--- [Lambda 表达式] ---\n";
        return a * b;
    };
    tasks.push_back(lambda_multiply);


    // d. 处理普通成员函数 (最能体现差别的地方)
    // 普通成员函数必须绑定到一个对象实例上才能调用。
    Calculator calc_A("Instance_A");
    Calculator calc_B("Instance_B");

    // d.1 使用 std::bind 将成员函数和对象实例绑定
    // std::bind(&Calculator::member_multiply, &calc_A, ...);
    // 第一个参数：成员函数指针
    // 第二个参数：对象实例的指针或引用
    // 后续参数：使用占位符 std::placeholders::_1, _2 ... 代表调用时传入的参数
    TaskType bind_task = std::bind(&Calculator::member_multiply, &calc_A,
                                   std::placeholders::_1, std::placeholders::_2);
    tasks.push_back(bind_task);

    // d.2 使用 Lambda 表达式捕获对象实例 (现代 C++ 更推荐的做法)
    // lambda 通过捕获列表 `[&calc_B]` 捕获了 calc_B 的引用
    TaskType lambda_bind_task = [&calc_B](int x, int y) {
        return calc_B.member_multiply(x, y);
    };
    tasks.push_back(lambda_bind_task);


    // 执行阶段 =========================================================
    std::cout << "\n>>> 所有实体已统一，开始执行...\n\n";

    int arg1 = 10;
    int arg2 = 5;

    for (size_t i = 0; i < tasks.size(); ++i) {
        std::cout << "Executing Task #" << i << std::endl;
        int result = tasks[i](arg1, arg2); // 以统一的方式调用
        std::cout << "Result: " << result << "\n\n";
    }

    return 0;
}