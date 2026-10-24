#include <iostream>
#include <string>
#include "Manager.h"

int main() 
{
    Manager mgr("tasks.db"); 
    int choice;

    while (true) 
    {
        std::cout << "\n=== 待办事项 ===\n";
        std::cout << "1. 查看任务\n";
        std::cout << "2. 添加任务\n";
        std::cout << "3. 标记完成\n";
        std::cout << "4. 删除任务\n";
        std::cout << "5. 按优先级排序查看\n"; // 【新增】
        std::cout << "6. 退出\n";
        std::cout << "请选择: ";
        
        std::cin >> choice;

        if (choice == 1) 
        {
            mgr.show();
        } 
        else if (choice == 2) 
        {
            std::cout << "输入任务描述: ";
            std::string desc;
            std::cin.ignore(); 
            std::getline(std::cin, desc); 

            // 【新增】优先级选择逻辑
            std::cout << "设置优先级 (1=高, 2=中, 3=低): ";
            int p;
            std::cin >> p;
            
            Priority pri = Priority::MEDIUM;
            if (p == 1) pri = Priority::HIGH;
            else if (p == 3) pri = Priority::LOW;

            mgr.add(desc, pri);
        } 
        else if (choice == 3) 
        {
            std::cout << "输入要完成的任务 ID: ";
            int id;
            std::cin >> id;
            mgr.done(id);
        }
        else if (choice == 4) 
        {
            std::cout << "输入要删除的任务 ID: ";
            int id;
            std::cin >> id;
            mgr.remove(id);
        }
        else if (choice == 5) // 【新增】
        {
            mgr.show_sorted();
        }
        else if (choice == 6) 
        {
            std::cout << "正在退出...\n";
            break; 
        } 
        else 
        {
            std::cout << "输入错误，请重新选。\n";
        }
    }
    return 0;
}