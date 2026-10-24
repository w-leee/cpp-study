#pragma once
#include <string>

/* 
 * 【语法】enum class (C++11 强类型枚举)
 * 传统的 enum 会把名字暴露在外层（容易命名冲突），且能隐式转换成 int。
 * enum class 必须写 Priority::HIGH，且绝对安全，不会乱转。
 */
enum class Priority {
    HIGH = 1,   // 高
    MEDIUM = 2, // 中
    LOW = 3     // 低
};

class Task 
{
private: 
    int m_id;                  // 任务的身份证号
    std::string m_desc;        // 任务的具体内容
    bool m_completed;          // 任务做没做完
    Priority m_priority;       // 任务的优先级

public:
    /* 
     * 【语法】const std::string&
     * 1. & 代表引用，避免字符串深拷贝（极大提升性能）。
     * 2. const 承诺在函数内部不修改传入的字符串，并且可以接收字面量（如 "买饭"）。
     * 
     * 【语法】completed = false (默认参数)
     * 声明处给了默认值，调用时就可以只传前两个参数。
     */
    Task(int id, const std::string& desc, Priority priority = Priority::MEDIUM, bool completed = false);
    
    /* 
     * 【语法】末尾的 const (常量成员函数)
     * 承诺：这个函数只负责读取数据，绝不修改类的任何成员变量。
     * 如果在一个 const 对象上调用非 const 函数，编译器会直接报错。
     */
    int getId() const;
    std::string getDesc() const;
    bool isCompleted() const;
    Priority getPriority() const;

    // 【核心机制】没有 const 修饰，因为它要修改 m_completed 的值。
    void markCompleted();
};