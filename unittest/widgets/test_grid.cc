//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <../../include/onyxui/widgets/containers/grid.hh>
#include <onyxui/widgets/label.hh>
#include <utility>
#include <vector>
#include <string>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/rule_of_five_tests.hh"
#include "onyxui/concepts/size_like.hh"
#include "onyxui/concepts/rect_like.hh"
using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Grid - Grid layout widget") {
    SUBCASE("Construction") {
        grid<test_canvas_backend> const g(3);  // 3 columns

        CHECK_FALSE(g.is_focusable());
        CHECK(g.num_columns() == 3);
        CHECK(g.num_rows() == -1);  // Auto-calculate
    }

    SUBCASE("Add children with auto-assignment") {
        grid<test_canvas_backend> g(2);  // 2 columns

        auto btn1 = std::make_unique<button<test_canvas_backend>>("1");
        auto btn2 = std::make_unique<button<test_canvas_backend>>("2");
        auto btn3 = std::make_unique<button<test_canvas_backend>>("3");
        auto btn4 = std::make_unique<button<test_canvas_backend>>("4");

        g.add_child(std::move(btn1));
        g.add_child(std::move(btn2));
        g.add_child(std::move(btn3));
        g.add_child(std::move(btn4));

        CHECK(g.children().size() == 4);

        // Should not crash
        CHECK_NOTHROW((void)g.measure(200, 200));
    }

    SUBCASE("Explicit cell assignment") {
        grid<test_canvas_backend> g(3, 3);  // 3x3 grid

        auto header = std::make_unique<label<test_canvas_backend>>("Header");
        auto* header_ptr = header.get();
        g.add_child(std::move(header));

        // Assign header to span entire top row
        bool const result = g.set_cell(header_ptr, 0, 0, 1, 3);  // row 0, col 0, span 1x3
        CHECK(result);

        // Measure should not crash
        CHECK_NOTHROW((void)g.measure(300, 300));
    }

    SUBCASE("Fixed-size grid") {
        std::vector<int> const col_widths = {100, 150, 100};
        std::vector<int> const row_heights = {50, 50};

        grid<test_canvas_backend> g(
            3,              // 3 columns
            2,              // 2 rows
            5,              // column spacing
            5,              // row spacing
            false,          // Use fixed sizing
            col_widths,     // Fixed column widths
            row_heights     // Fixed row heights
        );

        // Add some children
        for (int i = 0; i < 6; i++) {
            g.add_child(std::make_unique<label<test_canvas_backend>>(std::to_string(i)));
        }

        auto size = g.measure(500, 500);
        int const width = size_utils::get_width(size);
        int const height = size_utils::get_height(size);

        // Width = 100 + 150 + 100 + 2*5 (spacing) = 360
        CHECK(width == 360);
        // Height = 50 + 50 + 1*5 (spacing) = 105
        CHECK(height == 105);
    }

    SUBCASE("Grid with spacing") {
        grid<test_canvas_backend> g(2, 2, 10, 10);  // 2x2 grid with 10px spacing

        for (int i = 0; i < 4; i++) {
            g.add_child(std::make_unique<button<test_canvas_backend>>(std::to_string(i)));
        }

        CHECK_NOTHROW((void)g.measure(200, 200));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 200);
        CHECK_NOTHROW(g.arrange(bounds));
    }

    SUBCASE("Cell spanning") {
        grid<test_canvas_backend> g(4, 3);  // 4x3 grid

        // Header spans all 4 columns
        auto header = std::make_unique<label<test_canvas_backend>>("Header");
        auto* header_ptr = header.get();
        g.add_child(std::move(header));
        CHECK(g.set_cell(header_ptr, 0, 0, 1, 4));  // row 0, col 0, span 1x4

        // Sidebar spans 2 rows
        auto sidebar = std::make_unique<label<test_canvas_backend>>("Sidebar");
        auto* sidebar_ptr = sidebar.get();
        g.add_child(std::move(sidebar));
        CHECK(g.set_cell(sidebar_ptr, 1, 0, 2, 1));  // row 1, col 0, span 2x1

        // Other items auto-assigned
        for (int i = 0; i < 6; i++) {
            g.add_child(std::make_unique<button<test_canvas_backend>>(std::to_string(i)));
        }

        CHECK(g.children().size() == 8);
        CHECK_NOTHROW((void)g.measure(400, 300));
    }

    SUBCASE("Invalid cell assignment") {
        grid<test_canvas_backend> g(3, 3);  // 3x3 grid

        auto item = std::make_unique<label<test_canvas_backend>>("Item");
        auto* item_ptr = item.get();
        g.add_child(std::move(item));

        // Out of bounds column
        CHECK_FALSE(g.set_cell(item_ptr, 0, 3, 1, 1));

        // Out of bounds row
        CHECK_FALSE(g.set_cell(item_ptr, 3, 0, 1, 1));

        // Span exceeds bounds
        CHECK_FALSE(g.set_cell(item_ptr, 2, 2, 1, 2));  // Would span to column 4
    }

    // ===========================================================================
    // Visual Rendering Tests (MANDATORY - see docs/CLAUDE/TESTING.md)
    // ===========================================================================

    SUBCASE("Visual rendering - layout strategy verification") {
        grid<test_canvas_backend> g(3, 2, 2, 2);  // 3x2 grid with 2px spacing

        // Add children to grid cells
        g.add_child(std::make_unique<label<test_canvas_backend>>("A1"));
        g.add_child(std::make_unique<label<test_canvas_backend>>("A2"));
        g.add_child(std::make_unique<label<test_canvas_backend>>("A3"));
        g.add_child(std::make_unique<button<test_canvas_backend>>("B1"));
        g.add_child(std::make_unique<button<test_canvas_backend>>("B2"));
        g.add_child(std::make_unique<button<test_canvas_backend>>("B3"));

        // Measure
        auto size = g.measure(80, 25);
        INFO("Measured size: " << size_utils::get_width(size) << " x " << size_utils::get_height(size));
        CHECK(size_utils::get_width(size) > 0);   // CRITICAL: Must not measure to zero
        CHECK(size_utils::get_height(size) > 0);

        // Arrange
        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 80, 25);
        g.arrange(bounds);

        // Render
        auto canvas = render_to_canvas(g, 80, 25);
        std::string rendered = canvas->render_ascii();

        // Verify non-empty output
        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        INFO("Rendered " << content_chars << " non-whitespace characters");
        CHECK(content_chars > 0);  // Must render something

        // Verify expected content appears
        CHECK(rendered.find("A1") != std::string::npos);
        CHECK(rendered.find("A2") != std::string::npos);
        CHECK(rendered.find("A3") != std::string::npos);
        CHECK(rendered.find("B1") != std::string::npos);
        CHECK(rendered.find("B2") != std::string::npos);
        CHECK(rendered.find("B3") != std::string::npos);
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<grid<test_canvas_backend>>(
        [](auto& g) {
            g.add_child(std::make_unique<button<test_canvas_backend>>("1"));
            g.add_child(std::make_unique<button<test_canvas_backend>>("2"));
        },
        [](const auto& g) { return g.children().size() == 2; }
    );

    SUBCASE("Rule of Five - Dangling pointer fix verification") {
        // This test specifically verifies that the dangling pointer bug is fixed
        grid<test_canvas_backend> g1(2);

        auto btn1 = std::make_unique<button<test_canvas_backend>>("Button");
        auto* btn1_ptr = btn1.get();
        g1.add_child(std::move(btn1));

        // Set explicit cell
        g1.set_cell(btn1_ptr, 0, 0);

        // Move construct - this should NOT leave dangling pointer
        grid<test_canvas_backend> g2(std::move(g1));

        // g2's layout pointer should be valid, not dangling
        CHECK_NOTHROW((void)g2.measure(100, 100));

        // Arranging should work without crashes
        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 100, 100);
        CHECK_NOTHROW(g2.arrange(bounds));
    }
}