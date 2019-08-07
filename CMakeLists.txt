cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -fno-rtti -O3")
project(algorithms-fun)
add_executable(${PROJECT_NAME} "main.cpp")
