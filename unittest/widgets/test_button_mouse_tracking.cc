/**
 * @file test_button_mouse_tracking.cc
 * @brief Tests for button mouse event tracking and state management
 * @date 2025-10-25
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/button.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_handle.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "widgets.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

// Helper functions for simulating mouse events
namespace {
    template<typename Widget>
    void simulate_mouse_press(Widget& w, int x, int y) {
        mouse_event evt{.x = x, .y = y, .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
        w.handle_event(ui_event{evt}, event_phase::target);
    }

    template<typename Widget>
    void simulate_mouse_release(Widget& w, int x, int y) {
        mouse_event evt{.x = x, .y = y, .btn = mouse_event::button::left, .act = mouse_event::action::release, .modifiers = {}};
        w.handle_event(ui_event{evt}, event_phase::target);
    }

    template<typename Widget>
    void simulate_mouse_move(Widget& w, int x, int y) {
        mouse_event evt{.x = x, .y = y, .btn = mouse_event::button::none, .act = mouse_event::action::move, .modifiers = {}};
        w.handle_event(ui_event{evt}, event_phase::target);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Button - Mouse tracking and state management") {
    SUBCASE("Press then drag away - visual state should persist if pressed") {
        test_button<Backend> btn("Test");
        btn.arrange(logical_rect{0_lu, 0_lu, 100_lu, 20_lu});  // Set bounds for hit testing

        // Initially button is in normal state
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_visually_pressed());

        // Simulate mouse down event (button becomes pressed)
        simulate_mouse_press(btn, 10, 2);
        CHECK(btn.is_visually_pressed());  // Should be pressed
        CHECK_FALSE(btn.is_visually_normal());

        // Simulate mouse leave while button is held down (drag away)
        // This is the CRITICAL BUG FIX: mouse move outside should NOT
        // reset to normal if button is still pressed
        simulate_mouse_move(btn, 200, 200);  // Move outside bounds

        // Button should STILL be visually pressed after mouse leaves
        // because the mouse button is still held down
        CHECK(btn.is_visually_pressed());  // Should STILL be pressed (fix works!)

        // Simulate mouse up (release)
        simulate_mouse_release(btn, 200, 200);  // Release outside

        // NOW button should return to normal state
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_visually_pressed());
    }

    SUBCASE("Hover then leave - should return to normal if not pressed") {
        test_button<Backend> btn("Test");
        btn.arrange(logical_rect{0_lu, 0_lu, 100_lu, 20_lu});  // Set bounds for hit testing

        // Initially normal
        CHECK(btn.is_visually_normal());

        // Mouse enters
        simulate_mouse_move(btn, 50, 10);  // Move inside bounds
        CHECK(btn.is_visually_hovered());

        // Mouse leaves (without pressing)
        simulate_mouse_move(btn, 200, 200);  // Move outside bounds

        // Should return to normal
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Press, release inside - state should return to normal") {
        test_button<Backend> btn("Test");
        btn.arrange(logical_rect{0_lu, 0_lu, 100_lu, 20_lu});  // Set bounds for hit testing

        // Initially normal
        CHECK(btn.is_visually_normal());

        // Press
        simulate_mouse_press(btn, 10, 2);
        CHECK(btn.is_visually_pressed());

        // Release at same position (still inside bounds)
        simulate_mouse_release(btn, 10, 2);

        // With unified event API: mouse is still at (10,2) which is inside bounds
        // So button should be in hover state, not normal!
        CHECK(btn.is_visually_hovered());
    }

    SUBCASE("Press, release while hovering - should return to HOVER state (Bug Reproduction)") {
        test_button<Backend> btn("Test");
        btn.arrange(logical_rect{0_lu, 0_lu, 100_lu, 20_lu});  // Set bounds for hit testing

        // Initially normal
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_hovered());

        // Mouse enters button
        simulate_mouse_move(btn, 50, 10);  // Move inside bounds
        CHECK(btn.is_visually_hovered());
        CHECK(btn.is_hovered());  // is_hovered() should return true

        // Press mouse down (button becomes pressed)
        simulate_mouse_press(btn, 50, 10);
        CHECK(btn.is_visually_pressed());

        // Release mouse up WHILE STILL HOVERING (mouse hasn't left)
        simulate_mouse_release(btn, 50, 10);

        // CRITICAL BUG: Button should return to HOVER state, not NORMAL
        // because the mouse is still over the button!
        CHECK(btn.is_hovered());  // This should be true (mouse still over button)
        CHECK(btn.is_visually_hovered());  // Should be in hover state
        CHECK_FALSE(btn.is_visually_normal());  // Should NOT be normal

        // Now mouse leaves
        simulate_mouse_move(btn, 200, 200);  // Move outside bounds

        // NOW it should be normal
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_hovered());
    }

    SUBCASE("Multiple buttons - drag away from one, press another - states should be independent") {
        test_button<Backend> btn1("Button 1");
        test_button<Backend> btn2("Button 2");
        btn1.arrange(logical_rect{0_lu, 0_lu, 100_lu, 20_lu});  // Set bounds for hit testing
        btn2.arrange(logical_rect{0_lu, 30_lu, 100_lu, 20_lu});  // Set bounds below btn1

        // Press on button 1
        simulate_mouse_press(btn1, 50, 10);
        CHECK(btn1.is_visually_pressed());

        // Drag away from button 1
        simulate_mouse_move(btn1, 200, 200);  // Move outside bounds

        // Button 1 should still be visually pressed (this is the fix!)
        CHECK(btn1.is_visually_pressed());

        // Release on button 1 (even though mouse left)
        simulate_mouse_release(btn1, 200, 200);

        // Button 1 should be back to normal
        CHECK(btn1.is_visually_normal());

        // NOW button 2 should be able to be pressed normally
        // (this tests that btn1's state doesn't interfere with btn2)
        simulate_mouse_press(btn2, 50, 40);
        CHECK(btn2.is_visually_pressed());

        simulate_mouse_release(btn2, 50, 40);

        // With unified event API: mouse is still at (50,40) inside btn2 bounds
        // So btn2 should be in hover state after release
        CHECK(btn2.is_visually_hovered());

        // btn1 is in normal state (mouse is not over it)
        // btn2 is in hover state (mouse is still over it)
        CHECK(btn1.is_visually_normal());
        CHECK(btn2.is_visually_hovered());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Button - Bug: is_hovered() returns false after mouse up") {
    SUBCASE("Mouse enter, press, release - is_hovered() should stay true") {
        test_button<Backend> btn("Test");
        btn.arrange(logical_rect{0_lu, 0_lu, 100_lu, 20_lu});  // Set bounds for hit testing

        // Simulate proper mouse tracking sequence
        // 1. Mouse enters button
        simulate_mouse_move(btn, 50, 10);  // Move inside bounds
        CHECK(btn.is_hovered());  // Should be true

        // 2. Mouse down (press)
        simulate_mouse_press(btn, 50, 10);
        CHECK(btn.is_visually_pressed());

        // 3. Mouse up (release) - MOUSE STILL OVER BUTTON!
        // This is the critical moment - release is called,
        // but mouse hasn't moved away
        simulate_mouse_release(btn, 50, 10);

        // BUG: is_hovered() should STILL be true because mouse never left!
        // The stateful_widget calls is_hovered() to decide whether to go to
        // hover or normal state
        CHECK(btn.is_hovered());  // FAILS if bug exists - returns false
        CHECK(btn.is_visually_hovered());  // Should be in hover state

        // 4. Now mouse leaves
        simulate_mouse_move(btn, 200, 200);  // Move outside bounds
        CHECK_FALSE(btn.is_hovered());
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Real event sequence: mouse move, click, release (Demo Bug Reproduction)") {
        // Use fixture's ui_context with theme for measurement
        test_button<Backend> btn("Test");

        // Position button at specific location
        auto size = btn.measure(100_lu, 20_lu);
        (void)size;  // Measurement needed for arrange
        btn.arrange(logical_rect{10_lu, 10_lu, 100_lu, 20_lu});

        // === REAL EVENT SEQUENCE FROM DEMO ===

        // 1. Mouse moves over button (at position 50, 15 - inside button bounds)
        simulate_mouse_move(btn, 50, 15);

        // After mouse move, button should be hovering
        CHECK(btn.is_hovered());
        CHECK(btn.is_visually_hovered());

        // 2. Mouse button down (click at same position)
        simulate_mouse_press(btn, 50, 15);

        // Button should be in pressed state
        CHECK(btn.is_visually_pressed());

        // 3. Mouse button up (release at same position - mouse hasn't moved!)
        simulate_mouse_release(btn, 50, 15);

        // CRITICAL BUG CHECK:
        // After release, mouse is STILL at position (50, 15) inside button bounds
        // So is_hovered() should STILL be true, and button should be in hover state
        CHECK(btn.is_hovered());  // BUG: This might fail - is_hovered() returns false
        CHECK(btn.is_visually_hovered());  // BUG: Button goes to normal instead of hover

        // 4. Mouse moves away from button
        simulate_mouse_move(btn, 200, 200);  // Move outside button bounds

        // NOW button should be normal
        CHECK_FALSE(btn.is_hovered());
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Click button 1, then click button 2 sequence") {
        // Use fixture's ui_context with theme for measurement
        test_button<Backend> btn1("Normal");
        test_button<Backend> btn2("Quit");

        // Position buttons
        auto size1 = btn1.measure(100_lu, 20_lu);
        (void)size1;  // Measurement needed for arrange
        btn1.arrange(logical_rect{10_lu, 10_lu, 100_lu, 20_lu});

        auto size2 = btn2.measure(100_lu, 20_lu);
        (void)size2;  // Measurement needed for arrange
        btn2.arrange(logical_rect{10_lu, 40_lu, 100_lu, 20_lu});  // Below btn1

        // === Click button 1 ===

        // Mouse move to button 1
        simulate_mouse_move(btn1, 50, 15);
        CHECK(btn1.is_hovered());
        CHECK(btn1.is_visually_hovered());

        // Click button 1 (down + up)
        simulate_mouse_press(btn1, 50, 15);
        CHECK(btn1.is_visually_pressed());

        simulate_mouse_release(btn1, 50, 15);

        // After release, button 1 should STILL be hovered (BUG CHECK)
        CHECK(btn1.is_hovered());
        CHECK(btn1.is_visually_hovered());

        // === Move to button 2 ===

        // Mouse leaves button 1
        simulate_mouse_move(btn1, 200, 200);  // Move away
        CHECK_FALSE(btn1.is_hovered());
        CHECK(btn1.is_visually_normal());

        // Mouse moves to button 2
        simulate_mouse_move(btn2, 50, 45);
        CHECK(btn2.is_hovered());
        CHECK(btn2.is_visually_hovered());

        // Click button 2 (down + up)
        simulate_mouse_press(btn2, 50, 45);
        CHECK(btn2.is_visually_pressed());

        simulate_mouse_release(btn2, 50, 45);

        // After release, button 2 should be hovered
        CHECK(btn2.is_hovered());
        CHECK(btn2.is_visually_hovered());
    }
}
