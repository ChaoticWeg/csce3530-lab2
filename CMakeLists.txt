cmake_minimum_required(VERSION 3.13)
project(CSCE3530_Lab3 C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(src)

add_executable(server server.c)
add_executable(client client.c)

target_link_libraries(server LINK_PUBLIC common)
target_link_libraries(client LINK_PUBLIC common)

