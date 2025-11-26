//
// Created by Claude Code on 2025-11-23.
//
// Modeless Dialog Example - Demonstrates modeless window behavior
// Shows independent focus and non-blocking interaction
//

#pragma once
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/input/checkbox.hh>
#include <memory>
#include <string>

namespace widgets_demo_windows {

/**
 * @brief Create and show modeless dialog example
 * @tparam Backend UI backend type
 *
 * @details
 * Creates a modeless dialog that demonstrates:
 * - Independent focus (can switch between windows)
 * - Non-blocking (all windows remain interactive)
 * - Window z-order (click to bring to front)
 * - Normal window features (move, close, etc.)
 */
template<onyxui::UIBackend Backend>
std::shared_ptr<onyxui::window<Backend>> create_modeless_dialog() {
    // Create window with typical dialog flags
    typename onyxui::window<Backend>::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = true;
    flags.has_maximize_button = false;
    flags.has_menu_button = false;
    flags.is_resizable = false;
    flags.is_movable = true;  // Can be moved around
    flags.is_scrollable = false;

    auto dialog = std::make_shared<onyxui::window<Backend>>("Modeless Dialog", flags);

    // Create content
    auto content = std::make_unique<onyxui::vbox<Backend>>(onyxui::spacing::tiny);
    content->set_padding(onyxui::thickness::all(2));

    // Title
    auto* title_label = content->template emplace_child<onyxui::label>("Modeless Dialog Example");
    title_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Explanation
    content->template emplace_child<onyxui::label>("Modeless Dialog Features:");
    content->template emplace_child<onyxui::label>("  - Allows interaction with other windows");
    content->template emplace_child<onyxui::label>("  - Independent focus management");
    content->template emplace_child<onyxui::label>("  - Can be moved and positioned");
    content->template emplace_child<onyxui::label>("  - Click other windows to switch focus");
    content->template emplace_child<onyxui::label>("  - This dialog stays open while you work");

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Interactive controls to demonstrate non-blocking
    content->template emplace_child<onyxui::label>("Interactive Controls:");

    auto* checkbox1 = content->template emplace_child<onyxui::checkbox>("Option 1");
    checkbox1->set_checked(true);

    auto* checkbox2 = content->template emplace_child<onyxui::checkbox>("Option 2");

    auto* checkbox3 = content->template emplace_child<onyxui::checkbox>("Option 3");

    content->template emplace_child<onyxui::label>("");  // Spacer

    content->template emplace_child<onyxui::label>("Try this:");
    content->template emplace_child<onyxui::label>("  1. Toggle checkboxes here");
    content->template emplace_child<onyxui::label>("  2. Click main window to switch focus");
    content->template emplace_child<onyxui::label>("  3. Both windows remain active!");

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Close button
    auto* button_row = content->template emplace_child<onyxui::hbox>(onyxui::spacing::small);
    button_row->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* close_btn = button_row->template emplace_child<onyxui::button>("Close");
    close_btn->clicked.connect([dialog]() {
        dialog->close();
    });

    // Set content
    dialog->set_content(std::move(content));

    // Set size and position
    dialog->set_size(55, 24);
    dialog->set_position(20, 6);

    return dialog;
}

} // namespace widgets_demo_windows
