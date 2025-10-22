include(FetchContent)

# Allow FetchContent_Populate to be used (required for our PATCH_COMMAND pattern)
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

# Fetch fkYAML and replace its CMakeLists.txt with our minimal version
# This prevents Catch2 submodule and test targets from being built
fetchcontent_declare(
    fkYAML
    GIT_REPOSITORY "https://github.com/fktn-k/fkYAML.git"
    GIT_TAG v0.3.12
    GIT_PROGRESS TRUE
    GIT_SUBMODULES ""       # Don't clone Catch2 submodule
    GIT_SHALLOW TRUE        # Shallow clone for faster download
    UPDATE_COMMAND ""
    # Replace CMakeLists.txt with our minimal version after download
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${CMAKE_CURRENT_SOURCE_DIR}/fkyaml-minimal.cmake
                  <SOURCE_DIR>/CMakeLists.txt
)

# Populate (download) but don't add subdirectory yet
FetchContent_GetProperties(fkYAML)
if(NOT fkyaml_POPULATED)
    FetchContent_Populate(fkYAML)

    # Export source directory to parent scope for installation
    set(FKYAML_SOURCE_DIR ${fkyaml_SOURCE_DIR} PARENT_SCOPE)

    # Add subdirectory with our patched CMakeLists.txt
    add_subdirectory(${fkyaml_SOURCE_DIR} ${fkyaml_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

message(STATUS "fkYAML configured (patched to exclude tests/Catch2)")
