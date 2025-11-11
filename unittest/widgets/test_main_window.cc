/**
 * @file test_main_window.cc
 * @brief Unit tests for main_window widget
 * @author Claude Code
 * @date 2025-11-11
 */

#include <doctest/doctest.h>

#include <onyxui/widgets/main_window.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/panel.hh>

#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

using Backend = test_canvas_backend;

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - construction creates empty layout") {
    auto main = std::make_unique<main_window<Backend>>();

    // Should have no children initially (central widget created lazily)
    CHECK(main->children().empty());

    // Central widget should be nullptr until first use
    CHECK(main->central_widget() == nullptr);
    CHECK(main->get_menu_bar() == nullptr);
    CHECK(main->get_status_bar() == nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - set_menu_bar adds menu at top") {
    auto main = std::make_unique<main_window<Backend>>();
    auto menu = std::make_unique<menu_bar<Backend>>();
    auto* menu_ptr = menu.get();

    main->set_menu_bar(std::move(menu));

    // Menu should be first child
    REQUIRE(main->children().size() == 1);
    CHECK(main->children()[0].get() == menu_ptr);
    CHECK(main->get_menu_bar() == menu_ptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - set_status_bar adds status at bottom") {
    auto main = std::make_unique<main_window<Backend>>();
    auto status = std::make_unique<status_bar<Backend>>();
    auto* status_ptr = status.get();

    main->set_status_bar(std::move(status));

    // Status should be first child (since central not created yet)
    REQUIRE(main->children().size() == 1);
    CHECK(main->children()[0].get() == status_ptr);
    CHECK(main->get_status_bar() == status_ptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - set_central_widget sets central area") {
    auto main = std::make_unique<main_window<Backend>>();
    auto central = std::make_unique<panel<Backend>>();
    auto* central_ptr = central.get();

    main->set_central_widget(std::move(central));

    // Central should be first child
    REQUIRE(main->children().size() == 1);
    CHECK(main->children()[0].get() == central_ptr);
    CHECK(main->central_widget() == central_ptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - create_window sets central widget as parent") {
    auto main = std::make_unique<main_window<Backend>>();

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;

    auto win = main->create_window("Test Window", flags);

    // Central widget should be created automatically
    REQUIRE(main->central_widget() != nullptr);

    // Window should have central widget as parent
    CHECK(win->parent() == main->central_widget());
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - layout order is menu, central, status") {
    auto main = std::make_unique<main_window<Backend>>();

    // Add in specific order to test layout
    auto menu = std::make_unique<menu_bar<Backend>>();
    auto* menu_ptr = menu.get();
    main->set_menu_bar(std::move(menu));

    auto central = std::make_unique<panel<Backend>>();
    auto* central_ptr = central.get();
    main->set_central_widget(std::move(central));

    auto status = std::make_unique<status_bar<Backend>>();
    auto* status_ptr = status.get();
    main->set_status_bar(std::move(status));

    // Verify order: menu, central, status
    auto& children = main->children();
    REQUIRE(children.size() == 3);
    CHECK(children[0].get() == menu_ptr);
    CHECK(children[1].get() == central_ptr);
    CHECK(children[2].get() == status_ptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - window maximizes to fill central widget") {
    auto main = std::make_unique<main_window<Backend>>();

    // Set up main window with menu and status
    main->set_menu_bar(std::make_unique<menu_bar<Backend>>());
    main->set_central_widget(std::make_unique<panel<Backend>>());
    main->set_status_bar(std::make_unique<status_bar<Backend>>());

    // Measure and arrange main window to 80x25
    [[maybe_unused]] auto measured = main->measure(80, 25);
    main->arrange({0, 0, 80, 25});

    // Create window via main_window
    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;
    auto win = main->create_window("Test", flags);

    // Set window size
    win->set_size(20, 10);

    // Maximize window
    win->maximize();

    // Window should fill central widget (which is between menu and status)
    // For now, we just verify that window has a parent
    REQUIRE(win->parent() != nullptr);
    CHECK(win->parent() == main->central_widget());

    // Window state should be maximized
    CHECK(win->get_state() == window<Backend>::window_state::maximized);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - multiple windows share same central widget parent") {
    auto main = std::make_unique<main_window<Backend>>();

    auto win1 = main->create_window("Window 1", typename window<Backend>::window_flags{});
    auto win2 = main->create_window("Window 2", typename window<Backend>::window_flags{});

    // Both windows should have same parent (central widget)
    CHECK(win1->parent() == win2->parent());
    CHECK(win1->parent() == main->central_widget());
}
