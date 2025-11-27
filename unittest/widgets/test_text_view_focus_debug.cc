/**
 * @file test_text_view_focus_debug.cc
 * @brief Debug test for text_view focus - simpler version
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/events/event_router.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "DEBUG - text_view focus path") {
    // Create root panel with layout strategy
    auto root_panel = std::make_unique<panel<test_canvas_backend>>();
    auto* root = root_panel.get();

    // Set layout strategy for root panel to arrange children
    root->set_layout_strategy(
        std::make_unique<linear_layout<test_canvas_backend>>(
            direction::vertical, 0,
            horizontal_alignment::stretch,
            vertical_alignment::stretch));

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    // Create text_view
    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view_ptr = text_view_widget.get();
    text_view_widget->set_text("Line 1\nLine 2");

    // Add to root
    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto size = root->measure(80_lu, 25_lu);
    root->arrange(logical_rect{0_lu, 0_lu, 80_lu, 25_lu});

    // Hit test
    hit_test_path<test_canvas_backend> path;
    auto* target = root->hit_test(5, 5, path);
    (void)target;  // Unused in non-debug mode

    // Create mouse event
    mouse_event click{
        .x = 5,
        .y = 5,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = click;

    // Route event
    [[maybe_unused]] bool handled = route_event(ui_evt, path);

    // The test itself
    CHECK(path.contains(text_view_ptr));
    CHECK(input->get_focused() == text_view_ptr);
}
