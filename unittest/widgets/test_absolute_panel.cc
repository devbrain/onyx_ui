//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/absolute_panel.hh>
#include <onyxui/widgets/label.hh>
#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"
using namespace onyxui;

TEST_CASE("AbsolutePanel - Absolute positioning layout widget") {
    SUBCASE("Construction") {
        absolute_panel<test_backend> panel;

        CHECK_FALSE(panel.is_focusable());
    }

    SUBCASE("Add children with default positioning") {
        absolute_panel<test_backend> panel;

        // Children without explicit positions default to (0, 0)
        auto btn1 = std::make_unique<button<test_backend>>("Button 1");
        auto btn2 = std::make_unique<button<test_backend>>("Button 2");

        panel.add_child(std::move(btn1));
        panel.add_child(std::move(btn2));

        CHECK(panel.children().size() == 2);

        // Should not crash
        CHECK_NOTHROW((void)panel.measure(400, 400));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 400);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Explicit positioning with auto-size") {
        absolute_panel<test_backend> panel;

        // Position buttons at specific coordinates
        auto btn1 = std::make_unique<button<test_backend>>("Top-Left");
        auto* btn1_ptr = btn1.get();
        panel.add_child(std::move(btn1));
        panel.set_position(btn1_ptr, 10, 10);

        auto btn2 = std::make_unique<button<test_backend>>("Bottom-Right");
        auto* btn2_ptr = btn2.get();
        panel.add_child(std::move(btn2));
        panel.set_position(btn2_ptr, 200, 150);

        CHECK(panel.children().size() == 2);
        CHECK_NOTHROW((void)panel.measure(300, 200));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Explicit positioning with size override") {
        absolute_panel<test_backend> panel;

        // Position button with fixed size
        auto btn = std::make_unique<button<test_backend>>("Fixed Size");
        auto* btn_ptr = btn.get();
        panel.add_child(std::move(btn));
        panel.set_position(btn_ptr, 50, 50, 120, 40);

        CHECK_NOTHROW((void)panel.measure(300, 300));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 300);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Custom dialog layout") {
        absolute_panel<test_backend> dialog;

        // Title at top
        auto title = std::make_unique<label<test_backend>>("Confirm Action");
        auto* title_ptr = title.get();
        dialog.add_child(std::move(title));
        dialog.set_position(title_ptr, 20, 10, 260, 30);

        // Message in middle
        auto message = std::make_unique<label<test_backend>>("Are you sure?");
        auto* message_ptr = message.get();
        dialog.add_child(std::move(message));
        dialog.set_position(message_ptr, 20, 50, 260, 80);

        // OK button
        auto ok = std::make_unique<button<test_backend>>("OK");
        auto* ok_ptr = ok.get();
        dialog.add_child(std::move(ok));
        dialog.set_position(ok_ptr, 50, 150, 80, 30);

        // Cancel button
        auto cancel = std::make_unique<button<test_backend>>("Cancel");
        auto* cancel_ptr = cancel.get();
        dialog.add_child(std::move(cancel));
        dialog.set_position(cancel_ptr, 170, 150, 80, 30);

        CHECK(dialog.children().size() == 4);
        CHECK_NOTHROW((void)dialog.measure(300, 200));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        CHECK_NOTHROW(dialog.arrange(bounds));
    }

    SUBCASE("Tooltip positioning") {
        absolute_panel<test_backend> overlay;

        // Position tooltip at specific location
        int mouse_x = 150;
        int mouse_y = 200;

        auto tooltip = std::make_unique<label<test_backend>>("Click to continue");
        auto* tooltip_ptr = tooltip.get();
        overlay.add_child(std::move(tooltip));
        overlay.set_position(tooltip_ptr, mouse_x + 10, mouse_y - 30);

        CHECK_NOTHROW((void)overlay.measure(400, 400));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 400);
        CHECK_NOTHROW(overlay.arrange(bounds));
    }

    SUBCASE("Negative coordinates") {
        absolute_panel<test_backend> panel;

        // Position element with negative coordinates (outside panel)
        auto lbl = std::make_unique<label<test_backend>>("Outside");
        auto* lbl_ptr = lbl.get();
        panel.add_child(std::move(lbl));
        panel.set_position(lbl_ptr, -10, -10);

        CHECK_NOTHROW((void)panel.measure(200, 200));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 200);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Overlapping children") {
        absolute_panel<test_backend> canvas;

        // Multiple elements at same position (overlapping)
        auto bg = std::make_unique<panel<test_backend>>();
        auto* bg_ptr = bg.get();
        canvas.add_child(std::move(bg));
        canvas.set_position(bg_ptr, 0, 0, 200, 200);

        auto fg = std::make_unique<label<test_backend>>("Overlay");
        auto* fg_ptr = fg.get();
        canvas.add_child(std::move(fg));
        canvas.set_position(fg_ptr, 50, 50);

        CHECK(canvas.children().size() == 2);
        CHECK_NOTHROW((void)canvas.measure(300, 300));
    }

    SUBCASE("Node editor layout") {
        absolute_panel<test_backend> node_canvas;

        // Position nodes at specific coordinates
        struct Node {
            int x, y;
            std::string name;
        };

        std::vector<Node> nodes = {
            {50, 50, "Input"},
            {200, 100, "Process"},
            {350, 150, "Output"}
        };

        for (auto& node : nodes) {
            auto widget = std::make_unique<button<test_backend>>(node.name);
            auto* widget_ptr = widget.get();
            node_canvas.add_child(std::move(widget));
            node_canvas.set_position(widget_ptr, node.x, node.y, 80, 40);
        }

        CHECK(node_canvas.children().size() == 3);
        CHECK_NOTHROW((void)node_canvas.measure(500, 300));

        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 500, 300);
        CHECK_NOTHROW(node_canvas.arrange(bounds));
    }

    SUBCASE("Mixed auto and fixed sizing") {
        absolute_panel<test_backend> panel;

        // Element with auto-size
        auto auto_sized = std::make_unique<label<test_backend>>("Auto");
        auto* auto_ptr = auto_sized.get();
        panel.add_child(std::move(auto_sized));
        panel.set_position(auto_ptr, 10, 10);  // No size override

        // Element with fixed size
        auto fixed_sized = std::make_unique<label<test_backend>>("Fixed");
        auto* fixed_ptr = fixed_sized.get();
        panel.add_child(std::move(fixed_sized));
        panel.set_position(fixed_ptr, 10, 50, 100, 30);  // Width and height override

        CHECK(panel.children().size() == 2);
        CHECK_NOTHROW((void)panel.measure(300, 200));
    }

    SUBCASE("Zero size hiding") {
        absolute_panel<test_backend> panel;

        // Element with zero width (effectively hidden)
        auto hidden = std::make_unique<label<test_backend>>("Hidden");
        auto* hidden_ptr = hidden.get();
        panel.add_child(std::move(hidden));
        panel.set_position(hidden_ptr, 50, 50, 0, 0);

        // Visible element
        auto visible = std::make_unique<label<test_backend>>("Visible");
        auto* visible_ptr = visible.get();
        panel.add_child(std::move(visible));
        panel.set_position(visible_ptr, 100, 100);

        CHECK(panel.children().size() == 2);
        CHECK_NOTHROW((void)panel.measure(300, 200));
    }

    SUBCASE("Rule of Five - Copy operations deleted") {
        static_assert(!std::is_copy_constructible_v<absolute_panel<test_backend>>,
                      "absolute_panel should not be copy constructible");
        static_assert(!std::is_copy_assignable_v<absolute_panel<test_backend>>,
                      "absolute_panel should not be copy assignable");
    }

    SUBCASE("Rule of Five - Move constructor") {
        absolute_panel<test_backend> panel1;

        // Add child with position
        auto btn = std::make_unique<button<test_backend>>("Test");
        auto* btn_ptr = btn.get();
        panel1.add_child(std::move(btn));
        panel1.set_position(btn_ptr, 50, 50);

        // Move construct
        absolute_panel<test_backend> panel2(std::move(panel1));

        CHECK(panel2.children().size() == 1);
        CHECK_NOTHROW(panel1.add_child(std::make_unique<button<test_backend>>("New")));
    }

    SUBCASE("Rule of Five - Move assignment") {
        absolute_panel<test_backend> panel1;
        absolute_panel<test_backend> panel2;

        auto lbl = std::make_unique<label<test_backend>>("Label");
        panel1.add_child(std::move(lbl));

        // Move assign
        panel2 = std::move(panel1);

        CHECK(panel2.children().size() == 1);
        CHECK_NOTHROW(panel1.add_child(std::make_unique<label<test_backend>>("New")));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<absolute_panel<test_backend>> panels;

        panels.emplace_back();
        panels.emplace_back();

        CHECK(panels.size() == 2);
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        absolute_panel<test_backend> panel;
SUPPRESS_SELF_MOVE_BEGIN
        panel = std::move(panel);
SUPPRESS_SELF_MOVE_END
        CHECK_NOTHROW(panel.add_child(std::make_unique<label<test_backend>>("Test")));
    }

    SUBCASE("Rule of Five - Dangling pointer fix verification") {
        // This test specifically verifies that the dangling pointer bug is fixed
        absolute_panel<test_backend> panel1;

        auto btn = std::make_unique<button<test_backend>>("Button");
        auto* btn_ptr = btn.get();
        panel1.add_child(std::move(btn));
        panel1.set_position(btn_ptr, 100, 100, 80, 40);

        // Move construct - this should NOT leave dangling pointer
        absolute_panel<test_backend> panel2(std::move(panel1));

        // panel2's layout pointer should be valid, not dangling
        CHECK_NOTHROW((void)panel2.measure(300, 200));

        // Arranging should work without crashes
        test_backend::rect bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        CHECK_NOTHROW(panel2.arrange(bounds));
    }
}