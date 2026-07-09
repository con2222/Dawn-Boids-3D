cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME webgpu-boids-3d)
set(APP_NAME app)

include(FetchContent)

option(DEV_MODE "Set up development helper settings" ON)

project(${PROJECT_NAME}
        VERSION 0.1
        LANGUAGES C CXX
)

option(USE_WAYLAND "Build with Wayland support" ON)
option(USE_X11 "Build with X11 support" ON)

if (WIN32)
    message(STATUS "Configuring for Windows (D3D12/Vulkan)")
elseif (APPLE)
    message(STATUS "Configuring for macOS (Metal)")
elseif (EMSCRIPTEN)
    message(STATUS "Configuring for Web (WebGPU/WebGL)")
elseif (UNIX)
    message(STATUS "Configuring for Linux (X11 + Wayland)")
    set(DAWN_USE_WAYLAND ${USE_WAYLAND} CACHE BOOL "" FORCE)
    set(DAWN_USE_X11 ${USE_X11} CACHE BOOL "" FORCE)

    if (USE_WAYLAND)
        add_definitions(-D_GLFW_WAYLAND)
    endif()
    if (USE_X11)
        add_definitions(-D_GLFW_X11)
    endif()
endif()

set(GLFW_BUILD_WAYLAND ${USE_WAYLAND} CACHE BOOL "Enable Wayland for GLFW" FORCE)
set(GLFW_BUILD_X11 ${USE_X11} CACHE BOOL "Enable X11 for GLFW" FORCE)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_subdirectory(vendor/glfw3webgpu)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/webgpu)

set(SOURCES 
    src/main.cpp
    src/App.cpp
    src/Renderer.cpp
    src/Mesh.cpp
    src/ResourceManager.cpp
    src/WebGPUContext.cpp
    src/WindowContext.cpp
    src/Camera.cpp
)

add_executable(${APP_NAME} ${SOURCES})

if (DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(${APP_NAME} PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"   
        SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    )
else ()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(${APP_NAME} PRIVATE
        RESOURCE_DIR="./assets"
        SHADER_DIR="./shaders"
    )
endif()

target_include_directories(${APP_NAME} PRIVATE 
    include/
    vendor/
)

target_link_libraries(${APP_NAME} PRIVATE
    glfw
    webgpu
    glfw3webgpu
    glm
    imgui
)

set_target_properties(${APP_NAME} PROPERTIES
    COMPILE_WARNING_AS_ERROR ON
)

include(${CMAKE_CURRENT_SOURCE_DIR}/utils.cmake)
target_copy_webgpu_binaries(${APP_NAME})
