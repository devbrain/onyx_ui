find_package(doctest QUIET)

if (NOT doctest_FOUND)
    message(STATUS "doctest not found, fetching from GitHub...")

    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    # Allow FetchContent_Populate to be used
    if (POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif ()

    set(DOCTEST_WITH_TESTS OFF CACHE INTERNAL "")
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE INTERNAL "")
    set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "")

    FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/onqtam/doctest.git
            GIT_TAG v2.4.12
            GIT_PROGRESS TRUE
    )

    # Fetch the content first
    FetchContent_GetProperties(doctest)
    if(NOT doctest_POPULATED)
        FetchContent_Populate(doctest)

        # Patch the CMakeLists.txt to fix deprecation warning
        file(READ ${doctest_SOURCE_DIR}/CMakeLists.txt DOCTEST_CMAKE_CONTENT)
        # Replace any version less than 3.10 with 3.10
        string(REGEX REPLACE "cmake_minimum_required\\(VERSION [0-9]+\\.[0-9]+\\)"
                "cmake_minimum_required(VERSION 3.10)"
                DOCTEST_CMAKE_CONTENT "${DOCTEST_CMAKE_CONTENT}")
        file(WRITE ${doctest_SOURCE_DIR}/CMakeLists.txt "${DOCTEST_CMAKE_CONTENT}")

        # Now add the subdirectory
        add_subdirectory(${doctest_SOURCE_DIR} ${doctest_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif ()

# Export doctest for use in parent project
if (doctest_FOUND OR TARGET doctest::doctest)
    message(STATUS "doctest available for testing")
endif()

add_executable(ui_unittest
        core/test_element.cc
        core/test_signal_slot.cc
        core/test_rule_of_five.cc
        events/test_events.cc
        events/test_event_target.cc
        focus/test_focus_manager.cc
        layout/test_absolute.cc
        layout/test_anchor.cc
        layout/test_composition.cc
        layout/test_grid.cc
        layout/test_linear.cc
        main.cc
        utils/test_backend.hh
        utils/test_helpers.hh
        widgets/test_widgets.cc
        widgets/widgets.hh
        widgets/test_hbox.cc
        widgets/test_vbox.cc
        widgets/test_grid.cc
        widgets/test_anchor_panel.cc
        widgets/test_absolute_panel.cc
        widgets/test_button.cc
        widgets/test_label.cc
        widgets/test_panel.cc
        widgets/test_spacer.cc
        utils/warnings.hh
        widgets/test_spring.cc
        widgets/test_layout_correctness.cc
        widgets/test_factory_functions.cc
        widgets/test_actions.cc
        widgets/test_mnemonics.cc
        widgets/test_menus.cc
        widgets/test_status_bar.cc
        widgets/test_group_box.cc
        hotkeys/test_hotkeys.cc
        hotkeys/test_hotkey_manager.cc
)
target_compile_options(ui_unittest PRIVATE ${ONYX_UI_WARNING_FLAGS})
target_include_directories(ui_unittest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui_unittest
    onyx_ui
    doctest::doctest
    thirdparty::utf8cpp  # Required by test_backend.hh for measure_text()
)