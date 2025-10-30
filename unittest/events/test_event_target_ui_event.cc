/**
 * @file test_event_target_ui_event.cc
 * @brief Unit tests for event_target::handle_event() (Phase 3)
 * @author igor
 * @date 28/10/2025
 *
 * @details
 * Tests the new handle_event() API that accepts ui_event directly,
 * integrating the unified event system with event_target.
 *
 * Coverage:
 * - Keyboard event handling via handle_keyboard()
 * - Mouse event handling via handle_mouse()
 * - Resize event handling via handle_resize()
 * - Backward compatibility with old handle_* methods
 * - Enabled/disabled state
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/core/event_target.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/hotkeys/key_code.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// Test Helper Class
// ============================================================================

namespace {
    /**
     * @brief Test event target that tracks which handlers were called
     */
    template<UIBackend Backend>
    class test_event_target : public event_target<Backend> {
    public:
        int keyboard_count = 0;
        int mouse_count = 0;
        int resize_count = 0;
        int key_down_count = 0;
        int mouse_down_count = 0;

        keyboard_event last_keyboard{};
        mouse_event last_mouse{};
        resize_event last_resize{};

        // Override new Phase 3 handlers
        bool handle_keyboard(const keyboard_event& kbd) override {
            keyboard_count++;
            last_keyboard = kbd;
            return true;
        }

        bool handle_mouse(const mouse_event& mouse) override {
            mouse_count++;
            last_mouse = mouse;
            return true;
        }

        bool handle_resize(const resize_event& resize) override {
            resize_count++;
            last_resize = resize;
            return true;
        }

        // Track old API calls for backward compat testing
        bool handle_key_down(int, bool, bool, bool) override {
            key_down_count++;
            return true;
        }

        bool handle_mouse_down(int, int, int) override {
            mouse_down_count++;
            return true;
        }

        // Required pure virtual methods
        bool is_inside(int, int) const override {
            return true;
        }
    };
}

// ============================================================================
// Keyboard Event Tests
// ============================================================================

TEST_CASE("event_target::handle_event - Keyboard events") {
    test_event_target<test_canvas_backend> target;

    SUBCASE("Character key") {
        keyboard_event kbd{};
        kbd.key = key_code::a;
        kbd.modifiers = key_modifier::none;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.keyboard_count == 1);
        CHECK(target.last_keyboard.key == key_code::a);
        CHECK((target.last_keyboard.modifiers & key_modifier::ctrl) == key_modifier::none);
    }

    SUBCASE("Ctrl+S") {
        keyboard_event kbd{};
        kbd.key = key_code::s;
        kbd.modifiers = key_modifier::ctrl;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.keyboard_count == 1);
        CHECK(target.last_keyboard.key == key_code::s);
        CHECK((target.last_keyboard.modifiers & key_modifier::ctrl) != key_modifier::none);
    }

    SUBCASE("Function key F10") {
        keyboard_event kbd{};
        kbd.key = key_code::f10;
        kbd.modifiers = key_modifier::none;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.keyboard_count == 1);
        CHECK(target.last_keyboard.key == key_code::f10);
    }

    SUBCASE("Arrow down (special key)") {
        keyboard_event kbd{};
        kbd.key = key_code::arrow_down;
        kbd.modifiers = key_modifier::none;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.keyboard_count == 1);
        CHECK(target.last_keyboard.key == key_code::arrow_down);
    }
}

// ============================================================================
// Mouse Event Tests
// ============================================================================

TEST_CASE("event_target::handle_event - Mouse events") {
    test_event_target<test_canvas_backend> target;

    SUBCASE("Left button press") {
        mouse_event mouse{};
        mouse.x = 10;
        mouse.y = 20;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;
        mouse.modifiers = {.ctrl = false, .alt = false, .shift = false};

        ui_event evt = mouse;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.mouse_count == 1);
        CHECK(target.last_mouse.x == 10);
        CHECK(target.last_mouse.y == 20);
        CHECK(target.last_mouse.btn == mouse_event::button::left);
        CHECK(target.last_mouse.act == mouse_event::action::press);
    }

    SUBCASE("Right button press") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::right;
        mouse.act = mouse_event::action::press;

        ui_event evt = mouse;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.mouse_count == 1);
        CHECK(target.last_mouse.btn == mouse_event::button::right);
    }

    SUBCASE("Mouse move") {
        mouse_event mouse{};
        mouse.x = 100;
        mouse.y = 150;
        mouse.btn = mouse_event::button::none;
        mouse.act = mouse_event::action::move;

        ui_event evt = mouse;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.mouse_count == 1);
        CHECK(target.last_mouse.act == mouse_event::action::move);
    }

    SUBCASE("Mouse wheel up") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::none;
        mouse.act = mouse_event::action::wheel_up;

        ui_event evt = mouse;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.mouse_count == 1);
        CHECK(target.last_mouse.act == mouse_event::action::wheel_up);
    }

    SUBCASE("Mouse wheel down") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::none;
        mouse.act = mouse_event::action::wheel_down;

        ui_event evt = mouse;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.mouse_count == 1);
        CHECK(target.last_mouse.act == mouse_event::action::wheel_down);
    }
}

