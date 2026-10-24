#include "Task.h"

/* 
 * 【语法】初始化列表（冒号后面那部分）
 * 【核心机制】在对象出生、分配内存的瞬间，直接把参数塞进成员变量里。
 * 这比在函数体内赋值效率高，因为省去了一次“先默认初始化，再赋值”的开销。
 * 【避坑指南】初始化列表的顺序，必须和 .h 文件中声明的顺序一致！
 */
Task::Task(int id, const std::string& desc, Priority priority, bool completed) 
    : m_id(id), m_desc(desc), m_priority(priority), m_completed(completed) 
{
}

int Task::getId() const 
{ 
    return m_id; 
}

std::string Task::getDesc() const 
{ 
    return m_desc; 
}

bool Task::isCompleted() const 
{ 
    return m_completed; 
}

Priority Task::getPriority() const 
{ 
    return m_priority; 
}

void Task::markCompleted() 
{ 
    m_completed = true; 
}