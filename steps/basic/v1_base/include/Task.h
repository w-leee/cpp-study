#pragma once
#include <string>

/* 
【语法】enum class (C++11 强类型枚举)
传统的 enum 会把名字暴露在外层（容易命名冲突），且能隐式转换成 int。
enum class 必须写 Priority::HIGH，相对安全，不会隐式转换。
正确的用法：Priority priority = Priority::MEDIUM
正确的转换：int pri_num = static_cast<int>(pair.second->getPriority());
注意：static_cast 不校验数值范围，从 int 转回 Priority 时仍需自己检查。
*/
enum class Priority {
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3
};

/*
【命名约定】m_xx 代表成员变量（member variable）
m 是 member 的缩写，m_ 是常见命名前缀，用来和局部变量、函数参数区分开。
m_id、m_desc、m_completed、m_priority 都是 Task 类的成员变量。
*/
class Task
{
private:
    int m_id;
    std::string m_desc;
    bool m_completed;
    Priority m_priority;

public:
/* 
【语法】const std::string& 传参
1. & 代表引用，避免字符串拷贝，不必开辟新的内存空间，直接引用本体。

2. const 承诺在函数内部不修改传入的字符串。
   字面量 "买饭" 不是 std::string，传参时要先隐式构造一个临时 std::string。
   临时对象是右值（rvalue），没有名字，语句结束就销毁。
   std::string& 是非 const 左值引用，只能绑定有名字的左值，不能绑定右值，所以编译失败。
   const std::string& 是 const 左值引用，可以绑定右值。
   const 左值引用绑定临时对象时，会延长临时对象的生命周期。
   加上 const 承诺不修改，所以绑定这个临时对象是安全的。

【语法】completed = false (默认参数)
声明处给了默认值，调用时就可以只传前两个参数。
规则：默认参数必须从右往左连续给，不能跳着给；
所以 completed 放在最后，priority 在它左边也有默认值。
*/
    Task(int id,
         const std::string& desc,
         Priority priority = Priority::MEDIUM,
         bool completed = false);

/* 
【语法】末尾的 const (常量成员函数)
承诺：这些函数只负责读取数据，不会修改类的任何普通成员变量。

因此：
- const 对象只能调用 const 成员函数；
- const 成员函数里不能调用非 const 成员函数；
- 例外：mutable 成员变量在 const 函数里仍可修改（用于缓存/计数等）。

示例：
    private:
        mutable int m_access_count = 0;   // mutable 成员

    public:
        int getId() const {
            ++m_access_count;   // ✅ 合法：const 函数里可以改 mutable 成员
            return m_id;
        }
*/
    int getId() const;
    std::string getDesc() const;
    bool isCompleted() const;
    Priority getPriority() const;

    // 非 const：会修改 m_completed
    void markCompleted();
};