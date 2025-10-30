//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/anchor_panel.hh>
#include <onyxui/widgets/label.hh>
#include <utility>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/rect_like.hh"
#include "onyxui/layout/anchor_layout.hh"
using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "AnchorPanel - Anchor-based layout widget") {
    SUBCASE("Construction") {
        anchor_panel<test_canvas_backend> const panel;

        CHECK_FALSE(panel.is_focusable());
    }

    SUBCASE("Add children with default positioning") {
        anchor_panel<test_canvas_backend> panel;

        // Children without explicit anchors default to top-left
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

    SUBCASE("Anchor at corners") {
        anchor_panel<test_canvas_backend> panel;

        // Top-left
        auto tl = std::make_unique<label<test_canvas_backend>>("TL");
        auto* tl_ptr = tl.get();
        panel.add_child(std::move(tl));
        panel.set_anchor(tl_ptr, anchor_point::top_left);

        // Top-right
        auto tr = std::make_unique<label<test_canvas_backend>>("TR");
        auto* tr_ptr = tr.get();
        panel.add_child(std::move(tr));
        panel.set_anchor(tr_ptr, anchor_point::top_right);

        // Bottom-left
        auto bl = std::make_unique<label<test_canvas_backend>>("BL");
        auto* bl_ptr = bl.get();
        panel.add_child(std::move(bl));
        panel.set_anchor(bl_ptr, anchor_point::bottom_left);

        // Bottom-right
        auto br = std::make_unique<label<test_canvas_backend>>("BR");
        auto* br_ptr = br.get();
        panel.add_child(std::move(br));
        panel.set_anchor(br_ptr, anchor_point::bottom_right);

        CHECK(panel.children().size() == 4);
        CHECK_NOTHROW((void)panel.measure(300, 300));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 300);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Anchor at edges") {
        anchor_panel<test_canvas_backend> panel;

        // Top-center
        auto tc = std::make_unique<label<test_canvas_backend>>("TC");
        auto* tc_ptr = tc.get();
        panel.add_child(std::move(tc));
        panel.set_anchor(tc_ptr, anchor_point::top_center);

        // Center-left
        auto cl = std::make_unique<label<test_canvas_backend>>("CL");
        auto* cl_ptr = cl.get();
        panel.add_child(std::move(cl));
        panel.set_anchor(cl_ptr, anchor_point::center_left);

        // Center-right
        auto cr = std::make_unique<label<test_canvas_backend>>("CR");
        auto* cr_ptr = cr.get();
        panel.add_child(std::move(cr));
        panel.set_anchor(cr_ptr, anchor_point::center_right);

        // Bottom-center
        auto bc = std::make_unique<label<test_canvas_backend>>("BC");
        auto* bc_ptr = bc.get();
        panel.add_child(std::move(bc));
        panel.set_anchor(bc_ptr, anchor_point::bottom_center);

        CHECK(panel.children().size() == 4);
        CHECK_NOTHROW((void)panel.measure(300, 300));
    }

    SUBCASE("Center anchor") {
        anchor_panel<test_canvas_backend> panel;

        // Centered element
        auto centered = std::make_unique<label<test_canvas_backend>>("Centered");
        auto* centered_ptr = centered.get();
        panel.add_child(std::move(centered));
        panel.set_anchor(centered_ptr, anchor_point::center);

        CHECK_NOTHROW((void)panel.measure(400, 400));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 400);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Anchor with positive offsets") {
        anchor_panel<test_canvas_backend> panel;

        // Button at top-left with 10px margins
        auto btn = std::make_unique<button<test_canvas_backend>>("Button");
        auto* btn_ptr = btn.get();
        panel.add_child(std::move(btn));
        panel.set_anchor(btn_ptr, anchor_point::top_left, 10, 10);

        CHECK_NOTHROW((void)panel.measure(300, 300));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 300);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Anchor with negative offsets") {
        anchor_panel<test_canvas_backend> panel;

        // Button at bottom-right with margins (negative offsets)
        auto btn = std::make_unique<button<test_canvas_backend>>("FAB");
        auto* btn_ptr = btn.get();
        panel.add_child(std::move(btn));
        panel.set_anchor(btn_ptr, anchor_point::bottom_right, -16, -16);

        CHECK_NOTHROW((void)panel.measure(300, 300));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 300);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Title bar layout") {
        anchor_panel<test_canvas_backend> panel;

        // Menu button at left
        auto menu = std::make_unique<button<test_canvas_backend>>("Menu");
        auto* menu_ptr = menu.get();
        panel.add_child(std::move(menu));
        panel.set_anchor(menu_ptr, anchor_point::center_left, 10, 0);

        // Title in center
        auto title = std::make_unique<label<test_canvas_backend>>("My App");
        auto* title_ptr = title.get();
        panel.add_child(std::move(title));
        panel.set_anchor(title_ptr, anchor_point::center);

        // Close button at right
        auto close = std::make_unique<button<test_canvas_backend>>("X");
        auto* close_ptr = close.get();
        panel.add_child(std::move(close));
        panel.set_anchor(close_ptr, anchor_point::center_right, -10, 0);

        CHECK(panel.children().size() == 3);
        CHECK_NOTHROW((void)panel.measure(400, 50));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 50);
        CHECK_NOTHROW(panel.arrange(bounds));
    }

    SUBCASE("Game HUD layout") {
        anchor_panel<test_canvas_backend> hud;

        // Health bar at top-left
        auto health = std::make_unique<label<test_canvas_backend>>("HP: 100");
        auto* health_ptr = health.get();
        hud.add_child(std::move(health));
        hud.set_anchor(health_ptr, anchor_point::top_left, 20, 20);

        // Score at top-right
        auto score = std::make_unique<label<test_canvas_backend>>("Score: 0");
        auto* score_ptr = score.get();
        hud.add_child(std::move(score));
        hud.set_anchor(score_ptr, anchor_point::top_right, -20, 20);

        // Mini-map at bottom-right
        auto minimap = std::make_unique<panel<test_canvas_backend>>();
        auto* minimap_ptr = minimap.get();
        hud.add_child(std::move(minimap));
        hud.set_anchor(minimap_ptr, anchor_point::bottom_right, -150, -150);

        // Inventory at bottom-center
        auto inventory = std::make_unique<panel<test_canvas_backend>>();
        auto* inventory_ptr = inventory.get();
        hud.add_child(std::move(inventory));
        hud.set_anchor(inventory_ptr, anchor_point::bottom_center, 0, -100);

        CHECK(hud.children().size() == 4);
        CHECK_NOTHROW((void)hud.measure(800, 600));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 800, 600);
        CHECK_NOTHROW(hud.arrange(bounds));
    }

    SUBCASE("Overlapping children") {
        anchor_panel<test_canvas_backend> overlay;

        // Background panel fills entire area
        auto background = std::make_unique<panel<test_canvas_backend>>();
        auto* bg_ptr = background.get();
        overlay.add_child(std::move(background));
        overlay.set_anchor(bg_ptr, anchor_point::top_left);

        // Centered dialog on top
        auto dialog = std::make_unique<panel<test_canvas_backend>>();
        auto* dialog_ptr = dialog.get();
        overlay.add_child(std::move(dialog));
        overlay.set_anchor(dialog_ptr, anchor_point::center);

        CHECK(overlay.children().size() == 2);
        CHECK_NOTHROW((void)overlay.measure(500, 500));
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<anchor_panel<test_canvas_backend>>(
        [](auto& panel) {
            panel.add_child(std::make_unique<button<test_canvas_backend>>("Test"));
        },
        [](const auto& panel) { return panel.children().size() == 1; }
    );

    SUBCASE("Rule of Five - Dangling pointer fix verification") {
        // This test specifically verifies that the dangling pointer bug is fixed
        anchor_panel<test_canvas_backend> panel1;

        auto btn = std::make_unique<button<test_canvas_backend>>("Button");
        auto* btn_ptr = btn.get();
        panel1.add_child(std::move(btn));
        panel1.set_anchor(btn_ptr, anchor_point::center);

        // Move construct - this should NOT leave dangling pointer
        anchor_panel<test_canvas_backend> panel2(std::move(panel1));

        // panel2's layout pointer should be valid, not dangling
        CHECK_NOTHROW((void)panel2.measure(100, 100));

        // Arranging should work without crashes
        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 100, 100);
        CHECK_NOTHROW(panel2.arrange(bounds));
    }
}