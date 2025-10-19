/**
 * @file test_event_target_comprehensive.cc
 * @brief Comprehensive tests for event_target event processing
 *
 * Tests the event_target class's event processing pipeline, including:
 * - Mouse button click generation (press + release = click)
 * - Event concept priority (button events before position events)
 * - Mouse enter/leave tracking
 * - State management (hover, pressed, focus)
 * - Event handler callbacks
 */

#include <doctest/doctest.h>
#include <onyxui/event_target.hh>
#include <utils/test_backend.hh>

using namespace onyxui;

// ======================================================================
// Test Helper: Concrete event_target implementation for testing
// ======================================================================

class test_event_target_comprehensive : public event_target<test_backend> {
public:
    // Track which handlers were called
    int enter_count = 0;
    int leave_count = 0;
    int move_count = 0;
    int down_count = 0;
    int up_count = 0;
    int click_count = 0;

    // Last event details
    int last_x = -1;
    int last_y = -1;
    int last_button = -1;

    // Define hit testing bounds
    int bounds_x = 0;
    int bounds_y = 0;
    int bounds_w = 100;
    int bounds_h = 50;

    // Implement required abstract method
    [[nodiscard]] bool is_inside(int x, int y) const override {
        return x >= bounds_x && x < (bounds_x + bounds_w) &&
               y >= bounds_y && y < (bounds_y + bounds_h);
    }

protected:
    // Override handlers to track calls
    bool handle_mouse_enter() override {
        enter_count++;
        return event_target::handle_mouse_enter();
    }

    bool handle_mouse_leave() override {
        leave_count++;
        return event_target::handle_mouse_leave();
    }

    bool handle_mouse_move(int x, int y) override {
        move_count++;
        last_x = x;
        last_y = y;
        return event_target::handle_mouse_move(x, y);
    }

    bool handle_mouse_down(int x, int y, int button) override {
        down_count++;
        last_x = x;
        last_y = y;
        last_button = button;
        return event_target::handle_mouse_down(x, y, button);
    }

    bool handle_mouse_up(int x, int y, int button) override {
        up_count++;
        last_x = x;
        last_y = y;
        last_button = button;
        return event_target::handle_mouse_up(x, y, button);
    }

    bool handle_click(int x, int y) override {
        click_count++;
        last_x = x;
        last_y = y;
        return event_target::handle_click(x, y);
    }
};

// ======================================================================
// Mouse Button Click Generation Tests
// ======================================================================

// Verify concept satisfaction at compile time
static_assert(MousePositionEvent<test_backend::test_mouse_event>, "test_mouse_event must satisfy MousePositionEvent");
static_assert(MouseButtonEvent<test_backend::test_mouse_event>, "test_mouse_event must satisfy MouseButtonEvent");

