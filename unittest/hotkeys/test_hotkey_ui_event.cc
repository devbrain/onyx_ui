/**
 * @file test_hotkey_ui_event.cc
 * @brief Unit tests for hotkey_manager::handle_ui_event() (Phase 2)
 * @author igor
 * @date 28/10/2025
 *
 * @details
 * Tests the new handle_ui_event() API that accepts ui_event directly,
 * bypassing the old event_traits-based system.
 *
 * Coverage:
 * - Keyboard event handling
 * - Mouse/resize event rejection
 * - Hotkey matching and triggering
 * - Priority order (semantic → scoped → global)
 * - Modifier combinations
 */

#include <doctest/doctest.h>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/hotkeys/key_code.hh>
#include <onyxui/hotkeys/hotkey_scheme_registry.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// Helper Functions
// ============================================================================

namespace {
    /**
     * @brief Create keyboard ui_event from character and modifiers
     */
    ui_event make_key_event(char c, bool ctrl = false, bool alt = false, bool shift = false) {
        keyboard_event kbd{};
        kbd.key = static_cast<key_code>(c);
        kbd.modifiers = key_modifier::none;
        if (ctrl) kbd.modifiers |= key_modifier::ctrl;
        if (alt) kbd.modifiers |= key_modifier::alt;
        if (shift) kbd.modifiers |= key_modifier::shift;
        kbd.pressed = true;
        return ui_event{kbd};
    }

    /**
     * @brief Create function key ui_event
     */
    ui_event make_f_key_event(int f_key, bool ctrl = false, bool alt = false, bool shift = false) {
        keyboard_event kbd{};
        kbd.key = function_key_from_number(f_key);
        kbd.modifiers = key_modifier::none;
        if (ctrl) kbd.modifiers |= key_modifier::ctrl;
        if (alt) kbd.modifiers |= key_modifier::alt;
        if (shift) kbd.modifiers |= key_modifier::shift;
        kbd.pressed = true;
        return ui_event{kbd};
    }

    /**
     * @brief Create special key ui_event (arrow keys)
     */
    ui_event make_special_key_event(int special_key, bool ctrl = false, bool alt = false, bool shift = false) {
        keyboard_event kbd{};
        // Map old magic numbers to key_code
        if (special_key == -1) kbd.key = key_code::arrow_up;
        else if (special_key == -2) kbd.key = key_code::arrow_down;
        else if (special_key == -3) kbd.key = key_code::arrow_left;
        else if (special_key == -4) kbd.key = key_code::arrow_right;
        else kbd.key = static_cast<key_code>(special_key);
        kbd.modifiers = key_modifier::none;
        if (ctrl) kbd.modifiers |= key_modifier::ctrl;
        if (alt) kbd.modifiers |= key_modifier::alt;
        if (shift) kbd.modifiers |= key_modifier::shift;
        kbd.pressed = true;
        return ui_event{kbd};
    }
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Keyboard event handling") {
    hotkey_manager<test_canvas_backend> mgr;

    // Create action with Ctrl+S shortcut
    auto save_action = std::make_shared<action<test_canvas_backend>>();
    bool triggered = false;
    save_action->set_shortcut('s', key_modifier::ctrl);
    save_action->triggered.connect([&triggered]() {
        triggered = true;
    });

    // Register action globally
    mgr.register_action(save_action);

    SUBCASE("Plain character 's' - not handled") {
        auto evt = make_key_event('s', false, false, false);
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);
        CHECK_FALSE(triggered);
    }

    SUBCASE("Ctrl+S - handled") {
        auto evt = make_key_event('s', true, false, false);
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(triggered);
    }

    SUBCASE("Ctrl+Shift+S - not handled (different modifier)") {
        auto evt = make_key_event('s', true, false, true);
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);
        CHECK_FALSE(triggered);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Mouse/resize event rejection") {
    hotkey_manager<test_canvas_backend> mgr;

    SUBCASE("Mouse event - ignored") {
        mouse_event mouse{};
        mouse.x = 10.0_lu;
        mouse.y = 20.0_lu;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;

        ui_event evt = mouse;
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);  // Mouse events don't trigger hotkeys
    }

    SUBCASE("Resize event - ignored") {
        resize_event resize{};
        resize.width = 80;
        resize.height = 24;

        ui_event evt = resize;
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);  // Resize events don't trigger hotkeys
    }
}

