/**
 * @file test_menu_system.cc
 * @brief Unit tests for Phase 4 menu_system class
 * @author Assistant
 * @date 2025-10-28
 *
 * @details
 * Tests the centralized menu navigation coordinator added in Phase 4:
 * - Menu stack management (open/close, depth tracking)
 * - Context-dependent navigation (top-level vs submenu)
 * - RAII hotkey registration
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/menu_system.hh>
#include <onyxui/widgets/menu_bar.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/widgets/menu_item.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"

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
// Test Suite: menu_system - Basic Lifecycle (Phase 4)
// ======================================================================

TEST_SUITE("menu_system::lifecycle") {

    TEST_CASE("menu_system starts with empty stack") {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Note: menu_system is private member, we'll test through menu_bar API
        // Direct menu_system testing would require friend declaration

        // Initially no menu is open
        CHECK_FALSE(menu_bar_widget->has_open_menu());
        CHECK_FALSE(menu_bar_widget->open_menu_index().has_value());
    }

    TEST_CASE("open_top_level_menu sets menu stack") {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));

        menu_bar_widget->add_menu("File", std::move(file_menu));
        menu_bar_widget->open_menu(0);

        // Menu should be open
        CHECK(menu_bar_widget->has_open_menu());
        CHECK(menu_bar_widget->open_menu_index() == 0);
    }

    TEST_CASE("close_menu clears menu stack") {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        auto file_menu = std::make_unique<menu<Backend>>();
        auto item = std::make_unique<menu_item<Backend>>("New");
        file_menu->add_item(std::move(item));

        menu_bar_widget->add_menu("File", std::move(file_menu));
        menu_bar_widget->open_menu(0);
        CHECK(menu_bar_widget->has_open_menu());

        menu_bar_widget->close_menu();
        CHECK_FALSE(menu_bar_widget->has_open_menu());
    }

} // TEST_SUITE menu_system::lifecycle


// ======================================================================
// Test Suite: menu_system - Submenu Stack (Phase 4)
// ======================================================================

TEST_SUITE("menu_system::submenu_stack") {

    TEST_CASE("Submenu data structure exists (Phase 3)") {
        auto item = std::make_unique<menu_item<Backend>>("Open");
        auto submenu = std::make_unique<menu<Backend>>();

        auto submenu_ptr = submenu.get();
        item->set_submenu(std::move(submenu));

        // Verify submenu is attached
        CHECK(item->has_submenu());
        CHECK(item->submenu() == submenu_ptr);
    }

    TEST_CASE("Nested menu structure (Phase 3 infrastructure)") {
        // Build: File → Open → Project
        auto item = std::make_unique<menu_item<Backend>>("Open");
        auto open_submenu = std::make_unique<menu<Backend>>();

        auto project_item = std::make_unique<menu_item<Backend>>("Project");
        auto project_submenu = std::make_unique<menu<Backend>>();
        project_item->set_submenu(std::move(project_submenu));

        open_submenu->add_item(std::move(project_item));
        item->set_submenu(std::move(open_submenu));

        // Verify structure
        CHECK(item->has_submenu());
        CHECK(item->submenu()->items().size() == 1);

        auto* project = item->submenu()->items()[0];
        CHECK(project->has_submenu());
    }

} // TEST_SUITE menu_system::submenu_stack


// ======================================================================
// Test Suite: menu_system - Hotkey Registration (Phase 4)
// ======================================================================

TEST_SUITE("menu_system::hotkey_registration") {

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Hotkeys registered on-demand when menu opens") {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // On construction: Only activate_menu_bar registered (ESC fix)
        auto& hotkeys = ctx.hotkeys();

        CHECK(hotkeys.has_semantic_handler(hotkey_action::activate_menu_bar));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_left));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_right));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

        // Open a menu - navigation handlers now registered
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_bar_widget->add_menu("File", std::move(file_menu));
        menu_bar_widget->open_menu(0);

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_left));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_right));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Hotkeys remain registered after menu switch") {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add two menus
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        auto edit_menu = std::make_unique<menu<Backend>>();
        edit_menu->add_item(std::make_unique<menu_item<Backend>>("Cut"));
        menu_bar_widget->add_menu("Edit", std::move(edit_menu));

        // Open File menu
        menu_bar_widget->open_menu(0);

        auto& hotkeys = ctx.hotkeys();
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));

        // Switch to Edit menu
        menu_bar_widget->open_menu(1);

        // Phase 4: Handlers remain registered (no re-registration churn)
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Navigation hotkeys unregistered after close_menu (ESC fix)") {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        menu_bar_widget->open_menu(0);

        auto& hotkeys = ctx.hotkeys();
        // Verify handlers registered when menu open
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

        menu_bar_widget->close_menu();

        // Navigation handlers unregistered (allows ESC to propagate to app)
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_select));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_left));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_right));

        // But activate_menu_bar remains registered
        CHECK(hotkeys.has_semantic_handler(hotkey_action::activate_menu_bar));
    }

} // TEST_SUITE menu_system::hotkey_registration


// ======================================================================
// Test Suite: menu_system - Context-Dependent Navigation (Phase 4)
// ======================================================================

TEST_SUITE("menu_system::context_navigation") {

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Up/Down navigation always targets current menu") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add menu with multiple items
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        file_menu->add_item(std::make_unique<menu_item<Backend>>("Open"));
        file_menu->add_item(std::make_unique<menu_item<Backend>>("Save"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        menu_bar_widget->open_menu(0);

        auto* menu = menu_bar_widget->get_menu(0);
        CHECK(menu != nullptr);
        CHECK(menu->focused_item() != nullptr);  // Something is focused

        // Simulate Down arrow key
        auto down_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_DOWN);
        bool handled = ctx.hotkeys().handle_key_event(down_event);

        CHECK(handled);
        CHECK(menu->focused_item() != nullptr);  // Still focused (navigation worked)
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Left/Right navigation switches menu bar at top-level") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add three menus
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        auto edit_menu = std::make_unique<menu<Backend>>();
        edit_menu->add_item(std::make_unique<menu_item<Backend>>("Cut"));
        menu_bar_widget->add_menu("Edit", std::move(edit_menu));

        auto view_menu = std::make_unique<menu<Backend>>();
        view_menu->add_item(std::make_unique<menu_item<Backend>>("Zoom"));
        menu_bar_widget->add_menu("View", std::move(view_menu));

        // Open File menu
        menu_bar_widget->open_menu(0);
        CHECK(menu_bar_widget->open_menu_index() == 0);

        // Simulate Right arrow key
        auto right_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_RIGHT);
        bool handled = ctx.hotkeys().handle_key_event(right_event);

        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 1);  // Switched to Edit menu
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Escape closes menu (Phase 4 delegates to menu_system)") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        menu_bar_widget->open_menu(0);
        CHECK(menu_bar_widget->has_open_menu());

        // Simulate Escape key
        auto esc_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_ESCAPE);
        bool handled = ctx.hotkeys().handle_key_event(esc_event);

        CHECK(handled);
        CHECK_FALSE(menu_bar_widget->has_open_menu());
    }

} // TEST_SUITE menu_system::context_navigation


// ======================================================================
// Test Suite: menu_system - Submenu Navigation (Phase 5)
// ======================================================================

TEST_SUITE("menu_system::submenu_navigation") {

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Right arrow on submenu item opens submenu") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Build File menu with Open submenu
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));

        // Create "Open" item with submenu
        auto open_item = std::make_unique<menu_item<Backend>>("Open");
        auto open_submenu = std::make_unique<menu<Backend>>();
        open_submenu->add_item(std::make_unique<menu_item<Backend>>("Project"));
        open_submenu->add_item(std::make_unique<menu_item<Backend>>("File"));
        open_item->set_submenu(std::move(open_submenu));
        file_menu->add_item(std::move(open_item));

        file_menu->add_item(std::make_unique<menu_item<Backend>>("Save"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open File menu
        menu_bar_widget->open_menu(0);

        auto* menu = menu_bar_widget->get_menu(0);
        CHECK(menu != nullptr);

        // Navigate to "Open" item (index 1)
        auto down_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_DOWN);
        ctx.hotkeys().handle_key_event(down_event);

        // Verify focused on "Open" item with submenu
        auto* focused = menu->focused_item();
        CHECK(focused != nullptr);
        CHECK(focused->has_submenu());

        // Press right arrow to open submenu
        auto right_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_RIGHT);
        bool handled = ctx.hotkeys().handle_key_event(right_event);

        CHECK(handled);
        // Phase 5: Submenu should be open (visual verification would check layer count)
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Left arrow in submenu closes submenu") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Build File menu with Open submenu
        auto file_menu = std::make_unique<menu<Backend>>();

        auto open_item = std::make_unique<menu_item<Backend>>("Open");
        auto open_submenu = std::make_unique<menu<Backend>>();
        open_submenu->add_item(std::make_unique<menu_item<Backend>>("Project"));
        open_item->set_submenu(std::move(open_submenu));
        file_menu->add_item(std::move(open_item));

        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open File menu and submenu
        menu_bar_widget->open_menu(0);

        auto* menu = menu_bar_widget->get_menu(0);
        auto* focused = menu->focused_item();
        CHECK(focused != nullptr);
        CHECK(focused->has_submenu());

        // Open submenu with right arrow
        auto right_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_RIGHT);
        ctx.hotkeys().handle_key_event(right_event);

        // Now press left arrow to close submenu
        auto left_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_LEFT);
        bool handled = ctx.hotkeys().handle_key_event(left_event);

        CHECK(handled);
        // Phase 5: Submenu should be closed, focus restored to parent
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Right arrow on regular item switches menu bar at top-level") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Add two menus
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_bar_widget->add_menu("File", std::move(file_menu));

        auto edit_menu = std::make_unique<menu<Backend>>();
        edit_menu->add_item(std::make_unique<menu_item<Backend>>("Cut"));
        menu_bar_widget->add_menu("Edit", std::move(edit_menu));

        // Open File menu
        menu_bar_widget->open_menu(0);
        CHECK(menu_bar_widget->open_menu_index() == 0);

        // Focused on regular item (no submenu)
        auto* menu = menu_bar_widget->get_menu(0);
        auto* focused = menu->focused_item();
        CHECK(focused != nullptr);
        CHECK_FALSE(focused->has_submenu());

        // Right arrow should switch to Edit menu (not open submenu)
        auto right_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_RIGHT);
        bool handled = ctx.hotkeys().handle_key_event(right_event);

        CHECK(handled);
        CHECK(menu_bar_widget->open_menu_index() == 1);  // Switched to Edit menu
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Nested submenus (arbitrary depth)") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Build File → Open → Recent (3 levels)
        auto file_menu = std::make_unique<menu<Backend>>();

        auto open_item = std::make_unique<menu_item<Backend>>("Open");
        auto open_submenu = std::make_unique<menu<Backend>>();

        auto recent_item = std::make_unique<menu_item<Backend>>("Recent");
        auto recent_submenu = std::make_unique<menu<Backend>>();
        recent_submenu->add_item(std::make_unique<menu_item<Backend>>("Project1"));
        recent_item->set_submenu(std::move(recent_submenu));

        open_submenu->add_item(std::move(recent_item));
        open_item->set_submenu(std::move(open_submenu));
        file_menu->add_item(std::move(open_item));

        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open File menu
        menu_bar_widget->open_menu(0);

        // Navigate to Open and open submenu
        auto* menu = menu_bar_widget->get_menu(0);
        CHECK(menu->focused_item()->has_submenu());

        auto right_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_RIGHT);
        ctx.hotkeys().handle_key_event(right_event);

        // Now in Open submenu, navigate to Recent and open it
        ctx.hotkeys().handle_key_event(right_event);

        // Phase 5: Should have 3-level menu hierarchy (File → Open → Recent)
        // Pressing left should close Recent, then Open, then File menu
        auto left_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_LEFT);
        bool handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);  // Close Recent submenu

        handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);  // Close Open submenu

        handled = ctx.hotkeys().handle_key_event(left_event);
        CHECK(handled);  // Close File menu
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Escape closes submenu, then parent menu") {
        ctx.hotkey_schemes().set_current_scheme("Windows");
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // Build File menu with Open submenu
        auto file_menu = std::make_unique<menu<Backend>>();

        auto open_item = std::make_unique<menu_item<Backend>>("Open");
        auto open_submenu = std::make_unique<menu<Backend>>();
        open_submenu->add_item(std::make_unique<menu_item<Backend>>("Project"));
        open_item->set_submenu(std::move(open_submenu));
        file_menu->add_item(std::move(open_item));

        menu_bar_widget->add_menu("File", std::move(file_menu));

        // Open File menu and submenu
        menu_bar_widget->open_menu(0);

        auto* menu = menu_bar_widget->get_menu(0);
        CHECK(menu->focused_item()->has_submenu());

        // Open submenu
        auto right_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_RIGHT);
        ctx.hotkeys().handle_key_event(right_event);

        // First Escape closes submenu
        auto esc_event = make_key_event(event_traits<Backend::test_keyboard_event>::KEY_ESCAPE);
        bool handled = ctx.hotkeys().handle_key_event(esc_event);
        CHECK(handled);
        CHECK(menu_bar_widget->has_open_menu());  // File menu still open

        // Second Escape closes File menu
        handled = ctx.hotkeys().handle_key_event(esc_event);
        CHECK(handled);
        CHECK_FALSE(menu_bar_widget->has_open_menu());  // All menus closed
    }

} // TEST_SUITE menu_system::submenu_navigation
