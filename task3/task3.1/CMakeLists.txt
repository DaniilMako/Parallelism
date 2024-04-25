cmake_minimum_required(VERSION 3.0)
project(task3.1)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

add_executable(task3.1 task3.1.cpp)

target_link_libraries(task PRIVATE Threads::Threads)