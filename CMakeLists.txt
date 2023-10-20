cmake_minimum_required(VERSION 3.12)
project(1c-selection)

set(CMAKE_CXX_STANDARD 20)

add_executable(main main.cpp)

add_custom_target(test1 ALL main ${CMAKE_CURRENT_LIST_DIR}/folder1 ${CMAKE_CURRENT_LIST_DIR}/folder2 DEPENDS main)
add_custom_target(test2 ALL main ${CMAKE_CURRENT_LIST_DIR}/folder1 ${CMAKE_CURRENT_LIST_DIR}/folder2 60 DEPENDS main)
