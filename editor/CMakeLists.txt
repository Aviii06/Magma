cmake_minimum_required(VERSION 3.5)

project(magma_editor VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

set(SOURCES
        src/Editor.cpp
        src/gui/GuiContext.cpp
        src/gui/GuiRenderer.cpp
        src/gui/panes/ViewportPane.cpp
        src/gui/panes/PropertiesPane.cpp
        src/gui/panes/SceneHierarchyPane.cpp
        src/gui/panes/MenuBarPane.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
        magma_engine
        vendor)
