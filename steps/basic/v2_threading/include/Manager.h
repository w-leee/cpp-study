#pragma once
#include <unordered_map>
#include <memory>
#include <string>

/* 
【核心机制】多线程武器库
<thread>：用来创建后台小号。
<mutex>：互斥锁（PV操作），防止两个线程同时改数据导致崩溃。
<atomic>：原子变量，不可分割的安全开关。
*/
#include <thread>
#include <mutex>
#include <atomic>

#include "Task.h"

class Manager 
{
private:
    std::unordered_map<int, std::unique_ptr<Task>> m_tasks;
    int m_next_id = 1;         
    std::string m_file_path;   

    /* 
    【OS 概念】多线程核心三大件
    */
    
    // 1. 线程对象。代表操作系统里实实在在的一个执行流。
    std::thread m_auto_save_thread; 
    
    /* 
    【OS 概念】互斥锁。相当于临界区门的钥匙。
    
    【语法】为什么要加 mutable？因为 show() 函数是 const 的，编译器不允许在 const 函数里修改成员变量。
    但 lock() 会改变锁的内部状态，加 mutable 就是向编译器申请特权：“这把锁是例外，允许在只读函数里改变它”。
    */
    mutable std::mutex m_mutex;     
    
    /* 
    【OS 概念】原子变量（原语操作）。
    
    【核心机制】普通的 bool 多线程同时读写会发生数据竞争（可能读到半真半假的乱码）。
    atomic 保证了读写是一步到位的，不需要加锁（比 mutex 轻量很多）。
    */
    std::atomic<bool> m_running;    

    // 【避坑指南】放在 private，因为它是后台线程的死循环，外部调用会直接卡死主线程。
    void auto_save_loop(); 

public:
    Manager(const std::string& file_path);
    ~Manager(); 

    void add(const std::string& desc, Priority priority = Priority::MEDIUM); 
    void show() const;                 
    void show_sorted() const; 
    void done(int id);                
    void save() const;                
    void load();
    void remove(int id);
};