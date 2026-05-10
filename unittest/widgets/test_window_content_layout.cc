/**
 * @file test_window_content_layout.cc
 * @brief Tests for window content layout bug
 * @author OnyxUI Team
 * @date 2025-11-17
 *
 * @details
 * Tests to verify that window content is properly visible and laid out
 * when set_content() is called before set_size().
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>
#include <onyxui/widgets/menu/separator.hh>
#include <onyxui/services/layer_manager.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

TEST_SUITE("window - content layout") {

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Content is visible after set_content then set_size") {
        // This test reproduces the bug where content isn't visible until maximize

        // Create window
        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = false;
        flags.is_movable = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Create content with multiple children (like in the demo)
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);

        auto* label1 = content->template emplace_child<label>("First Label");
        auto* label2 = content->template emplace_child<label>("Second Label");
        auto* button1 = content->template emplace_child<button>("Button 1");

        // Set content FIRST (like in the working demo)
        win->set_content(std::move(content));

        // Set size AFTER content (like the fixed demo)
        win->set_size(45, 18);
        win->set_position(8, 4);

        // Now check that content is properly measured and arranged
        auto* content_widget = win->get_content();
        REQUIRE(content_widget != nullptr);

        // Content should have non-zero bounds
        auto content_bounds = content_widget->bounds();
        INFO("Content bounds: (" << content_bounds.x.to_int() << ", " << content_bounds.y.to_int()
             << ", " << content_bounds.width.to_int() << ", " << content_bounds.height.to_int() << ")");

        CHECK(content_bounds.width.to_int() > 0);
        CHECK(content_bounds.height.to_int() > 0);

        // Children should have non-zero bounds
        auto label1_bounds = label1->bounds();
        INFO("Label1 bounds: (" << label1_bounds.x.to_int() << ", " << label1_bounds.y.to_int()
             << ", " << label1_bounds.width.to_int() << ", " << label1_bounds.height.to_int() << ")");

        CHECK(label1_bounds.width.to_int() > 0);
        CHECK(label1_bounds.height.to_int() > 0);

        auto label2_bounds = label2->bounds();
        CHECK(label2_bounds.width.to_int() > 0);
        CHECK(label2_bounds.height.to_int() > 0);

        auto button1_bounds = button1->bounds();
        CHECK(button1_bounds.width.to_int() > 0);
        CHECK(button1_bounds.height.to_int() > 0);

        // Children should be positioned sequentially (vbox layout)
        // label2.y should be greater than label1.y (below it)
        CHECK(label2_bounds.y.to_int() > label1_bounds.y.to_int());

        // button1.y should be greater than label2.y (below it)
        CHECK(button1_bounds.y.to_int() > label2_bounds.y.to_int());
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Content WITHOUT stretch alignment should still be visible") {
        // This test specifically checks that content is visible even without
        // manually setting stretch alignment (the hack we added)

        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Create content WITHOUT setting stretch alignment
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);
        // DO NOT call: content->set_horizontal_align(horizontal_alignment::stretch);
        // DO NOT call: content->set_vertical_align(vertical_alignment::stretch);

        content->template emplace_child<label>("Test Label");
        auto* btn = content->template emplace_child<button>("Test Button");

        win->set_content(std::move(content));
        win->set_size(40, 15);

        // Button should have non-zero bounds
        auto btn_bounds = btn->bounds();
        INFO("Button bounds: (" << btn_bounds.x.to_int() << ", " << btn_bounds.y.to_int()
             << ", " << btn_bounds.width.to_int() << ", " << btn_bounds.height.to_int() << ")");

        CHECK(btn_bounds.width.to_int() > 0);
        CHECK(btn_bounds.height.to_int() > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Reverse order: set_size BEFORE set_content (the bug case)") {
        // This test demonstrates the ORIGINAL BUG where set_size() is called
        // BEFORE set_content(), causing content to not be visible

        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // BUG: Set size BEFORE content (like the broken demo did)
        win->set_size(45, 18);
        win->set_position(8, 4);

        // Create content AFTER sizing
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);
        auto* label1 = content->template emplace_child<label>("First Label");
        auto* button1 = content->template emplace_child<button>("Button 1");

        win->set_content(std::move(content));

        // Even though we set content AFTER sizing, it should still work!
        // The set_content() should trigger re-measure/re-arrange

        auto* content_widget = win->get_content();
        REQUIRE(content_widget != nullptr);

        auto content_bounds = content_widget->bounds();
        INFO("Content bounds: (" << content_bounds.x.to_int() << ", " << content_bounds.y.to_int()
             << ", " << content_bounds.width.to_int() << ", " << content_bounds.height.to_int() << ")");

        CHECK(content_bounds.width.to_int() > 0);
        CHECK(content_bounds.height.to_int() > 0);

        auto label1_bounds = label1->bounds();
        CHECK(label1_bounds.width.to_int() > 0);
        CHECK(label1_bounds.height.to_int() > 0);

        auto button1_bounds = button1->bounds();
        CHECK(button1_bounds.width.to_int() > 0);
        CHECK(button1_bounds.height.to_int() > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Footer stays bottom-aligned after hovering button inside scroll body") {
        typename window<Backend>::window_flags flags;
        flags.is_resizable = false;
        flags.is_movable = true;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Scenario Information", flags);

        size_constraint fixed_w;
        fixed_w.policy = size_policy::fixed;
        fixed_w.preferred_size = 640_lu;
        fixed_w.min_size = 380_lu;
        fixed_w.max_size = 700_lu;
        win->set_width_constraint(fixed_w);

        size_constraint fixed_h;
        fixed_h.policy = size_policy::fixed;
        fixed_h.preferred_size = 280_lu;
        fixed_h.min_size = 240_lu;
        fixed_h.max_size = 520_lu;
        win->set_height_constraint(fixed_h);

        auto root = std::make_unique<vbox<Backend>>(spacing::none);

        auto body_scroll = std::make_unique<scroll_view<Backend>>();
        body_scroll->set_height_constraint({size_policy::expand});
        body_scroll->set_horizontal_scroll_enabled(false);

        auto body_content = std::make_unique<vbox<Backend>>(spacing::none);
        body_content->add_child(std::make_unique<spacer<Backend>>(0, 12));
        body_content->template emplace_child<label>("Scenario title");
        body_content->template emplace_child<label>("Author: Test");
        body_content->template emplace_child<label>("Version: Test");
        body_content->template emplace_child<label>("Short scenario description");

        auto toggle_row = std::make_unique<hbox<Backend>>(spacing::none);
        auto toggle = std::make_unique<button<Backend>>("Show Advanced");
        auto* toggle_ptr = toggle.get();
        toggle_row->add_child(std::move(toggle));
        toggle_row->add_child(std::make_unique<spring<Backend>>());
        body_content->add_child(std::move(toggle_row));

        body_scroll->content_add_child(std::move(body_content));
        root->add_child(std::move(body_scroll));
        root->add_child(std::make_unique<spacer<Backend>>(0, 0));

        auto sep = std::make_unique<separator<Backend>>();
        auto* sep_ptr = sep.get();
        root->add_child(std::move(sep));
        root->add_child(std::make_unique<spacer<Backend>>(0, 6));

        auto footer = std::make_unique<hbox<Backend>>(spacing::medium);
        footer->add_child(std::make_unique<spring<Backend>>());
        auto ok = std::make_unique<button<Backend>>("OK");
        auto* ok_ptr = ok.get();
        footer->add_child(std::move(ok));
        root->add_child(std::move(footer));
        root->set_padding({0_lu, 0_lu, 0_lu, 6_lu});

        win->set_content(std::move(root));

        layer_manager<Backend> layers;
        win->show_modal(layers);

        auto canvas = std::make_shared<test_canvas>(800, 600);
        typename Backend::renderer_type renderer(canvas);
        typename Backend::rect_type viewport{0, 0, 800, 600};

        layers.render_all_layers(
            renderer,
            viewport,
            ctx.themes().get_current_theme(),
            ctx.metrics());

        auto const separator_y_before = sep_ptr->get_absolute_logical_bounds().y;
        auto const ok_y_before = ok_ptr->get_absolute_logical_bounds().y;

        auto const toggle_bounds = toggle_ptr->get_absolute_logical_bounds();
        CHECK(separator_y_before > toggle_bounds.y + 50_lu);

        mouse_event hover{
            .x = toggle_bounds.x + 1_lu,
            .y = toggle_bounds.y + 1_lu,
            .btn = mouse_event::button::none,
            .act = mouse_event::action::move,
            .modifiers = {}
        };
        CHECK(layers.route_event(ui_event{hover}) == true);

        layers.render_all_layers(
            renderer,
            viewport,
            ctx.themes().get_current_theme(),
            ctx.metrics());

        CHECK(sep_ptr->get_absolute_logical_bounds().y == separator_y_before);
        CHECK(ok_ptr->get_absolute_logical_bounds().y == ok_y_before);
    }
}
