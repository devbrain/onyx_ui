//
// Created by Claude Code on 2025-11-23.
//
// Modeless Dialog Example - Demonstrates modeless window behavior
// Shows independent focus and non-blocking interaction
//

#pragma once
#include "../backend_config.hh"  // Defines concrete Backend type
#include <memory>
#include <string>

namespace widgets_demo_windows {

using ui::button;
using ui::checkbox;
using ui::hbox;
using ui::label;
using ui::vbox;
using ui::window;

/**
 * @brief Create and show modeless dialog example
 *
 * @details
 * Creates a modeless dialog that demonstrates:
 * - Independent focus (can switch between windows)
 * - Non-blocking (all windows remain interactive)
 * - Window z-order (click to bring to front)
 * - Normal window features (move, close, etc.)
 */
inline std::unique_ptr<window> create_modeless_dialog() {
    // Create window with typical dialog flags
    window::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = true;
    flags.has_maximize_button = false;
    flags.has_menu_button = false;
    flags.is_resizable = false;
    flags.is_movable = true;  // Can be moved around
    flags.is_scrollable = false;

    auto dialog = std::make_unique<window>("Modeless Dialog", flags);
    auto* dialog_ptr = dialog.get();

    // Create content
    auto content = std::make_unique<vbox>(onyxui::spacing::tiny);
    content->set_padding(onyxui::logical_thickness(onyxui::logical_unit(2.0)));

    // Title
    auto* title_label = content->emplace_child<label>("Modeless Dialog Example");
    title_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<label>("");  // Spacer

    // Explanation
    content->emplace_child<label>("Modeless Dialog Features:");
    content->emplace_child<label>("  - Allows interaction with other windows");
    content->emplace_child<label>("  - Independent focus management");
    content->emplace_child<label>("  - Can be moved and positioned");
    content->emplace_child<label>("  - Click other windows to switch focus");
    content->emplace_child<label>("  - This dialog stays open while you work");

    content->emplace_child<label>("");  // Spacer

    // Interactive controls to demonstrate non-blocking
    content->emplace_child<label>("Interactive Controls:");

    auto* checkbox1 = content->emplace_child<checkbox>("Option 1");
    checkbox1->set_checked(true);

    auto* checkbox2 = content->emplace_child<checkbox>("Option 2");

    auto* checkbox3 = content->emplace_child<checkbox>("Option 3");

    content->emplace_child<label>("");  // Spacer

    content->emplace_child<label>("Try this:");
    content->emplace_child<label>("  1. Toggle checkboxes here");
    content->emplace_child<label>("  2. Click main window to switch focus");
    content->emplace_child<label>("  3. Both windows remain active!");

    content->emplace_child<label>("");  // Spacer

    // Close button
    auto* button_row = content->emplace_child<hbox>(onyxui::spacing::medium);
    button_row->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* close_btn = button_row->emplace_child<button>("Close");
    close_btn->clicked.connect([dialog_ptr]() {
        dialog_ptr->close();
    });

    // Set content
    dialog->set_content(std::move(content));

    // Set size and position
    dialog->set_size(55, 24);
    dialog->set_position(20, 6);

    // Suppress unused variable warnings
    (void)checkbox2;
    (void)checkbox3;

    return dialog;
}

} // namespace widgets_demo_windows
