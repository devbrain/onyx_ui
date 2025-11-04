/**
 * @file test_text_view_focus.cc
 * @brief Tests for text_view focus acquisition via capture phase event routing
 * @author OnyxUI Framework
 * @date 2025-11-04
 *
 * @details
 * Tests that text_view correctly uses capture phase to intercept mouse clicks
 * and acquire focus, solving the problem where clicking on child labels would
 * give them focus instead of the text_view.
 *
 * This is the final verification of Phase 4 in the event routing implementation.
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

// ======================================================================
// Helper to create root panel with layout
// ======================================================================
inline auto create_test_root() {
    auto root_panel = std::make_unique<panel<test_canvas_backend>>();
    root_panel->set_layout_strategy(
        std::make_unique<linear_layout<test_canvas_backend>>(
            direction::vertical, 0,
            horizontal_alignment::stretch,
            vertical_alignment::stretch));
    return root_panel;
}

// ======================================================================
// text_view Focus Tests
// ======================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Focus on click (empty)") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    // Create empty text_view
    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view_ptr = text_view_widget.get();

    // Add to root and layout
    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto _ = root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    SUBCASE("Initially unfocused") {
        CHECK(input->get_focused() == nullptr);
    }

    SUBCASE("Click gives focus") {
        // Simulate mouse click at (10, 10)
        mouse_event click{
            .x = 10,
            .y = 10,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        // Route event through hit test path
        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(10, 10, path);

        if (target) {
            ui_event ui_evt = click;
            route_event(ui_evt, path);
        }

        // text_view should now have focus
        CHECK(input->get_focused() == text_view_ptr);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Focus on click (with content)") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    // Create text_view with multiple lines
    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view_ptr = text_view_widget.get();

    text_view_widget->set_text(
        "Line 1\n"
        "Line 2\n"
        "Line 3\n"
        "Line 4\n"
        "Line 5"
    );

    // Add to root and layout
    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto _ = root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    SUBCASE("Click on text gives text_view focus (not label)") {
        // Click at (5, 5) - should hit a label child
        mouse_event click{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        // Route event through hit test path
        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 5, path);

        // Target might be label, but text_view should intercept in capture phase
        REQUIRE(target != nullptr);

        ui_event ui_evt = click;
        route_event(ui_evt, path);

        // text_view should have focus (not the label!)
        CHECK(input->get_focused() == text_view_ptr);
    }

    SUBCASE("Multiple clicks maintain focus") {
        // First click
        mouse_event click1{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path1;
        auto* target1 = root->hit_test(5, 5, path1);
        if (target1) {
            ui_event ui_evt1 = click1;
            route_event(ui_evt1, path1);
        }

        CHECK(input->get_focused() == text_view_ptr);

        // Second click at different location
        mouse_event click2{
            .x = 10,
            .y = 10,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path2;
        auto* target2 = root->hit_test(10, 10, path2);
        if (target2) {
            ui_event ui_evt2 = click2;
            route_event(ui_evt2, path2);
        }

        // Still has focus
        CHECK(input->get_focused() == text_view_ptr);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Focus with multiple text_views") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    // Create two text_views
    auto text_view1_obj = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view1 = text_view1_obj.get();
    text_view1_obj->set_text("View 1\nLine 2");

    auto text_view2_obj = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view2 = text_view2_obj.get();
    text_view2_obj->set_text("View 2\nLine 2");

    // Add to root with vertical layout
    root->add_child(std::move(text_view1_obj));
    root->add_child(std::move(text_view2_obj));

    // Use taller root to accommodate both text_views (each needs ~116 pixels)
    [[maybe_unused]] auto _ = root->measure(80, 250);
    root->arrange({0, 0, 80, 250});

    SUBCASE("Click first text_view gives it focus") {
        mouse_event click{
            .x = 5,
            .y = 2,  // Should hit first text_view
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 2, path);

        if (target) {
            ui_event ui_evt = click;
            route_event(ui_evt, path);
        }

        CHECK(input->get_focused() == text_view1);
    }

    SUBCASE("Click second text_view switches focus") {
        // First click on text_view1
        mouse_event click1{
            .x = 5,
            .y = 2,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path1;
        auto* target1 = root->hit_test(5, 2, path1);
        if (target1) {
            ui_event ui_evt1 = click1;
            route_event(ui_evt1, path1);
        }

        CHECK(input->get_focused() == text_view1);

        // Now click on text_view2 (lower in the layout)
        mouse_event click2{
            .x = 5,
            .y = 150,  // Should hit second text_view (text_view1 is ~116px, text_view2 starts at ~116px)
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path2;
        auto* target2 = root->hit_test(5, 150, path2);
        if (target2) {
            ui_event ui_evt2 = click2;
            route_event(ui_evt2, path2);
        }

        // Focus should switch
        CHECK(input->get_focused() == text_view2);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Event continues to children") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();
    (void)input;  // Not used in this test

    // Create custom label that tracks events
    class tracking_label : public label<test_canvas_backend> {
    public:
        bool received_event = false;

        explicit tracking_label(const std::string& text)
            : label<test_canvas_backend>(text) {}

        bool handle_event(const ui_event& evt, event_phase phase) override {
            (void)phase;
            if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
                if (mouse_evt->act == mouse_event::action::press) {
                    received_event = true;
                }
            }
            return label<test_canvas_backend>::handle_event(evt, phase);
        }
    };

    // We need to test that events continue to children, but this requires
    // manually constructing the text_view with custom labels, which is complex.
    // For now, we'll verify that text_view returns false from handle_event
    // so the event can continue.

    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    text_view_widget->set_text("Line 1\nLine 2");

    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto _ = root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    // Verify text_view doesn't consume the event
    mouse_event click{
        .x = 5,
        .y = 5,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };

    hit_test_path<test_canvas_backend> path;
    auto* target = root->hit_test(5, 5, path);

    if (target) {
        ui_event ui_evt = click;
        bool consumed = route_event(ui_evt, path);

        // text_view should NOT consume the event (returns false)
        // so it can continue to children
        CHECK_FALSE(consumed);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Focus only on mouse press") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view_ptr = text_view_widget.get();
    text_view_widget->set_text("Line 1");

    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto _ = root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    SUBCASE("Mouse release doesn't give focus") {
        mouse_event release{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::release,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 5, path);

        if (target) {
            ui_event ui_evt = release;
            route_event(ui_evt, path);
        }

        // Should NOT have focus (only press gives focus)
        CHECK(input->get_focused() == nullptr);
    }

    SUBCASE("Mouse move doesn't give focus") {
        mouse_event move{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::none,
            .act = mouse_event::action::move,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 5, path);

        if (target) {
            ui_event ui_evt = move;
            route_event(ui_evt, path);
        }

        // Should NOT have focus
        CHECK(input->get_focused() == nullptr);
    }

    SUBCASE("Mouse press DOES give focus") {
        mouse_event press{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 5, path);

        if (target) {
            ui_event ui_evt = press;
            route_event(ui_evt, path);
        }

        // Should have focus
        CHECK(input->get_focused() == text_view_ptr);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Capture phase interception") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view_ptr = text_view_widget.get();
    text_view_widget->set_text("Line 1\nLine 2\nLine 3");

    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto _ = root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    SUBCASE("Verify capture phase is used") {
        // This test verifies the architecture:
        // 1. Hit test finds deepest child (likely a label)
        // 2. Event routing starts with capture phase
        // 3. text_view intercepts in capture phase BEFORE label
        // 4. text_view requests focus and returns false
        // 5. Event continues to label

        mouse_event click{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 5, path);

        REQUIRE(target != nullptr);

        // Verify text_view is in the path (capture phase will deliver to it)
        CHECK(path.contains(text_view_ptr));

        // Route event
        ui_event ui_evt = click;
        route_event(ui_evt, path);

        // text_view should have focus (even though target might be label)
        CHECK(input->get_focused() == text_view_ptr);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Non-focusable text_view") {
    // Create root panel for testing
    auto root_panel = create_test_root();
    auto* root = root_panel.get();

    // Get input service
    auto* input = ui_services<test_canvas_backend>::input();

    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
    auto* text_view_ptr = text_view_widget.get();
    text_view_widget->set_text("Line 1");

    // Make text_view non-focusable
    text_view_widget->set_focusable(false);

    root->add_child(std::move(text_view_widget));
    [[maybe_unused]] auto _ = root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    SUBCASE("Click doesn't give focus to non-focusable widget") {
        mouse_event click{
            .x = 5,
            .y = 5,
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };

        hit_test_path<test_canvas_backend> path;
        auto* target = root->hit_test(5, 5, path);

        if (target) {
            ui_event ui_evt = click;
            route_event(ui_evt, path);
        }

        // Should NOT have focus (not focusable)
        CHECK(input->get_focused() != text_view_ptr);
        CHECK(input->get_focused() == nullptr);
    }
}
