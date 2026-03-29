# Patches cmake_minimum_required to version 3.10...3.31 for CMake 4.x compatibility.

file(READ CMakeLists.txt _content)

string(REGEX REPLACE
    "cmake_minimum_required\\(VERSION [0-9]+\\.[0-9]+(\\.[0-9]+)?(\\.\\.\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?)?\\)"
    "cmake_minimum_required(VERSION 3.10...3.31)"
    _content "${_content}")
file(WRITE CMakeLists.txt "${_content}")