// ============================================================================
// Resize Event Tests
// ============================================================================

TEST_CASE("event_target::handle_event - Resize events") {
    test_event_target<test_canvas_backend> target;

    SUBCASE("Window resized to 1024x768") {
        resize_event resize{};
        resize.width = 1024;
        resize.height = 768;

        ui_event evt = resize;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.resize_count == 1);
        CHECK(target.last_resize.width == 1024);
        CHECK(target.last_resize.height == 768);
    }

    SUBCASE("Terminal resized to 80x24") {
        resize_event resize{};
        resize.width = 80;
        resize.height = 24;

        ui_event evt = resize;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.resize_count == 1);
        CHECK(target.last_resize.width == 80);
        CHECK(target.last_resize.height == 24);
    }
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST_CASE("event_target::handle_event - Backward compatibility") {
    // Test that default handle_keyboard/mouse forward to old API

    class compat_target : public event_target<test_canvas_backend> {
    public:
        int key_down_calls = 0;
        int mouse_down_calls = 0;
        int last_key = 0;
        bool last_shift = false;
        bool last_ctrl = false;

        // Override old API instead of new API
        bool handle_key_down(int key, bool shift, bool ctrl, bool) override {
            key_down_calls++;
            last_key = key;
            last_shift = shift;
            last_ctrl = ctrl;
            return true;
        }

        bool handle_mouse_down(int, int, int) override {
            mouse_down_calls++;
            return true;
        }

        bool is_inside(int, int) const override {
            return true;
        }
    };

    compat_target target;

    SUBCASE("Keyboard event forwards to handle_key_down") {
        keyboard_event kbd{};
        kbd.key = key_code::s;
        kbd.modifiers = key_modifier::ctrl;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.key_down_calls == 1);
        CHECK(target.last_key == 's');
        CHECK(target.last_ctrl);
        CHECK_FALSE(target.last_shift);
    }

    SUBCASE("Mouse event forwards to handle_mouse_down") {
        mouse_event mouse{};
        mouse.x = 10;
        mouse.y = 20;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;

        ui_event evt = mouse;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.mouse_down_calls == 1);
    }
}

// ============================================================================
// Enabled/Disabled State Tests
// ============================================================================

TEST_CASE("event_target::handle_event - Enabled/disabled state") {
    test_event_target<test_canvas_backend> target;

    SUBCASE("Disabled target ignores events") {
        target.set_enabled(false);

        keyboard_event kbd{};
        kbd.key = key_code::a;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK_FALSE(handled);
        CHECK(target.keyboard_count == 0);
    }

    SUBCASE("Re-enabling target allows events") {
        target.set_enabled(false);
        target.set_enabled(true);

        keyboard_event kbd{};
        kbd.key = key_code::a;

        ui_event evt = kbd;
        bool handled = target.handle_event(evt);

        CHECK(handled);
        CHECK(target.keyboard_count == 1);
    }
}

// ============================================================================
// Variant Dispatch Tests
// ============================================================================

TEST_CASE("event_target::handle_event - Variant dispatch") {
    test_event_target<test_canvas_backend> target;

    SUBCASE("Multiple event types in sequence") {
        // Keyboard event
        keyboard_event kbd{};
        kbd.key = key_code::a;
        target.handle_event(ui_event{kbd});

        // Mouse event
        mouse_event mouse{};
        mouse.x = 10;
        mouse.y = 20;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;
        target.handle_event(ui_event{mouse});

        // Resize event
        resize_event resize{};
        resize.width = 80;
        resize.height = 24;
        target.handle_event(ui_event{resize});

        // Verify all handlers were called
        CHECK(target.keyboard_count == 1);
        CHECK(target.mouse_count == 1);
        CHECK(target.resize_count == 1);
    }
}
