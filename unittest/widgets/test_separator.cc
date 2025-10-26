/**
 * @file test_separator.cc
 * @brief Tests for separator widget
 * @author Claude Code
 * @date 2025-10-26
 *
 * @details
 * Comprehensive tests for horizontal and vertical separator widgets,
 * including measurement, rendering, and theme integration.
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/separator.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"

using namespace onyxui;
using namespace onyxui::testing;
using CanvasBackend = test_canvas_backend;

TEST_SUITE("Separator Widget") {

    TEST_CASE("Separator - Basic construction") {
        SUBCASE("Horizontal separator construction") {
            separator<CanvasBackend> sep(orientation::horizontal);

            CHECK(sep.get_orientation() == orientation::horizontal);
        }

        SUBCASE("Vertical separator construction") {
            separator<CanvasBackend> sep(orientation::vertical);

            CHECK(sep.get_orientation() == orientation::vertical);
        }

        SUBCASE("Default orientation is horizontal") {
            separator<CanvasBackend> sep;

            CHECK(sep.get_orientation() == orientation::horizontal);
        }
    }

    TEST_CASE("Separator - Orientation change") {
        separator<CanvasBackend> sep(orientation::horizontal);

        REQUIRE(sep.get_orientation() == orientation::horizontal);

        sep.set_orientation(orientation::vertical);

        CHECK(sep.get_orientation() == orientation::vertical);
    }

    TEST_CASE("Separator - Measurement (horizontal)") {
        scoped_ui_context<CanvasBackend> ctx;
        separator<CanvasBackend> sep(orientation::horizontal);

        // Horizontal separator should have height=1 and expand width
        auto measured = sep.measure(100, 100);

        // Height should be fixed at 1
        CHECK(size_utils::get_height(measured) == 1);

        // Width with expand policy returns natural content size (0) during measure
        // Actual width is determined during arrange by the layout
        CHECK(size_utils::get_width(measured) == 0);
    }

    TEST_CASE("Separator - Measurement (vertical)") {
        scoped_ui_context<CanvasBackend> ctx;
        separator<CanvasBackend> sep(orientation::vertical);

        // Vertical separator should have width=1 and expand height
        auto measured = sep.measure(100, 100);

        // Width should be fixed at 1
        CHECK(size_utils::get_width(measured) == 1);

        // Height with expand policy returns natural content size (0) during measure
        // Actual height is determined during arrange by the layout
        CHECK(size_utils::get_height(measured) == 0);
    }

    TEST_CASE("Separator - Arrangement") {
        scoped_ui_context<CanvasBackend> ctx;

        SUBCASE("Horizontal separator arrangement") {
            separator<CanvasBackend> sep(orientation::horizontal);

            [[maybe_unused]] auto size = sep.measure(50, 10);
            sep.arrange({0, 5, 50, 1});

            auto bounds = sep.bounds();
            CHECK(rect_utils::get_x(bounds) == 0);
            CHECK(rect_utils::get_y(bounds) == 5);
            CHECK(rect_utils::get_width(bounds) == 50);
            CHECK(rect_utils::get_height(bounds) == 1);
        }

        SUBCASE("Vertical separator arrangement") {
            separator<CanvasBackend> sep(orientation::vertical);

            [[maybe_unused]] auto size = sep.measure(10, 50);
            sep.arrange({5, 0, 1, 50});

            auto bounds = sep.bounds();
            CHECK(rect_utils::get_x(bounds) == 5);
            CHECK(rect_utils::get_y(bounds) == 0);
            CHECK(rect_utils::get_width(bounds) == 1);
            CHECK(rect_utils::get_height(bounds) == 50);
        }
    }

    TEST_CASE("Separator - Visual rendering (horizontal)") {
        scoped_ui_context<CanvasBackend> ctx;

        // Create theme with separator line style
        ui_theme<CanvasBackend> theme;
        theme.name = "Test Theme";
        theme.separator.line_style.horizontal = '-';
        theme.separator.line_style.vertical = '|';
        ctx.themes().register_theme(std::move(theme));

        separator<CanvasBackend> sep(orientation::horizontal);
        sep.apply_theme("Test Theme", ctx.themes());

        // Render to canvas (render_to_canvas does measure/arrange automatically)
        auto canvas = render_to_canvas(sep, 10, 5);

        // Verify horizontal line rendered at y=0 (separator has height=1)
        INFO("Rendered separator:\n", debug_canvas(*canvas));

        for (int x = 0; x < 10; ++x) {
            CHECK_MESSAGE(canvas->get(x, 0).ch == '-',
                "Horizontal line should be rendered at x=", x, ", y=0");
        }
    }

    TEST_CASE("Separator - Visual rendering (vertical)") {
        scoped_ui_context<CanvasBackend> ctx;

        // Create theme with separator line style
        ui_theme<CanvasBackend> theme;
        theme.name = "Test Theme";
        theme.separator.line_style.horizontal = '-';
        theme.separator.line_style.vertical = '|';
        ctx.themes().register_theme(std::move(theme));

        separator<CanvasBackend> sep(orientation::vertical);
        sep.apply_theme("Test Theme", ctx.themes());

        // Render to canvas (render_to_canvas does measure/arrange automatically)
        auto canvas = render_to_canvas(sep, 5, 10);

        // Verify vertical line rendered at x=0 (separator has width=1)
        INFO("Rendered separator:\n", debug_canvas(*canvas));

        for (int y = 0; y < 10; ++y) {
            CHECK_MESSAGE(canvas->get(0, y).ch == '|',
                "Vertical line should be rendered at x=0, y=", y);
        }
    }

    TEST_CASE("Separator - In menu context (horizontal divider)") {
        scoped_ui_context<CanvasBackend> ctx;

        // Create theme
        ui_theme<CanvasBackend> theme;
        theme.name = "Menu Theme";
        theme.separator.line_style.horizontal = '-';
        ctx.themes().register_theme(std::move(theme));

        // Simulate menu with items and separator
        vbox<CanvasBackend> menu;
        menu.set_spacing(0);

        auto item1 = std::make_unique<label<CanvasBackend>>("New");
        auto item2 = std::make_unique<label<CanvasBackend>>("Open");
        auto sep = std::make_unique<separator<CanvasBackend>>(orientation::horizontal);
        auto item3 = std::make_unique<label<CanvasBackend>>("Exit");

        menu.add_child(std::move(item1));
        menu.add_child(std::move(item2));
        menu.add_child(std::move(sep));
        menu.add_child(std::move(item3));

        menu.apply_theme("Menu Theme", ctx.themes());

        // Measure and arrange
        [[maybe_unused]] auto menu_size = menu.measure(20, 50);
        menu.arrange({0, 0, 20, 50});

        // Verify separator has height=1
        auto& children = menu.children();
        REQUIRE(children.size() == 4);

        auto* separator_widget = dynamic_cast<separator<CanvasBackend>*>(children[2].get());
        REQUIRE(separator_widget != nullptr);

        auto sep_bounds = separator_widget->bounds();
        CHECK(rect_utils::get_height(sep_bounds) == 1);
        CHECK(rect_utils::get_width(sep_bounds) == 20);
    }

    TEST_CASE("Separator - In toolbar context (vertical divider)") {
        scoped_ui_context<CanvasBackend> ctx;

        // Create theme
        ui_theme<CanvasBackend> theme;
        theme.name = "Toolbar Theme";
        theme.separator.line_style.vertical = '|';
        ctx.themes().register_theme(std::move(theme));

        // Simulate toolbar with buttons and separator
        hbox<CanvasBackend> toolbar;
        toolbar.set_spacing(0);

        auto btn1 = std::make_unique<button<CanvasBackend>>("Cut");
        auto btn2 = std::make_unique<button<CanvasBackend>>("Copy");
        auto sep = std::make_unique<separator<CanvasBackend>>(orientation::vertical);
        auto btn3 = std::make_unique<button<CanvasBackend>>("Paste");

        toolbar.add_child(std::move(btn1));
        toolbar.add_child(std::move(btn2));
        toolbar.add_child(std::move(sep));
        toolbar.add_child(std::move(btn3));

        toolbar.apply_theme("Toolbar Theme", ctx.themes());

        // Measure and arrange
        [[maybe_unused]] auto toolbar_size = toolbar.measure(50, 3);
        toolbar.arrange({0, 0, 50, 3});

        // Verify separator has width=1
        auto& children = toolbar.children();
        REQUIRE(children.size() == 4);

        auto* separator_widget = dynamic_cast<separator<CanvasBackend>*>(children[2].get());
        REQUIRE(separator_widget != nullptr);

        auto sep_bounds = separator_widget->bounds();
        CHECK(rect_utils::get_width(sep_bounds) == 1);
        CHECK(rect_utils::get_height(sep_bounds) == 3);
    }

    TEST_CASE("Separator - Theme application") {
        scoped_ui_context<CanvasBackend> ctx;

        // Create two themes with different line styles
        ui_theme<CanvasBackend> theme1;
        theme1.name = "Theme 1";
        theme1.separator.line_style.horizontal = '-';

        ui_theme<CanvasBackend> theme2;
        theme2.name = "Theme 2";
        theme2.separator.line_style.horizontal = '=';

        ctx.themes().register_theme(std::move(theme1));
        ctx.themes().register_theme(std::move(theme2));

        separator<CanvasBackend> sep(orientation::horizontal);

        // Apply first theme
        sep.apply_theme("Theme 1", ctx.themes());
        [[maybe_unused]] auto size = sep.measure(10, 1);
        sep.arrange({0, 0, 10, 1});
        auto canvas1 = render_to_canvas(sep, 10, 1);

        // Apply second theme
        sep.apply_theme("Theme 2", ctx.themes());
        [[maybe_unused]] auto size2 = sep.measure(10, 1);
        sep.arrange({0, 0, 10, 1});
        auto canvas2 = render_to_canvas(sep, 10, 1);

        // Verify different line characters were used
        CHECK(canvas1->get(0, 0).ch == '-');
        CHECK(canvas2->get(0, 0).ch == '=');
    }

    TEST_CASE("Separator - Size constraints") {
        scoped_ui_context<CanvasBackend> ctx;

        SUBCASE("Horizontal separator size constraints") {
            separator<CanvasBackend> sep(orientation::horizontal);

            // Should have fixed height=1, expand width
            auto measured = sep.measure(100, 50);

            CHECK(size_utils::get_height(measured) == 1);
            // Width with expand policy reports natural size (0) during measure
            CHECK(size_utils::get_width(measured) == 0);
        }

        SUBCASE("Vertical separator size constraints") {
            separator<CanvasBackend> sep(orientation::vertical);

            // Should have fixed width=1, expand height
            auto measured = sep.measure(50, 100);

            CHECK(size_utils::get_width(measured) == 1);
            // Height with expand policy reports natural size (0) during measure
            CHECK(size_utils::get_height(measured) == 0);
        }
    }

    TEST_CASE("Separator - Orientation change invalidates layout") {
        scoped_ui_context<CanvasBackend> ctx;

        separator<CanvasBackend> sep(orientation::horizontal);

        // Measure as horizontal
        auto size1 = sep.measure(100, 100);
        CHECK(size_utils::get_height(size1) == 1);

        // Change to vertical
        sep.set_orientation(orientation::vertical);

        // Re-measure should give different result
        auto size2 = sep.measure(100, 100);
        CHECK(size_utils::get_width(size2) == 1);

        // Heights should be different
        CHECK(size_utils::get_height(size1) != size_utils::get_height(size2));
    }
}
