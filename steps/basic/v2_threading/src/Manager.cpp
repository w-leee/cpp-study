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
    
    /* 
    【OS 概念】相当于系统调用 pthread_create()创造一个新线程

    【为什么要写这行代码？】
    向操作系统申请创建一个新的执行流（后台线程），让它去执行 auto_save_loop 函数。
    从这一秒开始，程序变成了两个执行流：主线程负责和你交互，后台线程负责偷偷存盘。

    【疑惑 1：为什么不直接写 auto_save_loop()？】
    如果带上 ()，意思是“立刻执行这个函数”。那么主线程会瞬间跳进 while(m_running) 的死循环里卡死，
    连菜单都打印不出来，你就没法操作了。
    传给 std::thread 的必须是“函数地址（代码的坐标）”，而不是“执行结果”。

    【疑惑 2：为什么要加 &Manager:: ？】
    auto_save_loop 是类的非静态成员函数。
    在 C++ 语法中，提取类的成员函数地址，必须加 & 并带上类名，写成 &Manager::auto_save_loop。
    & 的作用是把函数在内存（代码区）的精确坐标抄下来，告诉新线程去这个地址读指令。

    【疑惑 3：为什么还要传 this 指针？】
    this 就是当前对象的内存地址。因为成员函数底层自带一个隐藏参数 this。
    新线程拿到 this 后，才能精准锁定是给“当前这栋房子”干活，而不是给别的对象干活。

    【终极比喻：造房子与雇工人】
    类（Manager） = 造房子的图纸
    对象（mgr） = 当前造出来的实体房子
    成员函数（auto_save_loop） = 怎么扫地、存盘的说明书，放在小区公共图书馆（代码区）里，只有一份。
    成员变量（m_tasks，m_next_id，m_running） = 房子里的私人物品。

    这行代码的真实意图是：
    “系统，给我雇一个新工人（新线程），给他这张图书馆的说明书坐标（&Manager::auto_save_loop），
    再给他这栋房子的唯一钥匙（this），让他去这栋房子里按说明书干活。”
    */
    m_auto_save_thread = std::thread(&Manager::auto_save_loop, this);
}


Manager::~Manager() 
{
    // 【OS 概念】相当于发送一个终止信号。
    /* 1. 触发退出信号
    将原子变量 m_running 置为 false。
    后台线程在 while(m_running) 循环中检测到此信号后，将跳出循环并结束运行。
    */
    m_running = false; 

    /* 
    【OS 概念】join() 相当于父进程执行 wait() 系统调用。

    【关于 joinable()】
    严谨地说，joinable() 返回 true，代表这个线程对象当前正关联着一个活跃的（或者已经结束但尚未被回收的）底层系统线程。
    必须先判断它，因为如果对未关联底层线程（如空对象或已 join 过的对象）调用 join()，程序会直接抛出异常导致崩溃。

    【为什么必须 join】
    如果不等待后台线程结束，主线程直接退出（进程销毁），操作系统会强杀所有子线程。
    如果此时后台线程正在执行 save() 写硬盘，会导致文件损坏或数据丢失。此外，C++ 规定如果 std::thread 对象在销毁时依然是 joinable 状态，程序会直接触发 std::terminate 崩溃。

    【底层动作】
    调用 join() 会让主线程进入阻塞态（Blocked），原地死等，直到后台线程安全跑完最后一圈循环、彻底终止，操作系统回收其资源后，主线程才会被唤醒继续执行。
    */
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
    }
}

// =========================================================================
// 模块三：前台业务逻辑（全面加锁）
// =========================================================================

void Manager::add(const std::string& desc, Priority priority) 
{
    /* 
    【OS 操作系统概念映射】
    1. 临界资源：m_tasks (哈希表) 是被多线程共享的临界资源。
    2. 临界区：对 m_tasks 进行读写的这段代码就是临界区。
    3. 二值信号量：std::mutex 底层是一个初始值为 1 的二值信号量。
    4. P/V 原语操作：
       - lock() 相当于 P 操作（申请资源），将信号量 1 减为 0。若当前已有线程占用（信号量为 0），当前线程进入【阻塞态 (Blocked)】，交出 CPU 时间片排队。
       - unlock() 相当于 V 操作（释放资源），将信号量 0 加为 1，唤醒等待队列中的线程。
    
    【语法拆解】
    1. std::lock_guard<std::mutex> 是一个类模板（保镖类）。
    2. lock 是创建在栈区的一个局部对象。
    3. (m_mutex) 将我们在类中定义的互斥锁实例作为构造参数传入。

    【核心机制：RAII (资源获取即初始化)】
    - 出生即加锁：当 lock 对象被创建时，其构造函数会自动调用 m_mutex.lock() (底层 P 操作)。如果锁被占用，当前线程阻塞。
    - 死亡即解锁：当代码离开当前大括号 { } 作用域时（无论是正常结束、return 还是抛出异常），lock 对象会被自动销毁，其析构函数会自动调用 m_mutex.unlock() (底层 V 操作)。

    【为什么不用手写 lock() 和 unlock()？】
    如果手动调用 lock()，一旦中间发生 return 或抛出异常，unlock() 就会被跳过。
    这会导致锁永远无法释放，其他线程全部卡死，即发生【死锁 (Deadlock)】。
    使用 lock_guard 可以利用 C++ 的栈对象析构机制，保证锁绝对成对出现，彻底杜绝死锁。
    */
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