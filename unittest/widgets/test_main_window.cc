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

    // No children until the first access to the central widget, menu, or
    // status — the central widget is materialized lazily on first request.
    CHECK(main->children().empty());
    CHECK(main->get_menu_bar() == nullptr);
    CHECK(main->get_status_bar() == nullptr);

    // Accessing central_widget() materializes it lazily.
    CHECK(main->central_widget() != nullptr);
    CHECK(main->children().size() == 1);
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

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - central widget is the workspace for overlay windows") {
    auto main = std::make_unique<main_window<Backend>>();

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;

    auto win = std::make_unique<window<Backend>>("Test Window", flags);
    win->set_workspace(main->central_widget());

    // Central widget is created lazily the first time it's requested.
    REQUIRE(main->central_widget() != nullptr);

    // The window is an overlay — it is NOT a tree child of central widget.
    // Instead, its workspace (maximize bounds reference) points at central.
    CHECK(win->parent() == nullptr);
    CHECK(win->get_workspace() == main->central_widget());
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
    [[maybe_unused]] auto measured = main->measure(80_lu, 25_lu);
    main->arrange(logical_rect{0_lu, 0_lu, 80_lu, 25_lu});

    // Create a floating overlay window and pair it to the central widget.
    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;
    auto win = std::make_unique<window<Backend>>("Test", flags);
    win->set_workspace(main->central_widget());

    // Set window size
    win->set_size(20, 10);

    // Maximize window
    win->maximize();

    // Window is an overlay — no tree parent. Its workspace points at
    // central_widget, which is what maximize() fills into.
    CHECK(win->parent() == nullptr);
    CHECK(win->get_workspace() == main->central_widget());

    // Window state should be maximized
    CHECK(win->get_state() == window<Backend>::window_state::maximized);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "main_window - multiple windows share same central widget workspace") {
    auto main = std::make_unique<main_window<Backend>>();

    auto win1 = std::make_unique<window<Backend>>(
        "Window 1", typename window<Backend>::window_flags{});
    win1->set_workspace(main->central_widget());
    auto win2 = std::make_unique<window<Backend>>(
        "Window 2", typename window<Backend>::window_flags{});
    win2->set_workspace(main->central_widget());

    // Both windows are overlays (no tree parent); they share the same
    // workspace, which is the central widget.
    CHECK(win1->parent() == nullptr);
    CHECK(win2->parent() == nullptr);
    CHECK(win1->get_workspace() == win2->get_workspace());
    CHECK(win1->get_workspace() == main->central_widget());
}
