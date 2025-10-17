add_subdirectory(glfw)
add_subdirectory(glm)

find_package(Vulkan REQUIRED)

#add_subdirectory(imgui)
add_subdirectory(vk-bootstrap)
#add_subdirectory(vma)
#add_subdirectory(tinyobjloader)

add_library(vendor INTERFACE)

target_link_libraries(vendor
        INTERFACE
        glfw
        glm
        Vulkan::Headers
        Vulkan::Vulkan
#        imgui
        vk-bootstrap::vk-bootstrap
#        vma
#        tinyobjloader
)