TEST_SUITE("EventTarget - Mouse Click Generation") {
    TEST_CASE("Press inside + Release inside = Click generated") {
        test_event_target_comprehensive target;

        // Press button inside
        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;

        target.process_event_impl(press);
        CHECK(target.down_count == 1);
        CHECK(target.is_pressed());

        // Release button inside
        test_backend::test_mouse_event release;
        release.x = 50;
        release.y = 25;
        release.button = 1;
        release.pressed = false;

        target.process_event_impl(release);
        CHECK(target.up_count == 1);
        CHECK(target.click_count == 1);  // ✅ Click generated!
        CHECK_FALSE(target.is_pressed());
    }

    TEST_CASE("Press outside = No click generated") {
        test_event_target_comprehensive target;

        // Press button outside bounds
        test_backend::test_mouse_event press;
        press.x = 200;  // Outside bounds
        press.y = 25;
        press.button = 1;
        press.pressed = true;

        target.process_event_impl(press);
        CHECK(target.down_count == 0);  // Not called
        CHECK_FALSE(target.is_pressed());

        // Release inside
        test_backend::test_mouse_event release;
        release.x = 50;
        release.y = 25;
        release.button = 1;
        release.pressed = false;

        target.process_event_impl(release);
        CHECK(target.click_count == 0);  // ❌ No click (press was outside)
    }

    TEST_CASE("Press inside + Release outside = No click generated") {
        test_event_target_comprehensive target;

        // Press button inside
        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;

        target.process_event_impl(press);
        CHECK(target.down_count == 1);
        CHECK(target.is_pressed());

        // Release button outside
        test_backend::test_mouse_event release;
        release.x = 200;  // Outside bounds
        release.y = 25;
        release.button = 1;
        release.pressed = false;

        target.process_event_impl(release);
        CHECK(target.up_count == 1);  // Still called
        CHECK(target.click_count == 0);  // ❌ No click (release outside)
    }

    TEST_CASE("Multiple clicks") {
        test_event_target_comprehensive target;

        // First click
        test_backend::test_mouse_event press1;
        press1.x = 50;
        press1.y = 25;
        press1.button = 1;
        press1.pressed = true;
        target.process_event_impl(press1);

        test_backend::test_mouse_event release1;
        release1.x = 50;
        release1.y = 25;
        release1.button = 1;
        release1.pressed = false;
        target.process_event_impl(release1);

        CHECK(target.click_count == 1);

        // Second click
        test_backend::test_mouse_event press2;
        press2.x = 60;
        press2.y = 30;
        press2.button = 1;
        press2.pressed = true;
        target.process_event_impl(press2);

        test_backend::test_mouse_event release2;
        release2.x = 60;
        release2.y = 30;
        release2.button = 1;
        release2.pressed = false;
        target.process_event_impl(release2);

        CHECK(target.click_count == 2);  // ✅ Two clicks
    }

    TEST_CASE("Different mouse buttons") {
        test_event_target_comprehensive target;

        SUBCASE("Left button") {
            test_backend::test_mouse_event press;
            press.x = 50;
            press.y = 25;
            press.button = 1;
            press.pressed = true;
            target.process_event_impl(press);

            test_backend::test_mouse_event release;
            release.x = 50;
            release.y = 25;
            release.button = 1;
            release.pressed = false;
            target.process_event_impl(release);

            CHECK(target.click_count == 1);
            CHECK(target.last_button == 1);
        }

        SUBCASE("Right button") {
            test_backend::test_mouse_event press;
            press.x = 50;
            press.y = 25;
            press.button = 3;
            press.pressed = true;
            target.process_event_impl(press);

            test_backend::test_mouse_event release;
            release.x = 50;
            release.y = 25;
            release.button = 3;
            release.pressed = false;
            target.process_event_impl(release);

            CHECK(target.click_count == 1);
            CHECK(target.last_button == 3);
        }
    }
}

// ======================================================================
// Event Concept Priority Tests (THE CRITICAL BUG!)
// ======================================================================

TEST_SUITE("EventTarget - Event Concept Priority") {
    TEST_CASE("MouseButtonEvent has priority over MousePositionEvent") {
        test_event_target_comprehensive target;

        SUBCASE("Button press is NOT treated as move event") {
            // Create a mouse button press event
            // This satisfies BOTH MouseButtonEvent AND MousePositionEvent
            test_backend::test_mouse_event press;
            press.x = 50;
            press.y = 25;
            press.button = 1;
            press.pressed = true;

            // Verify it satisfies both concepts
            CHECK(MouseButtonEvent<test_backend::test_mouse_event>);
            CHECK(MousePositionEvent<test_backend::test_mouse_event>);

            // Process event - should be treated as button event, NOT move!
            target.process_event_impl(press);

            CHECK(target.down_count == 1);     // ✅ Mouse down called
            CHECK(target.move_count == 0);     // ❌ Mouse move NOT called
            CHECK(target.is_pressed());
        }

        SUBCASE("Button release is NOT treated as move event") {
            // First press
            test_backend::test_mouse_event press;
            press.x = 50;
            press.y = 25;
            press.button = 1;
            press.pressed = true;
            target.process_event_impl(press);

            // Then release
            test_backend::test_mouse_event release;
            release.x = 50;
            release.y = 25;
            release.button = 1;
            release.pressed = false;

            target.process_event_impl(release);

            CHECK(target.up_count == 1);      // ✅ Mouse up called
            CHECK(target.click_count == 1);   // ✅ Click generated
            CHECK(target.move_count == 0);    // ❌ Move still NOT called
        }

        SUBCASE("Full click sequence generates no move events") {
            // Press
            test_backend::test_mouse_event press;
            press.x = 50;
            press.y = 25;
            press.button = 1;
            press.pressed = true;
            target.process_event_impl(press);

            // Release
            test_backend::test_mouse_event release;
            release.x = 50;
            release.y = 25;
            release.button = 1;
            release.pressed = false;
            target.process_event_impl(release);

            // Verify: down + up + click, but NO move
            CHECK(target.down_count == 1);
            CHECK(target.up_count == 1);
            CHECK(target.click_count == 1);
            CHECK(target.move_count == 0);  // Critical: no move events!
        }
    }
}

