# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(TP C)

set(CMAKE_C_STANDARD 99)

add_executable(TP
        main.c
        src/sync.c
)

target_include_directories(TP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(TP PRIVATE Threads::Threads)