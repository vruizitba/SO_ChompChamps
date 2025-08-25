# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(TP C)

set(CMAKE_C_STANDARD 99)

# Encuentra todos los archivos .c en el directorio src
file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c")

# Añade los archivos encontrados al ejecutable
add_executable(TP
        main.c
        ${SRC_FILES}
)

target_include_directories(TP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(TP PRIVATE Threads::Threads)