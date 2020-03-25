cmake_minimum_required(VERSION 3.15)
project(source-store LANGUAGES CXX ASM)

add_subdirectory(src)

target_link_libraries(source-store dwarf)