//
// Created by Claude Code on 2025-11-23.
//
// Modal Dialog Example - Demonstrates modal window behavior
// Shows focus trapping, overlay, and blocking interaction with parent
//

#pragma once
#include "../backend_config.hh"  // Defines concrete Backend type
#include <memory>
#include <string>

namespace widgets_demo_windows {

using ui::button;
using ui::hbox;
using ui::label;
using ui::vbox;
using ui::window;

/**
 * @brief Create and show modal dialog example
 * @param message Dialog message to display
 *
 * @details
 * Creates a modal dialog that demonstrates:
 * - Focus trapping (Tab only cycles within modal)
 * - Blocks interaction with parent windows
 * - Dark overlay on background
 * - ESC to close
 * - OK/Cancel buttons
 */
inline std::unique_ptr<window> create_modal_dialog(const std::string& message) {
    // Create window with minimal flags (dialogs don't need all window features)
    window::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = false;
    flags.has_maximize_button = false;
    flags.has_menu_button = false;
    flags.is_resizable = false;
    flags.is_movable = true;   // Allow dragging
    flags.is_scrollable = false;

    auto dialog = std::make_unique<window>("Modal Dialog", flags);
    auto* dialog_ptr = dialog.get();

    auto* content = dialog->template emplace_content<vbox>(onyxui::spacing::tiny);
    content->set_padding(onyxui::logical_thickness(onyxui::logical_unit(2.0)));

    // Title
    auto* title_label = content->emplace_child<label>("Modal Dialog Example");
    title_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<label>("");  // Spacer

    // Message
    auto* msg_label = content->emplace_child<label>(message);
    msg_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<label>("");  // Spacer

    // Explanation
    content->emplace_child<label>("Modal Dialog Features:");
    content->emplace_child<label>("  - Blocks parent window interaction");
    content->emplace_child<label>("  - Focus trapped within dialog");
    content->emplace_child<label>("  - Tab cycles through buttons");
    content->emplace_child<label>("  - ESC or click OK to close");

    content->emplace_child<label>("");  // Spacer

    // Buttons
    auto* button_row = content->emplace_child<hbox>(onyxui::spacing::medium);
    button_row->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* ok_btn = button_row->emplace_child<button>("OK");
    ok_btn->clicked.connect([dialog_ptr]() {
        dialog_ptr->close();
    });

    auto* cancel_btn = button_row->emplace_child<button>("Cancel");
    cancel_btn->clicked.connect([dialog_ptr]() {
        dialog_ptr->close();
    });

    // Set size and position (will be centered by show_modal)
    dialog->set_size(50, 16);
    dialog->set_position(15, 8);  // Initial position, may be overridden by modal centering

    return dialog;
}

} // namespace widgets_demo_windows
