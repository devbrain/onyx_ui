# AddBackend.cmake
# Helper function for creating backend libraries with proper export/visibility handling
#
# Usage:
#   add_backend_library(
#       NAME <backend_name>              # e.g., "conio", "sdl2"
#       SOURCES <source_files>           # Implementation files
#       PUBLIC_HEADERS <header_files>    # Public API headers
#       DEPENDENCIES <targets>           # Public link dependencies
#       [PRIVATE_SOURCES <files>]        # Optional private sources
#       [PRIVATE_DEPENDENCIES <targets>] # Optional private link dependencies
#   )
#
# This function:
# - Creates shared or static library based on BUILD_SHARED_LIBS
# - Generates export header with symbol visibility macros
# - Sets proper symbol visibility (hidden by default)
# - Configures include directories (source + generated)
# - Applies warning flags
# - Creates properly namespaced target

include(GenerateExportHeader)

function(add_backend_library)
    set(options "")
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES PUBLIC_HEADERS PRIVATE_SOURCES DEPENDENCIES PRIVATE_DEPENDENCIES)
    cmake_parse_arguments(BACKEND "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT BACKEND_NAME)
        message(FATAL_ERROR "add_backend_library: NAME is required")
    endif()
    if(NOT BACKEND_SOURCES AND NOT BACKEND_PUBLIC_HEADERS)
        message(FATAL_ERROR "add_backend_library: SOURCES or PUBLIC_HEADERS required")
    endif()

    # Construct target name
    set(TARGET_NAME onyxui_${BACKEND_NAME}_backend)
    set(EXPORT_MACRO_PREFIX ONYXUI_${BACKEND_NAME})
    string(TOUPPER ${EXPORT_MACRO_PREFIX} EXPORT_MACRO_PREFIX)

    # Create library (type determined by BUILD_SHARED_LIBS)
    add_library(${TARGET_NAME}
        ${BACKEND_SOURCES}
        ${BACKEND_PUBLIC_HEADERS}
        ${BACKEND_PRIVATE_SOURCES}
    )

    # Symbol visibility: hidden by default (Windows-style)
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        POSITION_INDEPENDENT_CODE ON  # Required for shared libs
    )

    # Generate export header in binary directory
    set(EXPORT_HEADER_DIR "${CMAKE_BINARY_DIR}/include/onyxui/backends/${BACKEND_NAME}")
    set(EXPORT_HEADER_FILE "${EXPORT_HEADER_DIR}/onyxui_${BACKEND_NAME}_export.h")

    generate_export_header(${TARGET_NAME}
        BASE_NAME ${EXPORT_MACRO_PREFIX}
        EXPORT_FILE_NAME ${EXPORT_HEADER_FILE}
        EXPORT_MACRO_NAME ${EXPORT_MACRO_PREFIX}_EXPORT
        NO_EXPORT_MACRO_NAME ${EXPORT_MACRO_PREFIX}_NO_EXPORT
        DEPRECATED_MACRO_NAME ${EXPORT_MACRO_PREFIX}_DEPRECATED
        NO_DEPRECATED_MACRO_NAME ${EXPORT_MACRO_PREFIX}_NO_DEPRECATED
    )

    # Include directories
    target_include_directories(${TARGET_NAME}
        PUBLIC
            # Source headers
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            # Generated export header
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            # Install location
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    # Link public dependencies
    if(BACKEND_DEPENDENCIES)
        target_link_libraries(${TARGET_NAME} PUBLIC ${BACKEND_DEPENDENCIES})
    endif()

    # Link private dependencies
    if(BACKEND_PRIVATE_DEPENDENCIES)
        target_link_libraries(${TARGET_NAME} PRIVATE ${BACKEND_PRIVATE_DEPENDENCIES})
    endif()

    # Always link to core onyxui
    target_link_libraries(${TARGET_NAME} PUBLIC onyxui)

    # Apply warning flags from neutrino-cmake if available
    if(TARGET neutrino::warnings)
        target_link_libraries(${TARGET_NAME} PRIVATE neutrino::warnings)
    endif()

    # Set output name (remove redundant "onyxui_" prefix if present)
    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "onyxui_${BACKEND_NAME}"
    )

    # Display build info
    get_target_property(LIB_TYPE ${TARGET_NAME} TYPE)
    message(STATUS "Backend '${BACKEND_NAME}': ${LIB_TYPE}")
    message(STATUS "  Export header: ${EXPORT_HEADER_FILE}")
    message(STATUS "  Export macro: ${EXPORT_MACRO_PREFIX}_EXPORT")

    # Export target for parent scope (so demo apps can link)
    set(${TARGET_NAME}_CREATED TRUE PARENT_SCOPE)
endfunction()
