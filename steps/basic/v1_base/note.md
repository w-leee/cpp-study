##
命名空间	ns::name	std::string、std::unique_ptr	命名空间	
嵌套命名空间	ns1::ns2::name	std::this_thread::sleep_for	多层命名空间	
全局作用域	不写前缀，或 ::name	Task、Manager、Priority	全局命名空间	（不写前缀就是全局）
类作用域	Class::name	Manager::add、Task::getId（在 .cpp 里定义时）	类内部	
枚举作用域	Enum::name	Priority::MEDIUM	枚举内部	
对象成员	obj.name	task.markCompleted()、m_tasks	某个对象内部	
指针成员	ptr->name	it->second->getId()	指针指向的对象内部	
##