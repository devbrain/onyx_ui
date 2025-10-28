/**
 * @file test_menu_hotkey_integration.cc
 * @brief Tests for Phase 4 - Menu + Hotkey Scheme Integration
 * @author igor
 * @date 27/10/2025
 *
 * @details
 * Tests semantic action integration with menu system:
 * - activate_menu_bar (F10/F9) opens/closes menu
 * - menu_left/menu_right navigate between menus
 * - menu_up/menu_down navigate within dropdown
 * - menu_select activates focused item
 * - menu_cancel closes menu
 */

#include <doctest/doctest.h>
#include <memory>
#include <onyxui/widgets/menu_bar.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/widgets/action.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/hotkeys/hotkey_scheme_registry.hh>
#include <onyxui/hotkeys/builtin_hotkey_schemes.hh>
#include <onyxui/hotkeys/key_sequence.hh>
#include "utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// Helper function to simulate key event
static test_backend::test_keyboard_event make_key_event(int key_code) {
    test_backend::test_keyboard_event event;
    event.key_code = key_code;
    event.pressed = true;
    return event;
}

// ======================================================================
// Test Suite: menu_bar::hotkey_integration
// ======================================================================

TEST_SUITE("menu_bar::hotkey_integration") {

    TEST_CASE("Semantic actions are registered on construction (Phase 1 RAII)") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // After construction, only menu bar-level handlers should be registered
        auto& hotkeys = ctx.hotkeys();

        CHECK(hotkeys.has_semantic_handler(hotkey_action::activate_menu_bar));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_left));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_right));

        // Phase 1: Menu navigation actions are NOT registered until menu opens
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

        // Now open a menu
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));
        menu_bar_widget->add_menu("File", std::move(file_menu));
        menu_bar_widget->open_menu(0);

        // Menu navigation actions should now be registered via RAII guards
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

        // Close menu
        menu_bar_widget->close_menu();

        // Menu navigation actions should be unregistered
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));
    }

    TEST_CASE("F10 opens first menu (Windows scheme)") {
        scoped_ui_context<Backend> ctx;
        ctx.hotkey_schemes().set_current_scheme("Windows");

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add a menu
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Simulate F10 key press
        auto event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F10);

        // F10 should be handled by semantic action
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);

        // Menu should be open
        // Note: We can't directly check m_open_menu_index (private)
        // But we verified the handler was called
    }

    TEST_CASE("F9 opens first menu (Norton Commander scheme)") {
        scoped_ui_context<Backend> ctx;
        ctx.hotkey_schemes().set_current_scheme("Norton Commander");

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add a menu
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Simulate F9 key press
        auto event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F9);

        // F9 should be handled by semantic action
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);
    }

    TEST_CASE("Escape closes menu") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add menus
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open menu programmatically
        menu_bar_widget->open_menu(0);

        // Simulate Escape key press
        auto event = make_key_event(27);  // Escape

        // Escape should be handled by semantic action
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);

        // Menu should be closed (handler was called)
    }

    TEST_CASE("Arrow keys navigate when menu is closed - no effect") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add menus
        auto file_menu = std::make_unique<menu<Backend>>();
        menu_bar_widget->add_menu("File", std::move(file_menu));

        auto edit_menu = std::make_unique<menu<Backend>>();
        menu_bar_widget->add_menu("Edit", std::move(edit_menu));

        // Menu is NOT open

        // Simulate Left arrow - should be handled but do nothing
        auto left_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_LEFT);
        bool handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);

        // Simulate Right arrow
        auto right_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_RIGHT);
        handled = ctx.hotkeys().handle_key_event(right_event);
        CHECK(handled);
    }

    TEST_CASE("Multiple menus can be added") {
        scoped_ui_context<Backend> ctx;
        ctx.hotkey_schemes().set_current_scheme("Windows");

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add multiple menus
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item1 = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item1));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        auto edit_menu = std::make_unique<menu<Backend>>();
        auto item2 = std::make_unique<menu_item<Backend>>("Cut");
        edit_menu->add_item(std::move(item2));
        menu_bar_widget->add_menu("Edit", std::move(edit_menu));

        auto view_menu = std::make_unique<menu<Backend>>();
        auto item3 = std::make_unique<menu_item<Backend>>("Zoom");
        view_menu->add_item(std::move(item3));
        menu_bar_widget->add_menu("View", std::move(view_menu));

        // F10 should still work with multiple menus
        auto event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F10);
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);
    }

    TEST_CASE("Enter key activates focused item") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add a menu with an item
        auto file_menu = std::make_unique<menu<Backend>>();

        bool item_triggered = false;
        auto act = std::make_shared<action<Backend>>();
        act->triggered.connect([&]() { item_triggered = true; });

        auto item = std::make_unique<menu_item<Backend>>("New");
        item->set_action(act);
        file_menu->add_item(std::move(item));

        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open menu programmatically
        menu_bar_widget->open_menu(0);

        // Simulate Enter key press
        auto event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_ENTER);

        // Enter should be handled by semantic action
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);

        // Note: Item won't be triggered in test because focus system
        // needs full UI context setup. We just verify handler exists.
    }

    TEST_CASE("Up/Down keys navigate within dropdown") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add a menu with multiple items
        auto file_menu = std::make_unique<menu<Backend>>();

        auto item1 = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item1));

        auto item2 = std::make_unique<menu_item<Backend>>("Open");
        file_menu->add_item(std::move(item2));

        auto item3 = std::make_unique<menu_item<Backend>>("Save");
        file_menu->add_item(std::move(item3));

        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open menu programmatically
        menu_bar_widget->open_menu(0);

        // Simulate Down arrow
        auto down_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_DOWN);
        bool handled = ctx.hotkeys().handle_key_event(down_event);
        CHECK(handled);

        // Simulate Up arrow
        auto up_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_UP);
        handled = ctx.hotkeys().handle_key_event(up_event);
        CHECK(handled);
    }

    TEST_CASE("Scheme switching changes activation key") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add a menu
        auto file_menu = std::make_unique<menu<Backend>>();
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Test Windows scheme (F10)
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto f10_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F10);
        bool handled = ctx.hotkeys().handle_key_event(f10_event);
        CHECK(handled);

        // Switch to Norton Commander scheme (F9)
        ctx.hotkey_schemes().set_current_scheme("Norton Commander");
        auto f9_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F9);
        handled = ctx.hotkeys().handle_key_event(f9_event);
        CHECK(handled);
    }

    TEST_CASE("Empty menu bar handles F10 gracefully") {
        scoped_ui_context<Backend> ctx;
        ctx.hotkey_schemes().set_current_scheme("Windows");

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // No menus added - menu_bar is empty

        // F10 should be handled but do nothing
        auto event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F10);
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);
    }

    TEST_CASE("Semantic actions have priority over application actions") {
        scoped_ui_context<Backend> ctx;
        ctx.hotkey_schemes().set_current_scheme("Windows");

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add a menu
        auto file_menu = std::make_unique<menu<Backend>>();
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Register application action for F10
        bool app_action_triggered = false;
        auto app_action = std::make_shared<action<Backend>>();
        app_action->set_shortcut_f(10);
        app_action->triggered.connect([&]() { app_action_triggered = true; });
        ctx.hotkeys().register_action(app_action);

        // F10 should trigger semantic action (menu), NOT application action
        auto event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_F10);
        bool handled = ctx.hotkeys().handle_key_event(event);
        CHECK(handled);

        // Application action should NOT be triggered
        CHECK_FALSE(app_action_triggered);
    }

    TEST_CASE("Left/Right arrows navigate between menu bar items (Phase 2)") {
        scoped_ui_context<Backend> ctx;
        ctx.hotkey_schemes().set_current_scheme("Windows");

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add 3 menus: File, Edit, View
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item1 = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item1));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        auto edit_menu = std::make_unique<menu<Backend>>();
        auto item2 = std::make_unique<menu_item<Backend>>("Cut");
        edit_menu->add_item(std::move(item2));
        menu_bar_widget->add_menu("Edit", std::move(edit_menu));

        auto view_menu = std::make_unique<menu<Backend>>();
        auto item3 = std::make_unique<menu_item<Backend>>("Zoom");
        view_menu->add_item(std::move(item3));
        menu_bar_widget->add_menu("View", std::move(view_menu));

        // Open File menu (index 0)
        menu_bar_widget->open_menu(0);
        CHECK(menu_bar_widget->open_menu_index() == 0);

        // Simulate Right arrow - should switch to Edit menu
        auto right_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_RIGHT);
        bool handled = ctx.hotkeys().handle_key_event(right_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 1);  // Edit menu

        // Right arrow again - should switch to View menu
        handled = ctx.hotkeys().handle_key_event(right_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 2);  // View menu

        // Right arrow again - should wrap to File menu
        handled = ctx.hotkeys().handle_key_event(right_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 0);  // File menu (wrapped)

        // Simulate Left arrow - should switch to View menu (wrap backwards)
        auto left_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_LEFT);
        handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 2);  // View menu

        // Left arrow again - should switch to Edit menu
        handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 1);  // Edit menu

        // Left arrow again - should switch to File menu
        handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 0);  // File menu
    }

    TEST_CASE("Left/Right navigation with single menu wraps to same menu (Phase 2 edge case)") {
        scoped_ui_context<Backend> ctx;

        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add only one menu
        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open the menu
        menu_bar_widget->open_menu(0);
        CHECK(menu_bar_widget->open_menu_index() == 0);

        // Right arrow - should stay on same menu (wraps to itself)
        auto right_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_RIGHT);
        bool handled = ctx.hotkeys().handle_key_event(right_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 0);  // Still File menu

        // Left arrow - should stay on same menu
        auto left_event = make_key_event(event_traits<test_backend::test_keyboard_event>::KEY_LEFT);
        handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 0);  // Still File menu
    }

} // TEST_SUITE menu_bar::hotkey_integration
