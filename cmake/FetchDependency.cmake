# FetchDependency.cmake
# Helper function to fetch or find external dependencies with consistent pattern
#
# Usage:
#   fetch_dependency(
#       NAME <name>                    # Package name (e.g., "utf8cpp")
#       TARGET <target>                # Primary target name (e.g., "utf8cpp")
#       ALIAS <alias>                  # Standardized alias (e.g., "thirdparty::utf8cpp")
#       [ALTERNATIVE_TARGETS <list>]   # Alternative target names to check
#       [FETCH_METHOD <method>]        # "FETCHCONTENT" or "SUBDIR"
#       [FETCH_SOURCE <source>]        # FetchContent args or subdirectory path
#   )

function(fetch_dependency)
    set(options "")
    set(oneValueArgs NAME TARGET ALIAS FETCH_METHOD FETCH_SOURCE)
    set(multiValueArgs ALTERNATIVE_TARGETS)
    cmake_parse_arguments(DEP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT DEP_NAME OR NOT DEP_TARGET OR NOT DEP_ALIAS)
        message(FATAL_ERROR "fetch_dependency requires NAME, TARGET, and ALIAS")
    endif()

    # Build list of all targets to check
    set(ALL_TARGETS ${DEP_TARGET} ${DEP_ALIAS} ${DEP_ALTERNATIVE_TARGETS})

    # Step 1: Check if any target already exists
    set(FOUND_EXISTING FALSE)
    foreach(target_name IN LISTS ALL_TARGETS)
        if(TARGET ${target_name})
            message(STATUS "Found ${DEP_NAME} target: ${target_name}")
            set(FOUND_EXISTING TRUE)
            set(EXISTING_TARGET ${target_name})
            break()
        endif()
    endforeach()

    # Step 2: If found, create alias if needed
    if(FOUND_EXISTING)
        if(NOT TARGET ${DEP_ALIAS} AND NOT "${EXISTING_TARGET}" STREQUAL "${DEP_ALIAS}")
            add_library(${DEP_ALIAS} ALIAS ${EXISTING_TARGET})
            message(STATUS "Created alias ${DEP_ALIAS} -> ${EXISTING_TARGET}")
        endif()
        return()
    endif()

    # Step 3: Try find_package
    find_package(${DEP_NAME} QUIET)

    if(${DEP_NAME}_FOUND)
        message(STATUS "Found ${DEP_NAME} via find_package")

        # Create alias to the found target
        if(TARGET ${DEP_NAME}::${DEP_NAME})
            set(FOUND_TARGET ${DEP_NAME}::${DEP_NAME})
        elseif(TARGET ${DEP_TARGET})
            set(FOUND_TARGET ${DEP_TARGET})
        else()
            message(WARNING "${DEP_NAME} found but target unknown")
            return()
        endif()

        if(NOT TARGET ${DEP_ALIAS})
            add_library(${DEP_ALIAS} ALIAS ${FOUND_TARGET})
        endif()
        return()
    endif()

    # Step 4: Fetch from source
    message(STATUS "${DEP_NAME} not found - fetching from source...")

    if(DEP_FETCH_METHOD STREQUAL "SUBDIR")
        # Use add_subdirectory for ext/ pattern
        if(NOT DEP_FETCH_SOURCE)
            message(FATAL_ERROR "FETCH_SOURCE required for SUBDIR method")
        endif()

        add_subdirectory(${DEP_FETCH_SOURCE})

        # Verify target was created
        if(NOT TARGET ${DEP_TARGET})
            message(FATAL_ERROR "${DEP_TARGET} not created by ${DEP_FETCH_SOURCE}")
        endif()

        # Create alias
        if(NOT TARGET ${DEP_ALIAS})
            add_library(${DEP_ALIAS} ALIAS ${DEP_TARGET})
        endif()

    elseif(DEP_FETCH_METHOD STREQUAL "FETCHCONTENT")
        message(FATAL_ERROR "FETCHCONTENT method not yet implemented - use ext/ pattern")
    else()
        message(FATAL_ERROR "Unknown FETCH_METHOD: ${DEP_FETCH_METHOD}")
    endif()

    message(STATUS "${DEP_NAME} configured successfully")
endfunction()
