project(imgui)


file(GLOB imguiPath
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
add_library(imgui ${imguiPath})

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})