// ======================================================================
// Mouse Enter/Leave Tests
// ======================================================================

TEST_SUITE("EventTarget - Mouse Enter/Leave") {
    TEST_CASE("Mouse enter when moving into bounds") {
        test_event_target_comprehensive target;

        // Move mouse outside bounds
        test_backend::test_mouse_event outside;
        outside.x = 200;
        outside.y = 25;
        outside.button = 0;
        outside.pressed = false;

        target.process_event_impl(outside);
        CHECK(target.enter_count == 0);
        CHECK_FALSE(target.is_hovered());

        // Move mouse inside bounds
        test_backend::test_mouse_event inside;
        inside.x = 50;
        inside.y = 25;
        inside.button = 0;
        inside.pressed = false;

        target.process_event_impl(inside);
        CHECK(target.enter_count == 1);  // ✅ Enter event!
        CHECK(target.is_hovered());
    }

    TEST_CASE("Mouse leave when moving out of bounds") {
        test_event_target_comprehensive target;

        // Start inside
        test_backend::test_mouse_event inside;
        inside.x = 50;
        inside.y = 25;
        inside.button = 0;
        inside.pressed = false;
        target.process_event_impl(inside);

        CHECK(target.enter_count == 1);
        CHECK(target.is_hovered());

        // Move outside
        test_backend::test_mouse_event outside;
        outside.x = 200;
        outside.y = 25;
        outside.button = 0;
        outside.pressed = false;
        target.process_event_impl(outside);

        CHECK(target.leave_count == 1);  // ✅ Leave event!
        CHECK_FALSE(target.is_hovered());
    }

    TEST_CASE("No enter/leave when staying inside") {
        test_event_target_comprehensive target;

        // Move to position 1 inside
        test_backend::test_mouse_event pos1;
        pos1.x = 50;
        pos1.y = 25;
        pos1.button = 0;
        pos1.pressed = false;
        target.process_event_impl(pos1);

        CHECK(target.enter_count == 1);

        // Move to position 2 inside (still inside!)
        test_backend::test_mouse_event pos2;
        pos2.x = 60;
        pos2.y = 30;
        pos2.button = 0;
        pos2.pressed = false;
        target.process_event_impl(pos2);

        // Still only 1 enter, no leave
        CHECK(target.enter_count == 1);
        CHECK(target.leave_count == 0);
        CHECK(target.move_count == 1);  // Only pos2 triggers move (pos1 triggers enter)
    }
}

// ======================================================================
// State Management Tests
// ======================================================================

TEST_SUITE("EventTarget - State Management") {
    TEST_CASE("Pressed state tracking") {
        test_event_target_comprehensive target;

        CHECK_FALSE(target.is_pressed());

        // Press
        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;
        target.process_event_impl(press);

        CHECK(target.is_pressed());

        // Release
        test_backend::test_mouse_event release;
        release.x = 50;
        release.y = 25;
        release.button = 1;
        release.pressed = false;
        target.process_event_impl(release);

        CHECK_FALSE(target.is_pressed());
    }

    TEST_CASE("Hovered state tracking") {
        test_event_target_comprehensive target;

        CHECK_FALSE(target.is_hovered());

        // Move inside
        test_backend::test_mouse_event inside;
        inside.x = 50;
        inside.y = 25;
        inside.button = 0;
        inside.pressed = false;
        target.process_event_impl(inside);

        CHECK(target.is_hovered());

        // Move outside
        test_backend::test_mouse_event outside;
        outside.x = 200;
        outside.y = 25;
        outside.button = 0;
        outside.pressed = false;
        target.process_event_impl(outside);

        CHECK_FALSE(target.is_hovered());
    }

    TEST_CASE("Disabled target doesn't process events") {
        test_event_target_comprehensive target;
        target.set_enabled(false);

        // Try to click
        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;

        bool handled = target.process_event_impl(press);

        CHECK_FALSE(handled);
        CHECK(target.down_count == 0);  // Not called!
        CHECK_FALSE(target.is_pressed());
    }

    TEST_CASE("Re-enabling target allows events") {
        test_event_target_comprehensive target;
        target.set_enabled(false);
        target.set_enabled(true);

        // Click should work now
        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;

        target.process_event_impl(press);

        // Note: handled returns false if no callback is set, but the event is still processed
        CHECK(target.down_count == 1);  // Event was processed!
        CHECK(target.is_pressed());
    }
}

