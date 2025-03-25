cmake_minimum_required(VERSION 3.10)
project(eipc)

enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_subdirectory(src)
add_subdirectory(tests)