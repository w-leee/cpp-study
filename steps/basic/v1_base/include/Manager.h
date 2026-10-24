#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Task.h"

/* 
【核心机制】这是 Manager 类的头文件声明（单线程版本）。
它向外部提供了增删改查、排序、存盘和读盘的接口。
*/

class Manager
{
private:
/* 
【核心机制】任务容器（“大货架”）
底层是 std::unordered_map<int, std::unique_ptr<Task>>：
- 键（Key）：int 类型的任务 id。
- 值（Value）：std::unique_ptr<Task>，指向堆上的 Task 对象。

为什么值用 unique_ptr：
1. 自动管理生命周期（RAII）：Manager 析构时，map 里的 unique_ptr 会自动 delete 对应的 Task，
   避免忘记 delete 导致的内存泄漏。
2. 避免对象复制：map 扩容/rehash 时只移动 unique_ptr，不会复制 Task 对象本身；
   同时 unique_ptr 不可拷贝，从类型层面禁止了 Task 的意外复制。

注意：unordered_map 的查找是“平均 O(1)”，最坏情况（哈希冲突严重）可退化到 O(n)。
*/
    std::unordered_map<int, std::unique_ptr<Task>> m_tasks;

    int m_next_id = 1;

    std::string m_file_path;

public:
    Manager(const std::string& file_path);

/* 
【语法】析构函数
- 名字是 ~类名，没有参数、没有返回值、不能重载。
- 对象销毁时自动调用。
- C++11 起默认是 noexcept：异常不能从析构函数逃出去。

【核心机制】Manager 销毁时自动 save() 存盘。
把“存盘”绑定到对象生命周期上，避免忘记调用 save()。
这是 RAII 思想的一种应用（严格 RAII 是“构造获取、析构释放”）。
*/
    ~Manager();

    void add(const std::string& desc, Priority priority = Priority::MEDIUM);
    void show() const;
    void show_sorted() const;
    void done(int id);
    void save() const;
    void load();
    void remove(int id);
};