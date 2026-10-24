#include "Manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>

Manager::Manager(const std::string& file_path) : m_file_path(file_path)
{
    load();
}

/* 
【核心机制】析构函数默认 noexcept，save() 抛异常会直接 std::terminate 崩溃。
所以用 try-catch 包住 save()，让异常在析构函数内部消化：
- try：save() 成功才打印“已自动保存”，失败直接跳 catch，不会误报。
- catch：捕获 std::exception，用 e.what() 打印原因，只警告、不再抛。
*/
Manager::~Manager()
{
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

void Manager::add(const std::string& desc, Priority priority)
{
/* 
【核心机制】一行完成“创建 Task + 放入 map”：
1. std::make_unique<Task>(m_next_id, desc, priority)
   - 在堆上构造一个 Task 对象，返回 std::unique_ptr<Task>。
   - unique_ptr 独占所有权：同一时刻只有一个 unique_ptr 拥有这个 Task，
     不能拷贝，只能移动；Manager 析构时它会自动 delete Task（RAII）。
   - 用 make_unique 而不是 new：异常安全，且不需要手动 delete。

2. m_tasks.emplace(key, value)
   - 在 unordered_map 里“原地构造”键值对，键是 m_next_id，值是上面那个 unique_ptr。
   - emplace 直接构造，避免先创建临时 pair 再插入的额外移动/拷贝。
   - 如果 key 已存在，emplace 不会覆盖，插入失败（这里 m_next_id 自增，正常不会重复）。

3. 为什么值用 unique_ptr 而不是 Task 本身：
   - map 扩容/rehash 时只移动指针，不复制 Task；
   - unique_ptr 不可拷贝，从类型层面禁止 Task 被意外复制。
*/
    m_tasks.emplace(m_next_id, std::make_unique<Task>(m_next_id, desc, priority));
    m_next_id++;
    save();
    std::cout << "添加成功！\n";
}

void Manager::show() const
{
    if (m_tasks.empty())
    {
        std::cout << "当前没有任务。\n";
        return;
    }

/* 
【语法】范围 for 循环（range-based for）
for (const auto& pair : m_tasks)
- 自动遍历 m_tasks 的每个元素，不用写迭代器。
- m_tasks 是 unordered_map<int, unique_ptr<Task>>，每个元素是键值对 pair。

【语法】const auto& pair
- auto：编译器自动推导元素类型（map 的 value_type）。
- &：引用，不拷贝。这里必须用 &：
      map 里存的是 unique_ptr，unique_ptr 不可拷贝，按值遍历会编译失败。
- const：只读，不修改元素。

【这里的 const 和函数参数 const std::string& 不一样】
- const std::string& 的 const：不只是只读，还负责“允许绑定临时对象”，
  所以 Task(1, "买饭") 才能编译；去掉 const 会编译失败。
- const auto& pair 的 const：主要是配合 show() const 的只读语义，表达意图、防止误改。
  在 show() const 里，即使去掉显式 const，auto& 也会自动推导成 const 引用，仍然能编译。
  所以这里的 const 不是“编译必须”，而是“好习惯 + 意图明确”。

【语法】pair.first / pair.second
- pair.first  ：键（Key），int 类型的任务 id。
- pair.second ：值（Value），unique_ptr<Task>，指向堆上的 Task。
- unique_ptr 重载了 ->，所以 pair.second->getDesc() 等价于 (*(pair.second)).getDesc()。
*/
    for (const auto& pair : m_tasks)
    {
        std::string status = pair.second->isCompleted() ? "[已完成] " : "[未完成] ";
        std::cout << "[" << pair.first << "] " << status << pair.second->getDesc() << "\n";
    }
}

void Manager::show_sorted() const
{
    if (m_tasks.empty())
    {
        std::cout << "当前没有任务。\n";
        return;
    }

/* 
【核心机制】为什么要先拷一份指针到 vector 再排序：
- unordered_map 本身不提供“按值排序”的接口，它的迭代顺序由哈希决定，不是有序的。
- std::sort 需要“随机访问迭代器”，map 的迭代器不满足，不能直接 sort。
- 所以先把每个 Task 的地址收集到 vector 里，对 vector 排序，再按顺序输出。

【核心机制】为什么存 Task* 裸指针，而不是存 Task 或 unique_ptr：
- 存 Task：会拷贝整个 Task（含 string），浪费；而且 Task 可能很大。
- 存 unique_ptr：unique_ptr 不可拷贝，无法放进 vector 做排序（排序要交换元素）。
- 存 Task*：只复制 8 字节地址，排序时只交换指针，不碰 Task 本身，最快。
- 安全性：这些 Task 的真实所有权仍在 m_tasks 的 unique_ptr 里，
  vector 只是“借看”，不负责释放；函数结束时 vector 销毁，Task 不受影响。
- 生命周期：show_sorted() 是 const 成员函数，函数执行期间 m_tasks 不会被修改，
  所以指针不会悬空。

【语法】pair.second.get()
- pair.second 是 unique_ptr<Task>。
- .get() 返回它内部管理的裸指针 Task*，不转移所有权，unique_ptr 仍然拥有对象。
- 对比 .release()：release() 会放弃所有权并返回裸指针，这里绝对不能用 release()。
*/
    std::vector<Task*> task_ptrs;
    for (const auto& pair : m_tasks)
    {
        task_ptrs.push_back(pair.second.get());
    }

/* 
【语法】std::sort + lambda 比较器
std::sort(task_ptrs.begin(), task_ptrs.end(), 比较器)
- 对 vector 做原地排序，默认升序。
- 第三个参数是“比较器”：返回 true 表示 a 应该排在 b 前面。

【语法】std::sort(首迭代器, 尾后迭代器, 比较器)
- 接口收“一对迭代器”，不是收容器，也不是收一个指针：一个指针只表示位置，
  表示不了范围；sort 必须知道从哪排到哪。
- [begin, end) 左闭右开：begin 是第一个元素，end 是最后一个元素的下一个位置。
- 迭代器是“像指针一样的东西”：*it 取值、++it 下一个、it != end 判断到头。
- vector 的迭代器是随机访问迭代器，std::sort 要求随机访问，所以 vector 能排；
  list、map 的迭代器不是随机访问，不能直接用 std::sort。
- 算法只依赖迭代器、不依赖容器，所以 vector、数组、deque 共用同一个 std::sort。

  小例子（迭代器怎么操作）：
      std::vector<int> v = {10, 20, 30};

      std::vector<int>::iterator it = v.begin();   // it 指向第一个元素 10
      std::cout << *it;    // 10，用 * 取出它指向的值
      ++it;                // 移动到下一个元素
      std::cout << *it;    // 20
      ++it;
      std::cout << *it;    // 30

【语法】lambda 表达式
[](const Task* a, const Task* b) { return a->getPriority() < b->getPriority(); }
- []：捕获列表，这里为空，因为不用外部变量。
- (const Task* a, const Task* b)：两个参数都是只读指针，因为排序不修改 Task。
- 返回 a->getPriority() < b->getPriority()：如果 a 的数值小于 b，那么 a 比 b 优先级高，
  a 应该排在 b 前面。
*/
    std::sort(task_ptrs.begin(), task_ptrs.end(),
        [](const Task* a, const Task* b)
        {
            return a->getPriority() < b->getPriority();
        }
    );

/* 
【语法】范围 for + 指针
for (const auto* task : task_ptrs)
- 因为 task_ptrs 里存的是指针 Task*，拷贝一个指针很便宜，所以这里用不用 & 都可以。
- auto* 表示元素是指针；const auto* 表示“指向 const Task 的指针”，
  即 task 是 const Task*，不能通过 task 修改 Task。
- 这里用 const 是因为只是打印，不修改 Task，和 show_sorted() const 的只读语义一致。
*/
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

void Manager::done(int id)
{
/* 
【语法】m_tasks.find(id)
- 按 key 查找，返回迭代器：找到指向该键值对，找不到返回 end()。
- it != m_tasks.end() 是“是否找到”的标准判断。
- it->first 是键（int id）；it->second 是值（unique_ptr<Task>）。
- it->second->markCompleted() 两层 ->：迭代器 → pair.second → Task 成员。

【核心机制】为什么不用 m_tasks[id]：
错误写法示例：
    m_tasks[id]->markCompleted();   // ❌ id 不存在时插入空 unique_ptr，-> 崩溃
*/
    auto it = m_tasks.find(id);
    if (it != m_tasks.end())
    {
        it->second->markCompleted();
        save();
        std::cout << "任务 [" << id << "] 已标记为完成！\n";
    }
    else
    {
        std::cout << "未找到 ID 为 " << id << " 的任务。\n";
    }
}

void Manager::remove(int id)
{
    int count = m_tasks.erase(id);
    if (count > 0)
    {
        save();
        std::cout << "任务 [" << id << "] 已成功删除！\n";
    }
    else
    {
        std::cout << "未找到 ID 为 " << id << " 的任务，无法删除。\n";
    }
}

void Manager::save() const
{
/* 
【语法】std::ofstream file(m_file_path)
- 打开文件用于写入；默认会清空原文件内容再写（不是追加）。
- 文件打不开时，流状态为 fail，不会抛异常（除非开了 exceptions）。

【语法】if (!file) throw std::runtime_error(...)
- ofstream 可以隐式转 bool：打开成功为 true，失败为 false。
- 打开失败就抛异常，让调用者知道“保存失败”，而不是静默返回。
- 这里不是析构函数，抛异常是安全的，会正常传给调用者。

【语法】void save() const
- const 成员函数：只读 m_tasks，不修改 Manager 自身状态。
- 写文件是外部副作用，不违反 const。
- 注意：const 函数里 m_tasks 被视为 const，所以遍历用 const auto&。

【语法】for (const auto& pair : m_tasks)
- 范围 for 遍历 map 每个键值对。
- const auto&：不拷贝，只读；元素含 unique_ptr，不可拷贝，必须用 &。
- pair.first 是键（int id）；pair.second 是值（unique_ptr<Task>）。

【语法】static_cast<int>(pair.second->getPriority())
- getPriority() 返回 Priority（enum class），不能隐式转 int，必须显式转换。
- 存进文件的是 1/2/3。

【核心机制】写入格式：id|desc|completed|priority\n
- 和 load() 的解析顺序一一对应。
- 注意：desc 里如果含 '|'，会破坏格式，读盘时字段错位。
*/
    std::ofstream file(m_file_path);
    if (!file) throw std::runtime_error("无法打开文件进行保存: " + m_file_path);

    for (const auto& pair : m_tasks)
    {
        int pri_num = static_cast<int>(pair.second->getPriority());

        file << pair.first << "|"
             << pair.second->getDesc() << "|"
             << (pair.second->isCompleted() ? "1" : "0") << "|"
             << pri_num << "\n";
    }

/* 
【核心机制】写入后检查流状态（建议加）
- ofstream 写入失败（磁盘满、写入中断）默认不抛异常，只把流状态置为 fail。
- 不检查的话，函数会正常返回，调用者以为保存成功了。
- 加 if (!file) throw ... 才能发现写入失败。
*/
    if (!file)
        throw std::runtime_error("写入文件失败: " + m_file_path);
}

void Manager::load()
{
/* 
【语法】ifstream 打开文件读取；打不开时流为 fail，不抛异常。
        is_open() 判断是否成功，失败直接 return（首次运行没文件是正常的）。
【核心机制】clear() + m_next_id = 1：重新加载前清空旧数据，避免重复 id。
【语法】while (getline(file, line))：逐行读，返回流对象能隐式转 bool，EOF 时为 false。
        stringstream ss(line) 把一行包装成流，getline(ss, item, '|') 按 '|' 切字段。
【核心机制】try-catch 包住整行：任何一步出错（stoi 失败、字段缺失）都跳过该行，
        不影响其他行；catch 里打警告，不中断 load。
【核心机制】优先级向后兼容：先默认 MEDIUM，第 4 段不存在（旧文件）就保持 MEDIUM；
        1→HIGH，3→LOW，其他→MEDIUM（非法值也会静默变 MEDIUM，建议加警告）。
【核心机制】emplace 遇到重复 id 不会覆盖也不会报错，静默丢弃；
        建议检查返回值：auto [it, ok] = emplace(...); if (!ok) 警告。
【核心机制】m_next_id 取 max(m_next_id, id + 1)，保证新 id 大于所有已有 id。
*/
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