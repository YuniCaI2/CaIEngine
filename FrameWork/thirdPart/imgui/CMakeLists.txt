find_package(Vulkan REQUIRED)
find_package(assimp REQUIRED)

set(MacOSImGuiPath "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(imgui STATIC
        ${MacOSImGuiPath}/imgui.cpp
        ${MacOSImGuiPath}/imgui_draw.cpp
        ${MacOSImGuiPath}/imgui_tables.cpp
        ${MacOSImGuiPath}/imgui_widgets.cpp
        ${MacOSImGuiPath}/backends/imgui_impl_glfw.cpp
        ${MacOSImGuiPath}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
        ${MacOSImGuiPath}
        ${MacOSImGuiPath}/backends
)

target_link_libraries(imgui PRIVATE
        glfw
        Vulkan::Vulkan
)