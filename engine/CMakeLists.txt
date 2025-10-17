cmake_minimum_required(VERSION 3.5)
project(magma_engine VERSION 0.0.1 LANGUAGES CXX)

set(SOURCES
        src/Engine.cpp
        src/Window.cpp

        src/core/renderer/Renderer.cpp
        src/core/renderer/VkInitializers.cpp
        src/core/renderer/VkUtils.cpp
        src/core/renderer/Pipeline.cpp
        src/core/renderer/ShaderModule.cpp
        src/core/renderer/DescriptorManager.cpp
)

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
        vendor
)

file (GLOB SHADERS assets/shaders/*.frag assets/shaders/*.vert)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/editor/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/editor/assets/shaders)


foreach(shader ${SHADERS})
    get_filename_component(SHADER_NAME ${shader} NAME)

    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND glslc -c ${shader} -o ${CMAKE_BINARY_DIR}/editor/assets/shaders/${SHADER_NAME}.spv)
endforeach()
