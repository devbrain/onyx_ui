/**
 * @file test_event_phase.cc
 * @brief Tests for event phase infrastructure (capture/target/bubble)
 *
 * Tests the new event_phase enum and phase-aware event handling in event_target.
 * This is Phase 1 of the event routing implementation.
 */

#include <doctest/doctest.h>
#include <onyxui/events/event_phase.hh>
#include <onyxui/core/event_target.hh>
#include <utils/test_backend.hh>

using namespace onyxui;

// ======================================================================
// Test Helpers
// ======================================================================

/**
 * Test event_target that tracks which phases it received
 */
class phase_tracking_target : public event_target<test_backend> {
public:
    // Track which phases were called
    bool capture_called = false;
    bool target_called = false;
    bool bubble_called = false;

    // Track event details
    bool received_mouse_press = false;
    int event_count = 0;

    // Required override
    [[nodiscard]] bool is_inside(int x, int y) const override {
        return x >= 0 && x < 100 && y >= 0 && y < 100;
    }

    using event_target<test_backend>::handle_event;

    // Override phase-aware handler
    bool handle_event(const ui_event& evt, event_phase phase) override {
        event_count++;

        // Track which phase this is
        switch (phase) {
            case event_phase::capture:
                capture_called = true;
                break;
            case event_phase::target:
                target_called = true;
                break;
            case event_phase::bubble:
                bubble_called = true;
                break;
        }

        // Check if it's a mouse press
        if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
            if (mouse_evt->act == mouse_event::action::press) {
                received_mouse_press = true;
            }
        }

        // Don't consume (for testing propagation)
        return false;
    }

    void reset() {
        capture_called = false;
        target_called = false;
        bubble_called = false;
        received_mouse_press = false;
        event_count = 0;
    }
};

/**
 * Test target that only handles CAPTURE phase
 */
class capture_only_target : public event_target<test_backend> {
public:
    bool capture_handled = false;
    bool other_phase_called = false;

    [[nodiscard]] bool is_inside(int x, int y) const override {
        return x >= 0 && x < 100 && y >= 0 && y < 100;
    }

    using event_target<test_backend>::handle_event;

    bool handle_event(const ui_event& evt, event_phase phase) override {
        (void)evt;  // Unused - only testing phase handling
        if (phase == event_phase::capture) {
            capture_handled = true;
            return true;  // Consume in capture
        }
        other_phase_called = true;
        return false;
    }
};

// ======================================================================
// Tests
// ======================================================================

TEST_CASE("event_phase - Enum values") {
    SUBCASE("Phase values are correct") {
        CHECK(static_cast<std::uint8_t>(event_phase::capture) == 0);
        CHECK(static_cast<std::uint8_t>(event_phase::target) == 1);
        CHECK(static_cast<std::uint8_t>(event_phase::bubble) == 2);
    }

    SUBCASE("to_string() returns correct strings") {
        CHECK(std::string(to_string(event_phase::capture)) == "capture");
        CHECK(std::string(to_string(event_phase::target)) == "target");
        CHECK(std::string(to_string(event_phase::bubble)) == "bubble");
    }
}

TEST_CASE("event_target - Phase-aware event handling") {
    phase_tracking_target target;

    // Create a mouse press event
    mouse_event evt{
        .x = 50,
        .y = 50,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = evt;

    SUBCASE("Default implementation handles all phases") {
        // Our phase_tracking_target overrides to track all phases
        target.handle_event(ui_evt, event_phase::capture);
        CHECK(target.capture_called);
        CHECK_FALSE(target.target_called);
        CHECK_FALSE(target.bubble_called);
        CHECK(target.received_mouse_press);

        target.reset();

        target.handle_event(ui_evt, event_phase::target);
        CHECK_FALSE(target.capture_called);
        CHECK(target.target_called);
        CHECK_FALSE(target.bubble_called);
        CHECK(target.received_mouse_press);

        target.reset();

        target.handle_event(ui_evt, event_phase::bubble);
        CHECK_FALSE(target.capture_called);
        CHECK_FALSE(target.target_called);
        CHECK(target.bubble_called);
        CHECK(target.received_mouse_press);
    }

    SUBCASE("Can handle events in specific phase only") {
        capture_only_target capture_target;

        // Capture phase should be handled
        bool handled = capture_target.handle_event(ui_evt, event_phase::capture);
        CHECK(handled);  // Consumed in capture
        CHECK(capture_target.capture_handled);
        CHECK_FALSE(capture_target.other_phase_called);

        // Target phase should not be handled
        handled = capture_target.handle_event(ui_evt, event_phase::target);
        CHECK_FALSE(handled);
        CHECK(capture_target.other_phase_called);

        // Bubble phase should not be handled
        handled = capture_target.handle_event(ui_evt, event_phase::bubble);
        CHECK_FALSE(handled);
    }

    SUBCASE("Multiple phase calls with same event") {
        // Simulate routing through all phases
        target.handle_event(ui_evt, event_phase::capture);
        target.handle_event(ui_evt, event_phase::target);
        target.handle_event(ui_evt, event_phase::bubble);

        // All phases should have been called
        CHECK(target.capture_called);
        CHECK(target.target_called);
        CHECK(target.bubble_called);
        CHECK(target.event_count == 3);
    }
}

TEST_CASE("event_target - Different event types with phases") {
    phase_tracking_target target;

    SUBCASE("Keyboard events with phases") {
        keyboard_event kbd{
            .key = key_code::a,
            .modifiers = key_modifier::none,
            .pressed = true
        };
        ui_event ui_evt = kbd;

        target.handle_event(ui_evt, event_phase::capture);
        CHECK(target.capture_called);

        target.reset();
        target.handle_event(ui_evt, event_phase::target);
        CHECK(target.target_called);

        target.reset();
        target.handle_event(ui_evt, event_phase::bubble);
        CHECK(target.bubble_called);
    }

    SUBCASE("Resize events with phases") {
        resize_event resize{
            .width = 800,
            .height = 600
        };
        ui_event ui_evt = resize;

        target.handle_event(ui_evt, event_phase::capture);
        CHECK(target.capture_called);

        target.reset();
        target.handle_event(ui_evt, event_phase::target);
        CHECK(target.target_called);

        target.reset();
        target.handle_event(ui_evt, event_phase::bubble);
        CHECK(target.bubble_called);
    }
}
