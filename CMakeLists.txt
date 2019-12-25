cmake_minimum_required(VERSION 3.0.2)

project(thread_pool_on_mutexes
    LANGUAGES CXX
    VERSION 0.5)

add_library(thread_pool_on_mutexes
    src/thread_pool_on_mutexes.cpp)

target_include_directories(thread_pool_on_mutexes
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
