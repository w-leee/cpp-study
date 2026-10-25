#pragma once
#include <string>

enum class Priority
{
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3
};

class Task
{
  private:
    int m_id;
    std::string m_desc;
    bool m_completed;
    Priority m_priority;

  public:
    Task(int id, const std::string& desc, Priority priority = Priority::MEDIUM, bool completed = false);

    int getId() const;
    std::string getDesc() const;
    bool isCompleted() const;
    Priority getPriority() const;
    
    void markCompleted();
};