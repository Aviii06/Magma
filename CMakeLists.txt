cmake_minimum_required(VERSION 3.28)
project(magma)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor)
add_subdirectory(engine)
add_subdirectory(editor)
