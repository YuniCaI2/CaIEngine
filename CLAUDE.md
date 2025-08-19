# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses CMake with Ninja generator. The project is structured with a main executable linking to a FrameWork library.

### Build Commands
- **Debug Build**: 
  ```bash
  cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
  ninja -C cmake-build-debug
  ```
- **Release Build**: 
  ```bash
  cmake -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release  
  ninja -C cmake-build-release
  ```
- **Run Application**: `./cmake-build-debug/CaIEngine.exe` (Debug) or `./cmake-build-release/CaIEngine.exe` (Release)

### Shader Compilation
- Shaders are automatically compiled from GLSL to SPIR-V using the framework's built-in shader compilation system
- GLSL shaders are located in `resources/shaders/glsl/` with corresponding `.spv` files generated automatically
- The system detects shader file modifications and recompiles as needed using `shaderTimeCache.bin`

## Architecture Overview

CaIEngine is a Vulkan-based 3D rendering engine with the following key architectural components:

### Core Framework (`FrameWork/` directory)
- **vulkanFrameWork**: Main singleton class managing the entire Vulkan rendering pipeline, resource pools, and render state
- **VulkanDevice**: Wrapper for Vulkan logical device and queue management  
- **VulkanSwapChain**: Manages presentation and swapchain operations
- **Resource Management**: Centralized pools for textures, meshes, attachments, framebuffers, pipelines, and materials
- **Slot System**: Dynamic descriptor set management system for uniform binding

### Key Design Patterns
- **Singleton Pattern**: Core systems (vulkanFrameWork, InputManager, Resource) use singleton access via `GetInstance()`
- **Resource Pooling**: All Vulkan resources are managed through templated pools with automatic ID assignment and reuse
- **RAII Wrapper**: Vulkan objects wrapped in classes with automatic cleanup
- **Template-based Resource Management**: Generic `getByIndex<T>()`, `getNextIndex<T>()`, and `destroyByIndex<T>()` methods

### Resource Management System
Resources are managed through vector pools with automatic indexing:
- Textures, Meshes, Attachments, Framebuffers stored in separate vectors
- Resources marked as `inUse` when active, recycled when freed
- Template specialization handles different resource types uniformly

### Rendering Pipeline
1. **Preparation Phase**: Creates descriptors, pipelines, and framebuffers
2. **Frame Loop**: Updates uniforms, records command buffers, submits rendering commands
3. **Present Phase**: Handles swapchain presentation with optional MSAA resolve

### Dependencies
- **Vulkan 1.3+**: Core graphics API
- **GLFW**: Window management and input
- **GLM**: Mathematics library for 3D transformations  
- **ImGui**: Immediate mode GUI
- **Assimp**: 3D model loading
- **STB Image**: Texture loading

### Entry Point
The main application is implemented in `Renderer.cpp` which:
- Initializes the Vulkan framework
- Sets up rendering pipelines and resources
- Implements the main render loop with input handling
- Provides GUI controls for runtime settings

### Shader System
- Shaders written in GLSL and compiled to SPIR-V
- Automatic shader recompilation on file changes
- Support for vertex, fragment, and compute shaders
- Pipeline state configuration through framework API

### Camera and Input
- Free-flying camera with WASD movement and mouse look
- Input system handles keyboard/mouse events through GLFW
- ESC key exits the application

This architecture provides a modern Vulkan rendering framework with automatic resource management, flexible pipeline configuration, and efficient rendering performance.