// ======================================================================
// Callback Tests
// ======================================================================

TEST_SUITE("EventTarget - Callbacks") {
    TEST_CASE("Click callback is invoked") {
        test_event_target_comprehensive target;

        int callback_count = 0;
        target.set_on_click([&](int x, int y) {
            callback_count++;
            return true;
        });

        // Complete click
        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;
        target.process_event_impl(press);

        test_backend::test_mouse_event release;
        release.x = 50;
        release.y = 25;
        release.button = 1;
        release.pressed = false;
        target.process_event_impl(release);

        CHECK(callback_count == 1);  // ✅ Callback invoked!
    }

    TEST_CASE("Mouse enter/leave callbacks") {
        test_event_target_comprehensive target;

        int enter_callback_count = 0;
        int leave_callback_count = 0;

        target.set_on_mouse_enter([&]() {
            enter_callback_count++;
            return true;
        });

        target.set_on_mouse_leave([&]() {
            leave_callback_count++;
            return true;
        });

        // Move inside
        test_backend::test_mouse_event inside;
        inside.x = 50;
        inside.y = 25;
        inside.button = 0;
        inside.pressed = false;
        target.process_event_impl(inside);

        CHECK(enter_callback_count == 1);

        // Move outside
        test_backend::test_mouse_event outside;
        outside.x = 200;
        outside.y = 25;
        outside.button = 0;
        outside.pressed = false;
        target.process_event_impl(outside);

        CHECK(leave_callback_count == 1);
    }
}

// ======================================================================
// Edge Cases
// ======================================================================

TEST_SUITE("EventTarget - Edge Cases") {
    TEST_CASE("Rapid press/release") {
        test_event_target_comprehensive target;

        // Rapidly click 10 times
        for (int i = 0; i < 10; i++) {
            test_backend::test_mouse_event press;
            press.x = 50;
            press.y = 25;
            press.button = 1;
            press.pressed = true;
            target.process_event_impl(press);

            test_backend::test_mouse_event release;
            release.x = 50;
            release.y = 25;
            release.button = 1;
            release.pressed = false;
            target.process_event_impl(release);
        }

        CHECK(target.click_count == 10);
    }

    TEST_CASE("Press without release doesn't crash") {
        test_event_target_comprehensive target;

        test_backend::test_mouse_event press;
        press.x = 50;
        press.y = 25;
        press.button = 1;
        press.pressed = true;

        target.process_event_impl(press);
        CHECK(target.is_pressed());

        // Never release - state should persist
        CHECK(target.is_pressed());
    }

    TEST_CASE("Boundary coordinates") {
        test_event_target_comprehensive target;
        target.bounds_x = 0;
        target.bounds_y = 0;
        target.bounds_w = 100;
        target.bounds_h = 50;

        SUBCASE("Top-left corner") {
            test_backend::test_mouse_event evt;
            evt.x = 0;
            evt.y = 0;
            evt.button = 1;
            evt.pressed = true;
            target.process_event_impl(evt);

            CHECK(target.down_count == 1);  // Inside bounds
        }

        SUBCASE("Just outside right edge") {
            test_backend::test_mouse_event evt;
            evt.x = 100;  // bounds_x + bounds_w
            evt.y = 25;
            evt.button = 1;
            evt.pressed = true;
            target.process_event_impl(evt);

            CHECK(target.down_count == 0);  // Outside bounds
        }

        SUBCASE("Just inside right edge") {
            test_backend::test_mouse_event evt;
            evt.x = 99;
            evt.y = 25;
            evt.button = 1;
            evt.pressed = true;
            target.process_event_impl(evt);

            CHECK(target.down_count == 1);  // Inside bounds
        }
    }
}
