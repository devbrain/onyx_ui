/**
 * @file test_button_mouse_tracking.cc
 * @brief Tests for button mouse event tracking and state management
 * @date 2025-10-25
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/button.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/vbox.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_handle.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_context.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "widgets.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Button - Mouse tracking and state management") {
    SUBCASE("Press then drag away - visual state should persist if pressed") {
        test_button<Backend> btn("Test");

        // Initially button is in normal state
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_visually_pressed());

        // Simulate mouse down event (button becomes pressed)
        btn.handle_mouse_down(10, 2, 1);
        CHECK(btn.is_visually_pressed());  // Should be pressed
        CHECK_FALSE(btn.is_visually_normal());

        // Simulate mouse leave while button is held down (drag away)
        // This is the CRITICAL BUG FIX: handle_mouse_leave should NOT
        // reset to normal if button is still pressed
        btn.handle_mouse_leave();

        // Button should STILL be visually pressed after mouse leaves
        // because the mouse button is still held down
        CHECK(btn.is_visually_pressed());  // Should STILL be pressed (fix works!)

        // Simulate mouse up (release)
        btn.handle_mouse_up(100, 100, 1);

        // NOW button should return to normal state
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_visually_pressed());
    }

    SUBCASE("Hover then leave - should return to normal if not pressed") {
        test_button<Backend> btn("Test");

        // Initially normal
        CHECK(btn.is_visually_normal());

        // Mouse enters
        btn.handle_mouse_enter();
        CHECK(btn.is_visually_hovered());

        // Mouse leaves (without pressing)
        btn.handle_mouse_leave();

        // Should return to normal
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Press, release inside - state should return to normal") {
        test_button<Backend> btn("Test");

        // Initially normal
        CHECK(btn.is_visually_normal());

        // Press
        btn.handle_mouse_down(10, 2, 1);
        CHECK(btn.is_visually_pressed());

        // Release (note: click signal requires process_mouse_button, not tested here)
        btn.handle_mouse_up(10, 2, 1);

        // State should return to normal after release
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Press, release while hovering - should return to HOVER state (Bug Reproduction)") {
        test_button<Backend> btn("Test");

        // Initially normal
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_hovered());

        // Mouse enters button
        btn.handle_mouse_enter();
        CHECK(btn.is_visually_hovered());
        CHECK(btn.is_hovered());  // is_hovered() should return true

        // Press mouse down (button becomes pressed)
        btn.handle_mouse_down(10, 2, 1);
        CHECK(btn.is_visually_pressed());

        // Release mouse up WHILE STILL HOVERING (mouse hasn't left)
        btn.handle_mouse_up(10, 2, 1);

        // CRITICAL BUG: Button should return to HOVER state, not NORMAL
        // because the mouse is still over the button!
        CHECK(btn.is_hovered());  // This should be true (mouse still over button)
        CHECK(btn.is_visually_hovered());  // Should be in hover state
        CHECK_FALSE(btn.is_visually_normal());  // Should NOT be normal

        // Now mouse leaves
        btn.handle_mouse_leave();

        // NOW it should be normal
        CHECK(btn.is_visually_normal());
        CHECK_FALSE(btn.is_hovered());
    }

    SUBCASE("Multiple buttons - drag away from one, press another - states should be independent") {
        test_button<Backend> btn1("Button 1");
        test_button<Backend> btn2("Button 2");

        // Press on button 1
        btn1.handle_mouse_down(5, 2, 1);
        CHECK(btn1.is_visually_pressed());

        // Drag away from button 1
        btn1.handle_mouse_leave();

        // Button 1 should still be visually pressed (this is the fix!)
        CHECK(btn1.is_visually_pressed());

        // Release on button 1 (even though mouse left)
        btn1.handle_mouse_up(100, 100, 1);

        // Button 1 should be back to normal
        CHECK(btn1.is_visually_normal());

        // NOW button 2 should be able to be pressed normally
        // (this tests that btn1's state doesn't interfere with btn2)
        btn2.handle_mouse_down(50, 2, 1);
        CHECK(btn2.is_visually_pressed());

        btn2.handle_mouse_up(50, 2, 1);

        // Button 2 should return to normal
        CHECK(btn2.is_visually_normal());

        // Both buttons should be in normal state
        CHECK(btn1.is_visually_normal());
        CHECK(btn2.is_visually_normal());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Button - Bug: is_hovered() returns false after mouse up") {
    SUBCASE("Mouse enter, press, release - is_hovered() should stay true") {
        test_button<Backend> btn("Test");

        // Simulate proper mouse tracking sequence
        // 1. Mouse enters button
        btn.handle_mouse_enter();
        CHECK(btn.is_hovered());  // Should be true

        // 2. Mouse down (press)
        btn.handle_mouse_down(10, 2, 1);
        CHECK(btn.is_visually_pressed());

        // 3. Mouse up (release) - MOUSE STILL OVER BUTTON!
        // This is the critical moment - handle_mouse_up is called,
        // but handle_mouse_leave was NOT called
        btn.handle_mouse_up(10, 2, 1);

        // BUG: is_hovered() should STILL be true because mouse never left!
        // The stateful_widget calls is_hovered() to decide whether to go to
        // hover or normal state
        CHECK(btn.is_hovered());  // FAILS if bug exists - returns false
        CHECK(btn.is_visually_hovered());  // Should be in hover state

        // 4. Now mouse leaves
        btn.handle_mouse_leave();
        CHECK_FALSE(btn.is_hovered());
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Real event sequence: mouse move, click, release (Demo Bug Reproduction)") {
        // Setup ui_context with theme for measurement
        ui_theme<test_canvas_backend> theme;
        theme.name = "Test";
        scoped_ui_context<test_canvas_backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test");

        test_button<Backend> btn("Test");

        // Position button at specific location
        btn.measure(100, 20);
        btn.arrange(Backend::rect_type{10, 10, 100, 20});

        // === REAL EVENT SEQUENCE FROM DEMO ===

        // 1. Mouse moves over button (at position 50, 15 - inside button bounds)
        btn.process_mouse_move(50, 15);

        // After mouse move, button should be hovering
        CHECK(btn.is_hovered());
        CHECK(btn.is_visually_hovered());

        // 2. Mouse button down (click at same position)
        btn.process_mouse_button(50, 15, 1, true);  // true = pressed

        // Button should be in pressed state
        CHECK(btn.is_visually_pressed());

        // 3. Mouse button up (release at same position - mouse hasn't moved!)
        btn.process_mouse_button(50, 15, 1, false);  // false = released

        // CRITICAL BUG CHECK:
        // After release, mouse is STILL at position (50, 15) inside button bounds
        // So is_hovered() should STILL be true, and button should be in hover state
        CHECK(btn.is_hovered());  // BUG: This might fail - is_hovered() returns false
        CHECK(btn.is_visually_hovered());  // BUG: Button goes to normal instead of hover

        // 4. Mouse moves away from button
        btn.process_mouse_move(200, 200);  // Move outside button bounds

        // NOW button should be normal
        CHECK_FALSE(btn.is_hovered());
        CHECK(btn.is_visually_normal());
    }

    SUBCASE("Click button 1, then click button 2 sequence") {
        // Setup ui_context with theme for measurement
        ui_theme<test_canvas_backend> theme;
        theme.name = "Test";
        scoped_ui_context<test_canvas_backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test");

        test_button<Backend> btn1("Normal");
        test_button<Backend> btn2("Quit");

        // Position buttons
        btn1.measure(100, 20);
        btn1.arrange(Backend::rect_type{10, 10, 100, 20});

        btn2.measure(100, 20);
        btn2.arrange(Backend::rect_type{10, 40, 100, 20});  // Below btn1

        // === Click button 1 ===

        // Mouse move to button 1
        btn1.process_mouse_move(50, 15);
        CHECK(btn1.is_hovered());
        CHECK(btn1.is_visually_hovered());

        // Click button 1 (down + up)
        btn1.process_mouse_button(50, 15, 1, true);
        CHECK(btn1.is_visually_pressed());

        btn1.process_mouse_button(50, 15, 1, false);

        // After release, button 1 should STILL be hovered (BUG CHECK)
        CHECK(btn1.is_hovered());
        CHECK(btn1.is_visually_hovered());

        // === Move to button 2 ===

        // Mouse leaves button 1
        btn1.process_mouse_move(200, 200);  // Move away
        CHECK_FALSE(btn1.is_hovered());
        CHECK(btn1.is_visually_normal());

        // Mouse moves to button 2
        btn2.process_mouse_move(50, 45);
        CHECK(btn2.is_hovered());
        CHECK(btn2.is_visually_hovered());

        // Click button 2 (down + up)
        btn2.process_mouse_button(50, 45, 1, true);
        CHECK(btn2.is_visually_pressed());

        btn2.process_mouse_button(50, 45, 1, false);

        // After release, button 2 should be hovered
        CHECK(btn2.is_hovered());
        CHECK(btn2.is_visually_hovered());
    }
}
