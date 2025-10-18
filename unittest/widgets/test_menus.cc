/**
 * @file test_menus.cc
 * @brief Tests for Phase 4 - Menu System
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Tests cover:
 * - menu_item widget (text, mnemonics, actions)
 * - menu widget (item management, navigation)
 * - menu_bar widget (horizontal layout, dropdown management)
 * - Integration with actions and mnemonics
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/widgets/menu_bar.hh>
#include <onyxui/widgets/action.hh>
#include "utils/test_backend.hh"

using namespace onyxui;

// ======================================================================
// Test Suite: menu_item
// ======================================================================

TEST_SUITE("menu_item") {
    using Backend = test_backend;

    TEST_CASE("Construct with text") {
        auto item = std::make_unique<menu_item<Backend>>("Save");

        CHECK(item->text() == "Save");
        CHECK(!item->is_separator());
    }

    TEST_CASE("Set and get text") {
        auto item = std::make_unique<menu_item<Backend>>();

        item->set_text("Open");
        CHECK(item->text() == "Open");

        item->set_text("Close");
        CHECK(item->text() == "Close");
    }

    TEST_CASE("Create separator") {
        auto separator = menu_item<Backend>::make_separator();

        CHECK(separator->is_separator());
        CHECK(!separator->is_focusable());
    }

    TEST_CASE("Set mnemonic text") {
        auto item = std::make_unique<menu_item<Backend>>();

        item->set_mnemonic_text("&Save");

        // Plain text should be extracted
        CHECK(item->text() == "Save");

        // Mnemonic character should be stored (but requires theme)
        // Without theme, mnemonic won't be parsed yet
    }

    TEST_CASE("Mnemonic character extraction") {
        auto item = std::make_unique<menu_item<Backend>>();

        // Without theme, no mnemonic is active
        item->set_mnemonic_text("&File");
        CHECK(!item->has_mnemonic());  // No theme yet
    }

    TEST_CASE("Get shortcut text from action") {
        auto item = std::make_unique<menu_item<Backend>>();

        auto act = std::make_shared<action<Backend>>();
        act->set_text("Save");
        act->set_shortcut('s', key_modifier::ctrl);

        item->set_action(act);

        // Item should show shortcut from action
        std::string shortcut = item->get_shortcut_text();
        CHECK(shortcut == "Ctrl+S");
    }

    TEST_CASE("Action integration syncs text") {
        auto item = std::make_unique<menu_item<Backend>>();

        auto act = std::make_shared<action<Backend>>();
        act->set_text("New File");

        item->set_action(act);

        // Text should sync from action
        CHECK(item->text() == "New File");
    }

    TEST_CASE("Item triggers action on click") {
        auto item = std::make_unique<menu_item<Backend>>();

        auto act = std::make_shared<action<Backend>>();
        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        item->set_action(act);

        // Simulate click
        item->clicked.emit();

        CHECK(triggered);
    }

    TEST_CASE("Separator is not focusable") {
        auto separator = menu_item<Backend>::make_separator();

        CHECK(!separator->is_focusable());
    }

    TEST_CASE("Normal item is focusable") {
        auto item = std::make_unique<menu_item<Backend>>("Item");

        CHECK(item->is_focusable());
    }
}

// ======================================================================
// Test Suite: menu
// ======================================================================

TEST_SUITE("menu") {
    using Backend = test_backend;

    TEST_CASE("Construct empty menu") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        CHECK(menu_widget->items().empty());
    }

    TEST_CASE("Add menu item") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        auto item = std::make_unique<menu_item<Backend>>("File");
        menu_widget->add_item(std::move(item));

        CHECK(menu_widget->items().size() == 1);
    }

    TEST_CASE("Add multiple items") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Open"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Save"));

        CHECK(menu_widget->items().size() == 3);
    }

    TEST_CASE("Add separator") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("New"));
        menu_widget->add_separator();
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Exit"));

        CHECK(menu_widget->items().size() == 3);
        CHECK(menu_widget->items()[1]->is_separator());
    }

    TEST_CASE("Find item by mnemonic") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        auto item1 = std::make_unique<menu_item<Backend>>();
        item1->set_mnemonic_text("&New");
        menu_widget->add_item(std::move(item1));

        auto item2 = std::make_unique<menu_item<Backend>>();
        item2->set_mnemonic_text("&Open");
        menu_widget->add_item(std::move(item2));

        // Without theme, mnemonics won't be active
        // This tests the infrastructure
        auto* found = menu_widget->find_by_mnemonic('n');
        // May be nullptr without theme
        (void)found;
    }

    TEST_CASE("Focus first item") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 1"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 2"));

        menu_widget->focus_first();

        // First item should be focused
        auto* focused = menu_widget->focused_item();
        CHECK(focused != nullptr);
        if (focused) {
            CHECK(focused->text() == "Item 1");
        }
    }

    TEST_CASE("Focus first skips separators") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_separator();
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 1"));

        menu_widget->focus_first();

        auto* focused = menu_widget->focused_item();
        CHECK(focused != nullptr);
        if (focused) {
            CHECK(focused->text() == "Item 1");
        }
    }

    TEST_CASE("Focus next navigates forward") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 1"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 2"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 3"));

        menu_widget->focus_first();
        menu_widget->focus_next();

        auto* focused = menu_widget->focused_item();
        CHECK(focused != nullptr);
        if (focused) {
            CHECK(focused->text() == "Item 2");
        }
    }

    TEST_CASE("Focus next wraps to start") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 1"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 2"));

        menu_widget->focus_first();
        menu_widget->focus_next();
        menu_widget->focus_next();  // Wrap

        auto* focused = menu_widget->focused_item();
        CHECK(focused != nullptr);
        if (focused) {
            CHECK(focused->text() == "Item 1");
        }
    }

    TEST_CASE("Focus previous navigates backward") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 1"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 2"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 3"));

        menu_widget->focus_first();
        menu_widget->focus_next();
        menu_widget->focus_next();
        menu_widget->focus_previous();

        auto* focused = menu_widget->focused_item();
        CHECK(focused != nullptr);
        if (focused) {
            CHECK(focused->text() == "Item 2");
        }
    }

    TEST_CASE("Focus previous wraps to end") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 1"));
        menu_widget->add_item(std::make_unique<menu_item<Backend>>("Item 2"));

        menu_widget->focus_first();
        menu_widget->focus_previous();  // Wrap

        auto* focused = menu_widget->focused_item();
        CHECK(focused != nullptr);
        if (focused) {
            CHECK(focused->text() == "Item 2");
        }
    }

    TEST_CASE("Activate focused item triggers action") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        auto item = std::make_unique<menu_item<Backend>>();
        auto act = std::make_shared<action<Backend>>();

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        item->set_action(act);
        menu_widget->add_item(std::move(item));

        menu_widget->focus_first();
        menu_widget->activate_focused();

        CHECK(triggered);
    }

    TEST_CASE("Item activation emits closing signal") {
        auto menu_widget = std::make_unique<menu<Backend>>();

        auto item = std::make_unique<menu_item<Backend>>("Item");
        auto* item_ptr = menu_widget->add_item(std::move(item));

        bool closed = false;
        menu_widget->closing.connect([&]() { closed = true; });

        // Simulate item click
        item_ptr->clicked.emit();

        CHECK(closed);
    }
}

// ======================================================================
// Test Suite: menu_bar
// ======================================================================

TEST_SUITE("menu_bar") {
    using Backend = test_backend;

    TEST_CASE("Construct empty menu bar") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        CHECK(bar->menu_count() == 0);
        CHECK(!bar->has_open_menu());
    }

    TEST_CASE("Add menu to bar") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        auto file_menu = std::make_unique<menu<Backend>>();
        bar->add_menu("File", std::move(file_menu));

        CHECK(bar->menu_count() == 1);
    }

    TEST_CASE("Add multiple menus") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->add_menu("Edit", std::make_unique<menu<Backend>>());
        bar->add_menu("View", std::make_unique<menu<Backend>>());

        CHECK(bar->menu_count() == 3);
    }

    TEST_CASE("Add menu with mnemonic") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("&File", std::make_unique<menu<Backend>>());

        // Mnemonic should be parsed and stored
        auto index = bar->find_by_mnemonic('f');
        CHECK(index.has_value());
        if (index) {
            CHECK(*index == 0);
        }
    }

    TEST_CASE("Find menu by mnemonic") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("&File", std::make_unique<menu<Backend>>());
        bar->add_menu("&Edit", std::make_unique<menu<Backend>>());
        bar->add_menu("&View", std::make_unique<menu<Backend>>());

        CHECK(bar->find_by_mnemonic('f') == 0);
        CHECK(bar->find_by_mnemonic('e') == 1);
        CHECK(bar->find_by_mnemonic('v') == 2);
        CHECK(!bar->find_by_mnemonic('x').has_value());
    }

    TEST_CASE("Open menu by index") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());

        bar->open_menu(0);

        CHECK(bar->has_open_menu());
        CHECK(bar->open_menu_index() == 0);
    }

    TEST_CASE("Close menu") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->open_menu(0);

        bar->close_menu();

        CHECK(!bar->has_open_menu());
    }

    // NOTE: These tests are disabled because menu_bar no longer uses signals.
    // menu_bar is now self-contained and manages popups internally.
    // The functionality is still tested by "Open menu by index" and "Close menu" tests.

#if 0
    TEST_CASE("Opening menu emits signal") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());

        bool opened = false;
        std::size_t opened_index = 999;
        bar->menu_opened.connect([&](std::size_t index, menu<Backend>* menu_ptr) {
            opened = true;
            opened_index = index;
        });

        bar->open_menu(0);

        CHECK(opened);
        CHECK(opened_index == 0);
    }

    TEST_CASE("Closing menu emits signal") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->open_menu(0);

        bool closed = false;
        bar->menu_closed.connect([&](std::size_t index) {
            closed = true;
        });

        bar->close_menu();

        CHECK(closed);
    }
#endif

    TEST_CASE("Navigate to next menu") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->add_menu("Edit", std::make_unique<menu<Backend>>());

        bar->open_menu(0);
        bar->navigate_next();

        CHECK(bar->open_menu_index() == 1);
    }

    TEST_CASE("Navigate next wraps to start") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->add_menu("Edit", std::make_unique<menu<Backend>>());

        bar->open_menu(1);
        bar->navigate_next();

        CHECK(bar->open_menu_index() == 0);
    }

    TEST_CASE("Navigate to previous menu") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->add_menu("Edit", std::make_unique<menu<Backend>>());

        bar->open_menu(1);
        bar->navigate_previous();

        CHECK(bar->open_menu_index() == 0);
    }

    TEST_CASE("Navigate previous wraps to end") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->add_menu("Edit", std::make_unique<menu<Backend>>());

        bar->open_menu(0);
        bar->navigate_previous();

        CHECK(bar->open_menu_index() == 1);
    }

    TEST_CASE("Get menu by index") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());
        bar->add_menu("Edit", std::make_unique<menu<Backend>>());

        auto* file_menu = bar->get_menu(0);
        auto* edit_menu = bar->get_menu(1);

        CHECK(file_menu != nullptr);
        CHECK(edit_menu != nullptr);
        CHECK(file_menu != edit_menu);
    }

    TEST_CASE("Get menu with invalid index returns nullptr") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        bar->add_menu("File", std::make_unique<menu<Backend>>());

        CHECK(bar->get_menu(999) == nullptr);
    }
}

// ======================================================================
// Test Suite: Menu Integration
// ======================================================================

TEST_SUITE("menu_integration") {
    using Backend = test_backend;

    TEST_CASE("Complete menu bar with items") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        // Create File menu
        auto file_menu = std::make_unique<menu<Backend>>();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("New"));
        file_menu->add_item(std::make_unique<menu_item<Backend>>("Open"));
        file_menu->add_separator();
        file_menu->add_item(std::make_unique<menu_item<Backend>>("Exit"));

        bar->add_menu("&File", std::move(file_menu));

        // Create Edit menu
        auto edit_menu = std::make_unique<menu<Backend>>();
        edit_menu->add_item(std::make_unique<menu_item<Backend>>("Cut"));
        edit_menu->add_item(std::make_unique<menu_item<Backend>>("Copy"));
        edit_menu->add_item(std::make_unique<menu_item<Backend>>("Paste"));

        bar->add_menu("&Edit", std::move(edit_menu));

        CHECK(bar->menu_count() == 2);

        auto* file = bar->get_menu(0);
        CHECK(file->items().size() == 4);

        auto* edit = bar->get_menu(1);
        CHECK(edit->items().size() == 3);
    }

    TEST_CASE("Menu items with actions and shortcuts") {
        auto save_action = std::make_shared<action<Backend>>();
        save_action->set_text("Save");
        save_action->set_shortcut('s', key_modifier::ctrl);

        bool save_triggered = false;
        save_action->triggered.connect([&]() { save_triggered = true; });

        auto item = std::make_unique<menu_item<Backend>>();
        item->set_action(save_action);

        CHECK(item->text() == "Save");
        CHECK(item->get_shortcut_text() == "Ctrl+S");

        item->clicked.emit();
        CHECK(save_triggered);
    }

    TEST_CASE("Menu navigation workflow") {
        auto bar = std::make_unique<menu_bar<Backend>>();

        // Build File menu
        auto file_menu = std::make_unique<menu<Backend>>();

        auto new_item = std::make_unique<menu_item<Backend>>();
        auto new_action = std::make_shared<action<Backend>>();
        new_action->set_text("New");
        bool new_triggered = false;
        new_action->triggered.connect([&]() { new_triggered = true; });
        new_item->set_action(new_action);

        file_menu->add_item(std::move(new_item));
        bar->add_menu("&File", std::move(file_menu));

        // Open File menu
        bar->open_menu(0);
        CHECK(bar->has_open_menu());

        // Navigate and activate
        auto* menu_ptr = bar->get_menu(0);
        menu_ptr->focus_first();
        menu_ptr->activate_focused();

        CHECK(new_triggered);
    }
}
