cmake_minimum_required(VERSION 3.5)

project(magma_editor VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} src/Editor.cpp)

target_link_libraries(${PROJECT_NAME}
        magma_engine
        vendor)
