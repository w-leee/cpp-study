#include "Task.h"

// 【修改】初始化列表加入 m_priority
Task::Task(int id, const std::string& desc, Priority priority, bool completed) 
    : m_id(id), m_desc(desc), m_completed(completed), m_priority(priority) 
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

// 【新增】实现 getPriority
Priority Task::getPriority() const 
{
    return m_priority;
}

void Task::markCompleted() 
{ 
    m_completed = true; 
}