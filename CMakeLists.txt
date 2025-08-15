cmake_minimum_required(VERSION 3.16)
project(L2CrashSender)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(L2CrashSender main.cpp)

target_link_libraries(L2CrashSender PRIVATE Wininet.lib)

