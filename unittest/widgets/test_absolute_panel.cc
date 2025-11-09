//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/absolute_panel.hh>
#include <onyxui/widgets/label.hh>
#include <utility>
#include <string>
#include <vector>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/rect_like.hh"
using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "AbsolutePanel - Absolute positioning layout widget") {
    SUBCASE("Construction") {
        absolute_panel<test_canvas_backend> const panel;

        CHECK_FALSE(panel.is_focusable());
    }

    SUBCASE("Add children with default positioning") {
        absolute_panel<test_canvas_backend> panel;

        // Children without explicit positions default to (0, 0)
        auto btn1 = std::make_unique<button<test_canvas_backend>>("Button 1");
        auto btn2 = std::make_unique<button<test_canvas_backend>>("Button 2");

        panel.add_child(std::move(btn1));
        panel.add_child(std::move(btn2));

        CHECK(panel.children().size() == 2);

        // Should not crash
        CHECK_NOTHROW((void)panel.measure(400, 400));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 400);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Explicit positioning with auto-size") {
        absolute_panel<test_canvas_backend> panel;

        // Position buttons at specific coordinates
        auto btn1 = std::make_unique<button<test_canvas_backend>>("Top-Left");
        auto* btn1_ptr = btn1.get();
        panel.add_child(std::move(btn1));
        panel.set_position(btn1_ptr, 10, 10);

        auto btn2 = std::make_unique<button<test_canvas_backend>>("Bottom-Right");
        auto* btn2_ptr = btn2.get();
        panel.add_child(std::move(btn2));
        panel.set_position(btn2_ptr, 200, 150);

        CHECK(panel.children().size() == 2);
        CHECK_NOTHROW((void)panel.measure(300, 200));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Explicit positioning with size override") {
        absolute_panel<test_canvas_backend> panel;

        // Position button with fixed size
        auto btn = std::make_unique<button<test_canvas_backend>>("Fixed Size");
        auto* btn_ptr = btn.get();
        panel.add_child(std::move(btn));
        panel.set_position(btn_ptr, 50, 50, 120, 40);

        CHECK_NOTHROW((void)panel.measure(300, 300));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 300);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Custom dialog layout") {
        absolute_panel<test_canvas_backend> dialog;

        // Title at top
        auto title = std::make_unique<label<test_canvas_backend>>("Confirm Action");
        auto* title_ptr = title.get();
        dialog.add_child(std::move(title));
        dialog.set_position(title_ptr, 20, 10, 260, 30);

        // Message in middle
        auto message = std::make_unique<label<test_canvas_backend>>("Are you sure?");
        auto* message_ptr = message.get();
        dialog.add_child(std::move(message));
        dialog.set_position(message_ptr, 20, 50, 260, 80);

        // OK button
        auto ok = std::make_unique<button<test_canvas_backend>>("OK");
        auto* ok_ptr = ok.get();
        dialog.add_child(std::move(ok));
        dialog.set_position(ok_ptr, 50, 150, 80, 30);

        // Cancel button
        auto cancel = std::make_unique<button<test_canvas_backend>>("Cancel");
        auto* cancel_ptr = cancel.get();
        dialog.add_child(std::move(cancel));
        dialog.set_position(cancel_ptr, 170, 150, 80, 30);

        CHECK(dialog.children().size() == 4);
        CHECK_NOTHROW((void)dialog.measure(300, 200));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        CHECK_NOTHROW(dialog.arrange(bounds));
    }

    SUBCASE("Tooltip positioning") {
        absolute_panel<test_canvas_backend> overlay;

        // Position tooltip at specific location
        int const mouse_x = 150;
        int const mouse_y = 200;

        auto tooltip = std::make_unique<label<test_canvas_backend>>("Click to continue");
        auto* tooltip_ptr = tooltip.get();
        overlay.add_child(std::move(tooltip));
        overlay.set_position(tooltip_ptr, mouse_x + 10, mouse_y - 30);

        CHECK_NOTHROW((void)overlay.measure(400, 400));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 400);
        CHECK_NOTHROW(overlay.arrange(bounds));
    }

    SUBCASE("Negative coordinates") {
        absolute_panel<test_canvas_backend> panel;

        // Position element with negative coordinates (outside panel)
        auto lbl = std::make_unique<label<test_canvas_backend>>("Outside");
        auto* lbl_ptr = lbl.get();
        panel.add_child(std::move(lbl));
        panel.set_position(lbl_ptr, -10, -10);

        CHECK_NOTHROW((void)panel.measure(200, 200));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 200);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Overlapping children") {
        absolute_panel<test_canvas_backend> canvas;

        // Multiple elements at same position (overlapping)
        auto bg = std::make_unique<panel<test_canvas_backend>>();
        auto* bg_ptr = bg.get();
        canvas.add_child(std::move(bg));
        canvas.set_position(bg_ptr, 0, 0, 200, 200);

        auto fg = std::make_unique<label<test_canvas_backend>>("Overlay");
        auto* fg_ptr = fg.get();
        canvas.add_child(std::move(fg));
        canvas.set_position(fg_ptr, 50, 50);

        CHECK(canvas.children().size() == 2);
        CHECK_NOTHROW((void)canvas.measure(300, 300));
    }

    SUBCASE("Node editor layout") {
        absolute_panel<test_canvas_backend> node_canvas;

        // Position nodes at specific coordinates
        struct Node {
            int x, y;
            std::string name;
        };

        std::vector<Node> const nodes = {
            {50, 50, "Input"},
            {200, 100, "Process"},
            {350, 150, "Output"}
        };

        for (auto& node : nodes) {
            auto widget = std::make_unique<button<test_canvas_backend>>(node.name);
            auto* widget_ptr = widget.get();
            node_canvas.add_child(std::move(widget));
            node_canvas.set_position(widget_ptr, node.x, node.y, 80, 40);
        }

        CHECK(node_canvas.children().size() == 3);
        CHECK_NOTHROW((void)node_canvas.measure(500, 300));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 500, 300);
        CHECK_NOTHROW(node_canvas.arrange(bounds));
    }

    SUBCASE("Mixed auto and fixed sizing") {
        absolute_panel<test_canvas_backend> panel;

        // Element with auto-size
        auto auto_sized = std::make_unique<label<test_canvas_backend>>("Auto");
        auto* auto_ptr = auto_sized.get();
        panel.add_child(std::move(auto_sized));
        panel.set_position(auto_ptr, 10, 10);  // No size override

        // Element with fixed size
        auto fixed_sized = std::make_unique<label<test_canvas_backend>>("Fixed");
        auto* fixed_ptr = fixed_sized.get();
        panel.add_child(std::move(fixed_sized));
        panel.set_position(fixed_ptr, 10, 50, 100, 30);  // Width and height override

        CHECK(panel.children().size() == 2);
        CHECK_NOTHROW((void)panel.measure(300, 200));
    }

    SUBCASE("Zero size hiding") {
        absolute_panel<test_canvas_backend> panel;

        // Element with zero width (effectively hidden)
        auto hidden = std::make_unique<label<test_canvas_backend>>("Hidden");
        auto* hidden_ptr = hidden.get();
        panel.add_child(std::move(hidden));
        panel.set_position(hidden_ptr, 50, 50, 0, 0);

        // Visible element
        auto visible = std::make_unique<label<test_canvas_backend>>("Visible");
        auto* visible_ptr = visible.get();
        panel.add_child(std::move(visible));
        panel.set_position(visible_ptr, 100, 100);

        CHECK(panel.children().size() == 2);
        CHECK_NOTHROW((void)panel.measure(300, 200));
    }

    // ===========================================================================
    // Visual Rendering Tests (MANDATORY - see docs/CLAUDE/TESTING.md)
    // ===========================================================================

    SUBCASE("Visual rendering - layout strategy verification") {
        absolute_panel<test_canvas_backend> panel;

        // Add children at different positions
        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Top Left");
        auto* lbl1_ptr = lbl1.get();
        panel.add_child(std::move(lbl1));
        panel.set_position(lbl1_ptr, 10, 10);

        auto btn1 = std::make_unique<button<test_canvas_backend>>("Center");
        auto* btn1_ptr = btn1.get();
        panel.add_child(std::move(btn1));
        panel.set_position(btn1_ptr, 40, 12);

        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Bottom Right");
        auto* lbl2_ptr = lbl2.get();
        panel.add_child(std::move(lbl2));
        panel.set_position(lbl2_ptr, 60, 20);

        // Measure
        auto size = panel.measure(100, 30);
        INFO("Measured size: " << size_utils::get_width(size) << " x " << size_utils::get_height(size));
        CHECK(size_utils::get_width(size) > 0);   // CRITICAL: Must not measure to zero
        CHECK(size_utils::get_height(size) > 0);

        // Arrange
        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 100, 30);
        panel.arrange(bounds);

        // Render
        auto canvas = render_to_canvas(panel, 100, 30);
        std::string rendered = canvas->render_ascii();

        // Verify non-empty output
        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        INFO("Rendered " << content_chars << " non-whitespace characters");
        CHECK(content_chars > 0);  // Must render something

        // Verify expected content appears
        CHECK(rendered.find("Top Left") != std::string::npos);
        CHECK(rendered.find("Center") != std::string::npos);
        CHECK(rendered.find("Bottom Right") != std::string::npos);
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<absolute_panel<test_canvas_backend>>(
        [](auto& panel) {
            panel.add_child(std::make_unique<button<test_canvas_backend>>("Test"));
        },
        [](const auto& panel) { return panel.children().size() == 1; }
    );

    SUBCASE("Rule of Five - Dangling pointer fix verification") {
        // This test specifically verifies that the dangling pointer bug is fixed
        absolute_panel<test_canvas_backend> panel1;

        auto btn = std::make_unique<button<test_canvas_backend>>("Button");
        auto* btn_ptr = btn.get();
        panel1.add_child(std::move(btn));
        panel1.set_position(btn_ptr, 100, 100, 80, 40);

        // Move construct - this should NOT leave dangling pointer
        absolute_panel<test_canvas_backend> panel2(std::move(panel1));

        // panel2's layout pointer should be valid, not dangling
        CHECK_NOTHROW((void)panel2.measure(300, 200));

        // Arranging should work without crashes
        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        CHECK_NOTHROW(panel2.arrange(bounds));
    }
}