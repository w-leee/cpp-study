#include "Manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <chrono> // 【新增】时间库，让线程睡觉

// =========================================================================
// 模块一：对象的“出生”与“死亡”（生命周期管理）
// =========================================================================

Manager::Manager(const std::string& file_path) : m_file_path(file_path), m_running(true) 
{
    load();
    // 【OS 概念】相当于系统调用 pthread_create。
    // 我们向系统申请创建一个新的执行流（线程），让它去执行 auto_save_loop 函数。
    // 传 this 指针，是因为 auto_save_loop 是类的成员函数，需要知道操作哪个对象。
    m_auto_save_thread = std::thread(&Manager::auto_save_loop, this);
}


Manager::~Manager() 
{
    // 【OS 概念】相当于发送一个终止信号。
    // 把原子变量设为 false，后台线程的 while(m_running) 判断到后，就会跳出循环。
    m_running = false; 

    // 【OS 概念】相当于父进程执行 wait()。
    // 如果后台线程还没退出，主线程就强行结束，整个程序会崩溃（std::terminate）。
    // join() 会让主线程阻塞，直到后台线程安全跑完最后一圈。
    if (m_auto_save_thread.joinable()) {
        m_auto_save_thread.join(); 
    }
    
    // 退出前，主线程最后再存一次盘（作为最后的兜底）
    try 
    {
        save(); 
        std::cout << "\n[系统] 数据已自动保存至 " << m_file_path << std::endl;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "\n[系统警告] 自动保存失败: " << e.what() << std::endl;
    }
}

// =========================================================================
// 模块二：后台线程的“永动机”
// =========================================================================

void Manager::auto_save_loop() 
{
    // 只要开关没被关掉，就一直循环
    while (m_running) 
    {
        /*【OS 概念】相当于 sleep() 系统调用，让出 CPU 时间片。
         让后台线程睡 5 秒，不要一直空转抢 CPU。
         std::this_thread：表示“当前这个线程”。因为你写在 auto_save_loop 里，而后台线程正在跑这个函数，所以它指的就是后台小号自己。
         std::chrono::seconds(5)：表示“5 秒”。
         chrono 是 C++11 专门处理时间的新库。
         seconds 是时间单位，你也可以换成 milliseconds(500)（500毫秒）或 minutes(1)（1分钟）
        */ 
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 醒来后，先看看是不是主程序要退出了。如果是，立刻跑路。
        if (!m_running) break; 

        // 【OS 概念】进入临界区前，必须加锁（P 操作）。
        // lock_guard 极其聪明，它在离开当前作用域（大括号）时，会自动调用 unlock()（V 操作）。
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 在临界区内安全地存盘
        save(); 
        std::cout << "\n[后台自动保存] 数据已安全落盘...\n";
    }
}

// =========================================================================
// 模块三：前台业务逻辑（全面加锁）
// =========================================================================

void Manager::add(const std::string& desc, Priority priority) 
{
    // 主线程要修改 m_tasks，必须抢到锁！
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_tasks.emplace(m_next_id, std::make_unique<Task>(m_next_id, desc, priority));
    m_next_id++;
    
    // 【注意】这里不再调用 save() 了！彻底解放主线程的性能！
    std::cout << "添加成功！\n";
}

void Manager::done(int id) 
{
    std::lock_guard<std::mutex> lock(m_mutex); 
    auto it = m_tasks.find(id); 
    if (it != m_tasks.end()) 
    {
        it->second->markCompleted(); 
        std::cout << "任务 [" << id << "] 已标记为完成！\n";
    } 
    else 
    {
        std::cout << "未找到 ID 为 " << id << " 的任务。\n";
    }
}

void Manager::remove(int id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int count = m_tasks.erase(id);
    if (count > 0) 
    {
        std::cout << "任务 [" << id << "] 已成功删除！\n";
    } 
    else 
    {
        std::cout << "未找到 ID 为 " << id << " 的任务，无法删除。\n";
    }
}

void Manager::show() const 
{
    // 注意：这里的 m_mutex 在 .h 里加了 mutable，所以这里可以加锁。
    std::lock_guard<std::mutex> lock(m_mutex); 
    
    if (m_tasks.empty()) 
    { 
        std::cout << "当前没有任务。\n"; 
        return; 
    }
    
    for (const auto& pair : m_tasks) 
    {
        std::string status = pair.second->isCompleted() ? "[已完成] " : "[未完成] ";
        std::cout << "[" << pair.first << "] " << status << pair.second->getDesc() << "\n";
    }
}

void Manager::show_sorted() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_tasks.empty()) 
    {
        std::cout << "当前没有任务。\n";
        return;
    }

    std::vector<Task*> task_ptrs;
    for (const auto& pair : m_tasks) 
    {
        task_ptrs.push_back(pair.second.get()); 
    }

    std::sort(task_ptrs.begin(), task_ptrs.end(), 
        [](const Task* a, const Task* b) 
        {
            return a->getPriority() < b->getPriority(); 
        }
    );

    for (const auto* task : task_ptrs) 
    {
        std::string status = task->isCompleted() ? "[已完成] " : "[未完成] ";
        std::string pri_str;
        if (task->getPriority() == Priority::HIGH) pri_str = "[高] ";
        else if (task->getPriority() == Priority::MEDIUM) pri_str = "[中] ";
        else pri_str = "[低] ";

        std::cout << "[" << task->getId() << "] " << pri_str << status << task->getDesc() << "\n";
    }
}

// =========================================================================
// 持久化操作（被调用时，外部已经加好锁了）
// =========================================================================

void Manager::save() const 
{
    std::ofstream file(m_file_path); 
    if (!file) throw std::runtime_error("无法打开文件进行保存: " + m_file_path);

    for (const auto& pair : m_tasks) 
    {
        int pri_num = static_cast<int>(pair.second->getPriority());
        file << pair.first << "|" << pair.second->getDesc() << "|" 
             << (pair.second->isCompleted() ? "1" : "0") << "|" << pri_num << "\n";
    }
}

void Manager::load() 
{
    std::ifstream file(m_file_path);
    if (!file.is_open()) return; 

    m_tasks.clear(); 
    m_next_id = 1;

    std::string line;
    while (std::getline(file, line)) 
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string item;

        try 
        {
            std::getline(ss, item, '|'); 
            int id = std::stoi(item);

            std::getline(ss, item, '|'); 
            std::string desc = item;

            std::getline(ss, item, '|'); 
            bool completed = (item == "1");

            Priority pri = Priority::MEDIUM;
            if (std::getline(ss, item, '|')) 
            {
                int p = std::stoi(item);
                if (p == 1) pri = Priority::HIGH;
                else if (p == 3) pri = Priority::LOW;
            }

            m_tasks.emplace(id, std::make_unique<Task>(id, desc, pri, completed));
            
            if (id >= m_next_id) m_next_id = id + 1;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "【警告】解析出错，跳过此行: " << line << "\n";
        }
    }
}