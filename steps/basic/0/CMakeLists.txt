# 【注意】这里的相对路径，是相对于当前文件（steps/basic/0/）的
include_directories(include)

add_executable(run
    src/main.cpp
    src/Task.cpp
    src/Manager.cpp
)

target_compile_options(run PRIVATE -Wall -Wextra)