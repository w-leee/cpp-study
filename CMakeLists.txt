cmake_minimum_required(VERSION 3.10)
project(study CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 只有根目录才能写 add_subdirectory！
add_subdirectory(steps/basic/v1_base)
add_subdirectory(steps/basic/v2_threading)