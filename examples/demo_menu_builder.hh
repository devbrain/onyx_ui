//
// Created by igor on 21/10/2025.
//
// Menu bar builder for main_widget demo
// Constructs File, Theme, and Help menus with cascading submenus
//

#pragma once
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu_item.hh>

namespace demo_menu_builder {

/**
 * @brief Build and attach menu bar to widget
 *
 * @details
 * Creates a complete menu bar with:
 * - File menu: New, Open (with submenu), Quit
 * - Theme menu: Dynamically built from available themes
 * - Help menu: About
 *
 * @tparam Backend UI backend type
 * @tparam Widget The main widget type (must have add_child method)
 * @param widget Pointer to the main widget instance
 * @param theme_names List of available theme names
 * @param theme_actions List of theme switching actions
 * @param new_action Action for New menu item
 * @param open_action Action for Open menu item
 * @param quit_action Action for Quit menu item
 * @param about_action Action for About menu item
 * @return Pointer to the created menu bar
 */
template<typename Backend, typename Widget>
onyxui::menu_bar<Backend>* build_menu_bar(
    Widget* widget,
    const std::vector<std::string>& theme_names,
    const std::vector<std::shared_ptr<onyxui::action<Backend>>>& theme_actions,
    std::shared_ptr<onyxui::action<Backend>> new_action,
    std::shared_ptr<onyxui::action<Backend>> open_action,
    std::shared_ptr<onyxui::action<Backend>> quit_action,
    std::shared_ptr<onyxui::action<Backend>> about_action) {

    // Create menu bar
    auto menu_bar_ptr = std::make_unique<onyxui::menu_bar<Backend>>(widget);

    // File menu
    auto file_menu = std::make_unique<onyxui::menu<Backend>>();

    auto new_item = std::make_unique<onyxui::menu_item<Backend>>("New");
    new_item->set_action(new_action);
    file_menu->add_item(std::move(new_item));

    // Phase 5: Open submenu - demonstrates cascading menu navigation!
    auto open_item = std::make_unique<onyxui::menu_item<Backend>>("Open");

    // Create Open submenu with multiple options
    auto open_submenu = std::make_unique<onyxui::menu<Backend>>();

    // Open File option (using existing open_action)
    auto open_file_item = std::make_unique<onyxui::menu_item<Backend>>("");
    open_file_item->set_mnemonic_text("Open &File...");
    open_file_item->set_action(open_action);  // Ctrl+O
    open_submenu->add_item(std::move(open_file_item));

    // Open Folder option
    auto open_folder_item = std::make_unique<onyxui::menu_item<Backend>>("");
    open_folder_item->set_mnemonic_text("Open F&older...");
    open_folder_item->clicked.connect([]() {
        // Open Folder clicked
    });
    open_submenu->add_item(std::move(open_folder_item));

    open_submenu->add_separator();

    // Recent Files submenu (nested submenu - demonstrates arbitrary depth!)
    auto recent_item = std::make_unique<onyxui::menu_item<Backend>>("");
    recent_item->set_mnemonic_text("&Recent Files");

    auto recent_submenu = std::make_unique<onyxui::menu<Backend>>();
    auto recent1 = std::make_unique<onyxui::menu_item<Backend>>("demo.cc");
    recent1->clicked.connect([]() {
        // Opening recent file: demo.cc
    });
    recent_submenu->add_item(std::move(recent1));

    auto recent2 = std::make_unique<onyxui::menu_item<Backend>>("main.cc");
    recent2->clicked.connect([]() {
        // Opening recent file: main.cc
    });
    recent_submenu->add_item(std::move(recent2));

    auto recent3 = std::make_unique<onyxui::menu_item<Backend>>("test.cc");
    recent3->clicked.connect([]() {
        // Opening recent file: test.cc
    });
    recent_submenu->add_item(std::move(recent3));

    // Attach Recent Files submenu to Recent item
    recent_item->set_submenu(std::move(recent_submenu));
    open_submenu->add_item(std::move(recent_item));

    // Attach Open submenu to Open item
    open_item->set_submenu(std::move(open_submenu));
    file_menu->add_item(std::move(open_item));

    file_menu->add_separator();

    auto quit_item = std::make_unique<onyxui::menu_item<Backend>>("Quit");
    quit_item->set_action(quit_action);
    file_menu->add_item(std::move(quit_item));

    menu_bar_ptr->add_menu("&File", std::move(file_menu));

    // Theme menu - dynamically built from theme registry
    auto theme_menu = std::make_unique<onyxui::menu<Backend>>();

    for (size_t i = 0; i < theme_actions.size(); ++i) {
        auto theme_item = std::make_unique<onyxui::menu_item<Backend>>(theme_names[i]);
        theme_item->set_action(theme_actions[i]);
        theme_menu->add_item(std::move(theme_item));
    }

    menu_bar_ptr->add_menu("&Theme", std::move(theme_menu));

    // Window menu
    auto window_menu = std::make_unique<onyxui::menu<Backend>>();

    // Basic Window
    auto basic_win_item = std::make_unique<onyxui::menu_item<Backend>>("");
    basic_win_item->set_mnemonic_text("&Basic Window");
    basic_win_item->clicked.connect([]() {
        demo_windows::show_basic_window<Backend>(
            "Demo Window",
            "This is a basic window with title bar controls!"
        );
    });
    window_menu->add_item(std::move(basic_win_item));

    // Scrollable Window
    auto scroll_win_item = std::make_unique<onyxui::menu_item<Backend>>("");
    scroll_win_item->set_mnemonic_text("&Scrollable Content");
    scroll_win_item->clicked.connect([]() {
        demo_windows::show_scrollable_window<Backend>();
    });
    window_menu->add_item(std::move(scroll_win_item));

    // Controls Window
    auto controls_win_item = std::make_unique<onyxui::menu_item<Backend>>("");
    controls_win_item->set_mnemonic_text("&Interactive Controls");
    controls_win_item->clicked.connect([]() {
        demo_windows::show_controls_window<Backend>();
    });
    window_menu->add_item(std::move(controls_win_item));

    window_menu->add_separator();

    // Modal Dialog
    auto modal_item = std::make_unique<onyxui::menu_item<Backend>>("");
    modal_item->set_mnemonic_text("&Modal Dialog");
    modal_item->clicked.connect([]() {
        demo_windows::show_modal_dialog<Backend>(
            "This is a modal dialog.\nIt blocks other windows until closed."
        );
    });
    window_menu->add_item(std::move(modal_item));

    window_menu->add_separator();

    // Window List (Ctrl+W)
    auto win_list_item = std::make_unique<onyxui::menu_item<Backend>>("");
    win_list_item->set_mnemonic_text("Window &List...\\tCtrl+W");
    win_list_item->clicked.connect([]() {
        auto* window_mgr = onyxui::ui_services<Backend>::windows();
        if (window_mgr) {
            window_mgr->show_window_list();
        }
    });
    window_menu->add_item(std::move(win_list_item));

    menu_bar_ptr->add_menu("&Window", std::move(window_menu));

    // Help menu
    auto help_menu = std::make_unique<onyxui::menu<Backend>>();

    auto about_item = std::make_unique<onyxui::menu_item<Backend>>("About");
    about_item->set_action(about_action);
    help_menu->add_item(std::move(about_item));

    menu_bar_ptr->add_menu("&Help", std::move(help_menu));

    // Store pointer before moving
    auto* menu_bar = menu_bar_ptr.get();

    // Add menu bar to UI
    widget->add_child(std::move(menu_bar_ptr));

    return menu_bar;
}

} // namespace demo_menu_builder