// ============================================================================
// Function Key Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Function keys") {
    hotkey_manager<test_canvas_backend> mgr;

    // Register F10 hotkey
    auto menu_action = std::make_shared<action<test_canvas_backend>>();
    bool triggered = false;
    menu_action->set_shortcut_f(10);  // F10
    menu_action->triggered.connect([&triggered]() {
        triggered = true;
    });
    mgr.register_action(menu_action);

    SUBCASE("F10 - handled") {
        auto evt = make_f_key_event(10);
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(triggered);
    }

    SUBCASE("F9 - not handled") {
        auto evt = make_f_key_event(9);
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);
        CHECK_FALSE(triggered);
    }

    SUBCASE("Alt+F4") {
        auto close_action = std::make_shared<action<test_canvas_backend>>();
        bool close_triggered = false;
        close_action->set_shortcut_f(4, key_modifier::alt);
        close_action->triggered.connect([&close_triggered]() {
            close_triggered = true;
        });
        mgr.register_action(close_action);

        auto evt = make_f_key_event(4, false, true, false);
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(close_triggered);
    }
}

// ============================================================================
// Special Key Tests (Arrow keys, navigation)
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Arrow keys") {
    hotkey_manager<test_canvas_backend> mgr;

    // Register Down arrow semantic action
    hotkey_scheme_registry scheme_registry;

    // Create a simple scheme with Down arrow bound to menu_down
    hotkey_scheme scheme;
    scheme.name = "Test Scheme";
    scheme.set_binding(hotkey_action::menu_down, key_sequence{key_code::arrow_down});  // Arrow down

    scheme_registry.register_scheme(std::move(scheme));
    scheme_registry.set_current_scheme("Test Scheme");

    mgr.set_scheme_registry(&scheme_registry);

    bool menu_down_called = false;
    mgr.register_semantic_action(
        hotkey_action::menu_down,
        [&menu_down_called]() { menu_down_called = true; }
    );

    SUBCASE("Arrow Down - handled via semantic action") {
        auto evt = make_special_key_event(-2);  // Down arrow
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(menu_down_called);
    }

    SUBCASE("Arrow Up - not handled (not bound)") {
        auto evt = make_special_key_event(-1);  // Up arrow
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);
        CHECK_FALSE(menu_down_called);
    }
}

// ============================================================================
// Priority Order Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Priority order") {
    hotkey_manager<test_canvas_backend> mgr;

    // Setup scheme with Ctrl+S bound to semantic action
    hotkey_scheme_registry scheme_registry;
    hotkey_scheme scheme;
    scheme.name = "Test Scheme";
    scheme.set_binding(hotkey_action::menu_select, key_sequence{'s', key_modifier::ctrl});
    scheme_registry.register_scheme(std::move(scheme));
    scheme_registry.set_current_scheme("Test Scheme");
    mgr.set_scheme_registry(&scheme_registry);

    bool semantic_called = false;
    bool global_called = false;

    mgr.register_semantic_action(
        hotkey_action::menu_select,
        [&semantic_called]() { semantic_called = true; }
    );

    // Register global action for same key
    auto global_action = std::make_shared<action<test_canvas_backend>>();
    global_action->set_shortcut('s', key_modifier::ctrl);
    global_action->triggered.connect([&global_called]() {
        global_called = true;
    });
    mgr.register_action(global_action);

    SUBCASE("Semantic action has priority over global action") {
        auto evt = make_key_event('s', true, false, false);
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(semantic_called);
        CHECK_FALSE(global_called);  // Global action NOT triggered
    }
}

