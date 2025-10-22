cmake_minimum_required(VERSION 3.15)
project(utf8cpp
        VERSION 4.0.5
        LANGUAGES CXX
        DESCRIPTION "UTF-8 with C++ in a Portable Way")

# Create interface library (header-only)
add_library(${PROJECT_NAME} INTERFACE)

# Set include directories
target_include_directories(utf8cpp INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>"
    "$<INSTALL_INTERFACE:include/utf8cpp>"
)

# Skip all tests, samples, and installation
# This minimal CMakeLists.txt only creates the library target
set(CMAKE_SKIP_INSTALL_RULES ON)
