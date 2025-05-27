
project(imgui)

find_package(Vulkan REQUIRED)


file(GLOB imguiPath
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/*.h"
)
add_library(imgui ${imguiPath})

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