// ============================================================================
// Scoped Hotkeys Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Scoped hotkeys") {
    hotkey_manager<test_canvas_backend> mgr;

    auto root = std::make_unique<ui_element<test_canvas_backend>>(nullptr);
    auto child = std::make_unique<ui_element<test_canvas_backend>>(root.get());
    auto* child_ptr = child.get();
    root->add_child(std::move(child));

    // Register scoped action for child element
    auto scoped_action = std::make_shared<action<test_canvas_backend>>();
    bool scoped_triggered = false;
    scoped_action->set_shortcut('x', key_modifier::ctrl);
    scoped_action->triggered.connect([&scoped_triggered]() {
        scoped_triggered = true;
    });
    mgr.register_action(scoped_action, child_ptr);

    // Register global action for same key
    auto global_action = std::make_shared<action<test_canvas_backend>>();
    bool global_triggered = false;
    global_action->set_shortcut('x', key_modifier::ctrl);
    global_action->triggered.connect([&global_triggered]() {
        global_triggered = true;
    });
    mgr.register_action(global_action);

    SUBCASE("Scoped action triggers when focused") {
        auto evt = make_key_event('x', true, false, false);
        bool handled = mgr.handle_ui_event(evt, child_ptr);

        CHECK(handled);
        CHECK(scoped_triggered);
        CHECK_FALSE(global_triggered);  // Global not triggered
    }

    SUBCASE("Global action triggers when different element focused") {
        auto other = std::make_unique<ui_element<test_canvas_backend>>(nullptr);
        auto evt = make_key_event('x', true, false, false);
        bool handled = mgr.handle_ui_event(evt, other.get());

        CHECK(handled);
        CHECK_FALSE(scoped_triggered);  // Scoped not triggered
        CHECK(global_triggered);
    }

    SUBCASE("Global action triggers when no focus") {
        auto evt = make_key_event('x', true, false, false);
        bool handled = mgr.handle_ui_event(evt, nullptr);

        CHECK(handled);
        CHECK_FALSE(scoped_triggered);
        CHECK(global_triggered);
    }
}

// ============================================================================
// Control Character Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Control characters") {
    hotkey_manager<test_canvas_backend> mgr;

    SUBCASE("Enter key (newline)") {
        auto enter_action = std::make_shared<action<test_canvas_backend>>();
        bool triggered = false;
        enter_action->set_shortcut('\n');
        enter_action->triggered.connect([&triggered]() {
            triggered = true;
        });
        mgr.register_action(enter_action);

        auto evt = make_key_event('\n');
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(triggered);
    }

    SUBCASE("Tab key") {
        auto tab_action = std::make_shared<action<test_canvas_backend>>();
        bool triggered = false;
        tab_action->set_shortcut('\t');
        tab_action->triggered.connect([&triggered]() {
            triggered = true;
        });
        mgr.register_action(tab_action);

        auto evt = make_key_event('\t');
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(triggered);
    }

    SUBCASE("Escape key") {
        auto esc_action = std::make_shared<action<test_canvas_backend>>();
        bool triggered = false;
        esc_action->set_shortcut(27);  // ESC
        esc_action->triggered.connect([&triggered]() {
            triggered = true;
        });
        mgr.register_action(esc_action);

        auto evt = make_key_event(27);
        bool handled = mgr.handle_ui_event(evt);

        CHECK(handled);
        CHECK(triggered);
    }
}

// ============================================================================
// Unhandled Event Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "handle_ui_event - Unhandled events") {
    hotkey_manager<test_canvas_backend> mgr;

    SUBCASE("Unknown key - not handled") {
        auto evt = make_key_event('z');  // No hotkey registered for 'z'
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);
    }

    SUBCASE("Modifier mismatch - not handled") {
        auto save_action = std::make_shared<action<test_canvas_backend>>();
        save_action->set_shortcut('s', key_modifier::ctrl);
        mgr.register_action(save_action);

        auto evt = make_key_event('s', false, true, false);  // Alt+S instead of Ctrl+S
        bool handled = mgr.handle_ui_event(evt);

        CHECK_FALSE(handled);
    }
}
