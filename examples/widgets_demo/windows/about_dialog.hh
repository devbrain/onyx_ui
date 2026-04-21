//
// Created by Claude Code on 2025-11-23.
//
// About Dialog - Information about OnyxUI Widgets Demo
//

#pragma once
#include "../backend_config.hh"  // Defines concrete Backend type
#include <memory>

namespace widgets_demo_windows {

using ui::button;
using ui::group_box;
using ui::hbox;
using ui::label;
using ui::vbox;
using ui::window;

/**
 * @brief Create and show About dialog
 *
 * @details
 * Simple informational dialog about the OnyxUI Widgets Demo application.
 */
inline std::unique_ptr<window> create_about_dialog() {
    // Create window with minimal flags
    window::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = false;
    flags.has_maximize_button = false;
    flags.has_menu_button = false;
    flags.is_resizable = false;
    flags.is_movable = false;
    flags.is_scrollable = false;

    auto dialog = std::make_unique<window>("About OnyxUI", flags);
    auto* dialog_ptr = dialog.get();

    // Create content
    auto content = std::make_unique<vbox>(onyxui::spacing::tiny);
    content->set_padding(onyxui::logical_thickness(onyxui::logical_unit(2.0)));

    // Title
    auto* title = content->emplace_child<label>("OnyxUI Widgets Demo");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<label>("");  // Spacer

    // Version info
    auto* version_section = content->emplace_child<group_box>();
    version_section->set_title("Version");
    auto* version_vbox = version_section->emplace_child<vbox>(onyxui::spacing::tiny);
    version_vbox->emplace_child<label>("  Version: 1.0");
    version_vbox->emplace_child<label>("  Build Date: 2025-11-23");
    version_vbox->emplace_child<label>("  Framework: OnyxUI");

    // Description
    auto* desc_section = content->emplace_child<group_box>();
    desc_section->set_title("Description");
    auto* desc_vbox = desc_section->emplace_child<vbox>(onyxui::spacing::tiny);
    desc_vbox->emplace_child<label>("Comprehensive demonstration of OnyxUI");
    desc_vbox->emplace_child<label>("framework features:");
    desc_vbox->emplace_child<label>("");
    desc_vbox->emplace_child<label>("  - Complete widget gallery");
    desc_vbox->emplace_child<label>("  - Layout and scrolling systems");
    desc_vbox->emplace_child<label>("  - Event system and focus management");
    desc_vbox->emplace_child<label>("  - MVC pattern with models/views");
    desc_vbox->emplace_child<label>("  - Window spawning and management");
    desc_vbox->emplace_child<label>("  - Modal and modeless dialogs");
    desc_vbox->emplace_child<label>("  - Theming and styling");

    // Credits
    auto* credits_section = content->emplace_child<group_box>();
    credits_section->set_title("Credits");
    auto* credits_vbox = credits_section->emplace_child<vbox>(onyxui::spacing::tiny);
    credits_vbox->emplace_child<label>("  Created by: Claude Code");
    credits_vbox->emplace_child<label>("  Framework: OnyxUI");
    credits_vbox->emplace_child<label>("  Backend: ConIO (Terminal UI)");

    // Hotkeys
    auto* hotkeys_section = content->emplace_child<group_box>();
    hotkeys_section->set_title("Quick Reference");
    auto* hotkeys_vbox = hotkeys_section->emplace_child<vbox>(onyxui::spacing::tiny);
    hotkeys_vbox->emplace_child<label>("  F2 / F9        - Screenshot");
    hotkeys_vbox->emplace_child<label>("  Ctrl+M         - MVC Demo");
    hotkeys_vbox->emplace_child<label>("  Ctrl+T         - Theme Editor");
    hotkeys_vbox->emplace_child<label>("  F12            - Debug Tools");
    hotkeys_vbox->emplace_child<label>("  Alt+F4 / Alt+Q - Exit");
    hotkeys_vbox->emplace_child<label>("  Ctrl+Tab       - Next Tab");

    content->emplace_child<label>("");  // Spacer

    // Close button
    auto* button_row = content->emplace_child<hbox>(onyxui::spacing::medium);
    button_row->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* close_btn = button_row->emplace_child<button>("OK");
    close_btn->clicked.connect([dialog_ptr]() {
        dialog_ptr->close();
    });

    // Set content
    dialog->set_content(std::move(content));

    // Set size and position
    dialog->set_size(60, 30);
    dialog->set_position(10, 2);

    return dialog;
}

} // namespace widgets_demo_windows
