# 在这个子目录下找头文件
include_directories(include)

# 把当前目录的源文件编译成 run_v2
add_executable(run_v2
    src/main.cpp
    src/Task.cpp
    src/Manager.cpp
)

# 开启警告
target_compile_options(run_v2 PRIVATE -Wall -Wextra)

# 链接多线程库
target_link_libraries(run_v2 pthread)