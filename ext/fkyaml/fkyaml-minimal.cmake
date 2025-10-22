cmake_minimum_required(VERSION 3.15)
project(fkYAML
        VERSION 0.3.12
        LANGUAGES CXX
        DESCRIPTION "A C++ header-only YAML library")

# Create interface library (header-only)
add_library(fkYAML INTERFACE)
add_library(fkYAML::fkYAML ALIAS fkYAML)

# Set include directories
target_include_directories(fkYAML INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

# Require C++11 minimum
target_compile_features(fkYAML INTERFACE cxx_std_11)

# Skip all tests, tools, and installation
# This minimal CMakeLists.txt only creates the library target
set(CMAKE_SKIP_INSTALL_RULES ON)
