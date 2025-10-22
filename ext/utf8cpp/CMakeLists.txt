include(FetchContent)

# Allow FetchContent_Populate to be used (required for our PATCH_COMMAND pattern)
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

# Fetch utf8cpp and replace its CMakeLists.txt with our minimal version
# This prevents ftest submodule and test targets from being built
fetchcontent_declare(
    UTF8CPP
    GIT_REPOSITORY "https://github.com/nemtrif/utfcpp.git"
    GIT_TAG v4.0.5
    GIT_PROGRESS TRUE
    GIT_SUBMODULES ""       # Don't clone ftest submodule
    GIT_SHALLOW TRUE        # Shallow clone for faster download
    UPDATE_COMMAND ""
    # Replace CMakeLists.txt with our minimal version after download
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${CMAKE_CURRENT_SOURCE_DIR}/utf8cpp-minimal.cmake
                  <SOURCE_DIR>/CMakeLists.txt
)

# Populate (download) but don't add subdirectory yet
FetchContent_GetProperties(UTF8CPP)
if(NOT utf8cpp_POPULATED)
    FetchContent_Populate(UTF8CPP)

    # Export source directory to parent scope for installation
    set(UTF8CPP_SOURCE_DIR ${utf8cpp_SOURCE_DIR} PARENT_SCOPE)

    # Add subdirectory with our patched CMakeLists.txt
    add_subdirectory(${utf8cpp_SOURCE_DIR} ${utf8cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

message(STATUS "utf8cpp configured (patched to exclude tests/ftest)")
