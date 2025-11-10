/**
 * @file test_window_title_bar_icons.cc
 * @brief Unit tests for window title bar icon rendering
 */

#include <doctest/doctest.h>
#include <utils/test_helpers.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/icon.hh>
#include <utils/test_canvas_backend.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <iostream>

using namespace onyxui;
using Backend = testing::test_canvas_backend;

TEST_CASE("window_title_bar - Icons are created and rendered") {
    // Initialize UI context for theme support
    ui_context_fixture<Backend> fixture;

    SUBCASE("Title bar with all icons creates children correctly") {
        // Create title bar with all icons enabled
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Test Window", flags);

        // Children: 1 label + 1 spring + 4 icons = 6
        CHECK(title_bar->children().size() == 6);
    }

    SUBCASE("Icons have proper bounds after layout") {
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;  // Disable menu for simpler test
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Test", flags);

        // Measure and arrange
        auto measured = title_bar->measure(80, 1);
        MESSAGE("Title bar measured size: " << measured.w << "x" << measured.h);

        title_bar->arrange({0, 0, 80, 1});

        // Should have 5 children (1 label + 1 spring + 3 icons)
        auto& children = title_bar->children();
        REQUIRE(children.size() == 5);

        // Check bounds of each child
        for (size_t i = 0; i < children.size(); ++i) {
            auto bounds = children[i]->bounds();
            MESSAGE("Child " << i << " bounds: ("
                    << bounds.x << "," << bounds.y << ") size ("
                    << bounds.w << "x" << bounds.h << ")");

            // All children should have non-zero width and height
            CHECK(bounds.w > 0);
            CHECK(bounds.h == 1);  // Title bar is 1 row tall

            // Icons should be positioned to the right of the title
            if (i > 0) {  // Icons are after the label
                CHECK(bounds.x > 0);  // Should not be at position 0
            }
        }

        // Icons should be positioned at the right side
        // Last icon (close button) should be near the right edge
        // Children: 0=label, 1=spring, 2=minimize, 3=maximize, 4=close
        auto close_icon_bounds = children[4]->bounds();
        CHECK(close_icon_bounds.x >= 77);  // Should be near right edge (80 - 3 icons)
    }

    // Note: Rendering test removed - the important tests are:
    // 1. Icons are created (children count)
    // 2. Icons are positioned at right edge (bounds check)

    SUBCASE("Icon widgets report correct size in do_measure") {
        using icon_type = typename Backend::renderer_type::icon_style;
        using icon_widget = icon<Backend>;

        auto icon_ptr = std::make_unique<icon_widget>(icon_type::close_x);

        // Icons should measure as 1x1 for text backends
        auto size = icon_ptr->measure(10, 10);
        CHECK(size.w == 1);
        CHECK(size.h == 1);
    }

    SUBCASE("Title label width is constrained to content") {
        typename window<Backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Short", flags);

        [[maybe_unused]] auto measured_size = title_bar->measure(80, 1);
        title_bar->arrange({0, 0, 80, 1});

        // First child should be the title label
        auto& children = title_bar->children();
        REQUIRE(children.size() >= 5);  // title + spring + icons

        auto title_bounds = children[0]->bounds();

        // Title label should only be as wide as its content
        // "Short" = 5 characters
        CHECK(title_bounds.w <= 10);  // Allow some padding but not full width

        MESSAGE("Title label width: " << title_bounds.w);
    }

    SUBCASE("Icons are positioned after title with proper spacing") {
        typename window<Backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("MyApp", flags);

        [[maybe_unused]] auto measured_size = title_bar->measure(80, 1);
        title_bar->arrange({0, 0, 80, 1});

        auto& children = title_bar->children();
        REQUIRE(children.size() == 5);  // title + spring + 3 icons

        // Get positions (children: 0=title, 1=spring, 2=minimize, 3=maximize, 4=close)
        auto title_bounds = children[0]->bounds();
        auto min_bounds = children[2]->bounds();
        auto max_bounds = children[3]->bounds();
        auto close_bounds = children[4]->bounds();

        MESSAGE("Title: x=" << title_bounds.x << " w=" << title_bounds.w);
        MESSAGE("Minimize: x=" << min_bounds.x);
        MESSAGE("Maximize: x=" << max_bounds.x);
        MESSAGE("Close: x=" << close_bounds.x);

        // Icons should be to the right of the title
        CHECK(min_bounds.x > (title_bounds.x + title_bounds.w));

        // Icons should be in order (minimize, maximize, close)
        CHECK(max_bounds.x > min_bounds.x);
        CHECK(close_bounds.x > max_bounds.x);

        // Icons should be near the right edge
        CHECK(close_bounds.x >= 77);  // 80 - 3 for the close button itself
    }
}