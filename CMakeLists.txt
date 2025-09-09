# Shoutout to CMake guide: https://eliasdaler.github.io/using-cmake/
cmake_minimum_required(VERSION 3.16)

# Get project name from folder name
get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId} VERSION 1.0.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable additional warnings and debug mode
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find SFML
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib ${CMAKE_PREFIX_PATH})
set(SFML_DIR ${CMAKE_SOURCE_DIR}/lib/cmake/SFML)
set(SFML_STATIC_LIBRARIES OFF)
find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics Audio)

# Fetch ImGui and ImGui-SFML
include(FetchContent)

# Fetch ImGui first
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)
FetchContent_MakeAvailable(imgui)

# Configure ImGui-SFML to use dynamic SFML and set ImGui directory
set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

# Fetch ImGui-SFML
FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui-sfml)

# All source files
file(GLOB_RECURSE SOURCES 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link SFML and ImGui-SFML libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SFML::System 
    SFML::Window 
    SFML::Graphics 
    SFML::Audio
)

# Platform-specific linking
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE GL)
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

# Copy resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})

# Set output name
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "mandelbrotset")