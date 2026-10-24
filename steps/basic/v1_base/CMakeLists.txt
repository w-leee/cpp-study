include_directories(include)
add_executable(run_v1
    src/main.cpp
    src/Task.cpp
    src/Manager.cpp
)
target_compile_options(run_v1 PRIVATE -Wall -Wextra)