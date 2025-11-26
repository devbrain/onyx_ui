//
// Created by Claude Code on 2025-11-23.
//
// About Dialog - Information about OnyxUI Widgets Demo
//

#pragma once
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <memory>

namespace widgets_demo_windows {

/**
 * @brief Create and show About dialog
 * @tparam Backend UI backend type
 *
 * @details
 * Simple informational dialog about the OnyxUI Widgets Demo application.
 */
template<onyxui::UIBackend Backend>
std::shared_ptr<onyxui::window<Backend>> create_about_dialog() {
    // Create window with minimal flags
    typename onyxui::window<Backend>::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = false;
    flags.has_maximize_button = false;
    flags.has_menu_button = false;
    flags.is_resizable = false;
    flags.is_movable = false;
    flags.is_scrollable = false;

    auto dialog = std::make_shared<onyxui::window<Backend>>("About OnyxUI", flags);

    // Create content
    auto content = std::make_unique<onyxui::vbox<Backend>>(onyxui::spacing::tiny);
    content->set_padding(onyxui::thickness::all(2));

    // Title
    auto* title = content->template emplace_child<onyxui::label>("OnyxUI Widgets Demo");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Version info
    auto* version_section = content->template emplace_child<onyxui::group_box>();
    version_section->set_title("Version");
    auto* version_vbox = version_section->template emplace_child<onyxui::vbox>(onyxui::spacing::tiny);
    version_vbox->template emplace_child<onyxui::label>("  Version: 1.0");
    version_vbox->template emplace_child<onyxui::label>("  Build Date: 2025-11-23");
    version_vbox->template emplace_child<onyxui::label>("  Framework: OnyxUI");

    // Description
    auto* desc_section = content->template emplace_child<onyxui::group_box>();
    desc_section->set_title("Description");
    auto* desc_vbox = desc_section->template emplace_child<onyxui::vbox>(onyxui::spacing::tiny);
    desc_vbox->template emplace_child<onyxui::label>("Comprehensive demonstration of OnyxUI");
    desc_vbox->template emplace_child<onyxui::label>("framework features:");
    desc_vbox->template emplace_child<onyxui::label>("");
    desc_vbox->template emplace_child<onyxui::label>("  - Complete widget gallery");
    desc_vbox->template emplace_child<onyxui::label>("  - Layout and scrolling systems");
    desc_vbox->template emplace_child<onyxui::label>("  - Event system and focus management");
    desc_vbox->template emplace_child<onyxui::label>("  - MVC pattern with models/views");
    desc_vbox->template emplace_child<onyxui::label>("  - Window spawning and management");
    desc_vbox->template emplace_child<onyxui::label>("  - Modal and modeless dialogs");
    desc_vbox->template emplace_child<onyxui::label>("  - Theming and styling");

    // Credits
    auto* credits_section = content->template emplace_child<onyxui::group_box>();
    credits_section->set_title("Credits");
    auto* credits_vbox = credits_section->template emplace_child<onyxui::vbox>(onyxui::spacing::tiny);
    credits_vbox->template emplace_child<onyxui::label>("  Created by: Claude Code");
    credits_vbox->template emplace_child<onyxui::label>("  Framework: OnyxUI");
    credits_vbox->template emplace_child<onyxui::label>("  Backend: ConIO (Terminal UI)");

    // Hotkeys
    auto* hotkeys_section = content->template emplace_child<onyxui::group_box>();
    hotkeys_section->set_title("Quick Reference");
    auto* hotkeys_vbox = hotkeys_section->template emplace_child<onyxui::vbox>(onyxui::spacing::tiny);
    hotkeys_vbox->template emplace_child<onyxui::label>("  F2 / F9        - Screenshot");
    hotkeys_vbox->template emplace_child<onyxui::label>("  Ctrl+M         - MVC Demo");
    hotkeys_vbox->template emplace_child<onyxui::label>("  Ctrl+T         - Theme Editor");
    hotkeys_vbox->template emplace_child<onyxui::label>("  F12            - Debug Tools");
    hotkeys_vbox->template emplace_child<onyxui::label>("  Alt+F4 / Alt+Q - Exit");
    hotkeys_vbox->template emplace_child<onyxui::label>("  Ctrl+Tab       - Next Tab");

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Close button
    auto* button_row = content->template emplace_child<onyxui::hbox>(onyxui::spacing::small);
    button_row->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* close_btn = button_row->template emplace_child<onyxui::button>("OK");
    close_btn->clicked.connect([dialog]() {
        dialog->close();
    });

    // Set content
    dialog->set_content(std::move(content));

    // Set size and position
    dialog->set_size(60, 30);
    dialog->set_position(10, 2);

    return dialog;
}

} // namespace widgets_demo_windows
