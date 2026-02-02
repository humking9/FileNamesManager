cmake_minimum_required(VERSION 3.14)

project(FileNamesManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Action: Static Linking for Windows (MSVC)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/MT$<$<CONFIG:Debug>:d>)
endif()

# Dependencies using FetchContent

include(FetchContent)

# 1. GLFW
message(STATUS "Fetching GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
FetchContent_MakeAvailable(glfw)

# 2. Dear ImGui
message(STATUS "Fetching Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        v1.91.8
)
FetchContent_MakeAvailable(imgui)

# 3. Portable File Dialogs (Header Only)
message(STATUS "Fetching Portable File Dialogs...")
FetchContent_Declare(
    pfd
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs
    GIT_TAG        main
)
FetchContent_GetProperties(pfd)
if(NOT pfd_POPULATED)
    FetchContent_Populate(pfd)
endif()
# FetchContent_MakeAvailable(pfd) # Header only, no need to build or add_subdirectory if it causes issues

# Source Files
set(SOURCES
    src/main.cpp
    src/FileScanner.cpp
    src/FileScanner.h
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Compile Definitions
# add_compile_definitions() 

# Create Executable
add_executable(FileNamesManager ${SOURCES})

# Include Directories
target_include_directories(FileNamesManager PRIVATE
    src
    ${glfw_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${pfd_SOURCE_DIR}
)

# Link Libraries
target_link_libraries(FileNamesManager PRIVATE glfw)

# Platform Specific Linking
if(WIN32)
    # Windows libs usually handled by GLFW, but explicitly:
    target_link_libraries(FileNamesManager PRIVATE opengl32)
elseif(APPLE)
    target_link_libraries(FileNamesManager PRIVATE "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
else()
    # Limux
    target_link_libraries(FileNamesManager PRIVATE GL dl pthread)
endif()

# Copy assets if needed (none for now)
