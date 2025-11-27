//
// Created by Claude Code on 2025-11-23.
//
// Modal Dialog Example - Demonstrates modal window behavior
// Shows focus trapping, overlay, and blocking interaction with parent
//

#pragma once
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <memory>
#include <string>

namespace widgets_demo_windows {

/**
 * @brief Create and show modal dialog example
 * @tparam Backend UI backend type
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
template<onyxui::UIBackend Backend>
std::shared_ptr<onyxui::window<Backend>> create_modal_dialog(const std::string& message) {
    // Create window with minimal flags (dialogs don't need all window features)
    typename onyxui::window<Backend>::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = false;
    flags.has_maximize_button = false;
    flags.has_menu_button = false;
    flags.is_resizable = false;
    flags.is_movable = false;  // Centered, non-movable
    flags.is_scrollable = false;

    auto dialog = std::make_shared<onyxui::window<Backend>>("Modal Dialog", flags);

    // Create content
    auto content = std::make_unique<onyxui::vbox<Backend>>(onyxui::spacing::tiny);
    content->set_padding(onyxui::logical_thickness(onyxui::logical_unit(2.0)));

    // Title
    auto* title_label = content->template emplace_child<onyxui::label>("Modal Dialog Example");
    title_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Message
    auto* msg_label = content->template emplace_child<onyxui::label>(message);
    msg_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Explanation
    content->template emplace_child<onyxui::label>("Modal Dialog Features:");
    content->template emplace_child<onyxui::label>("  - Blocks parent window interaction");
    content->template emplace_child<onyxui::label>("  - Focus trapped within dialog");
    content->template emplace_child<onyxui::label>("  - Tab cycles through buttons");
    content->template emplace_child<onyxui::label>("  - ESC or click OK to close");

    content->template emplace_child<onyxui::label>("");  // Spacer

    // Buttons
    auto* button_row = content->template emplace_child<onyxui::hbox>(onyxui::spacing::small);
    button_row->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* ok_btn = button_row->template emplace_child<onyxui::button>("OK");
    ok_btn->clicked.connect([dialog]() {
        dialog->close();
    });

    auto* cancel_btn = button_row->template emplace_child<onyxui::button>("Cancel");
    cancel_btn->clicked.connect([dialog]() {
        dialog->close();
    });

    // Set content
    dialog->set_content(std::move(content));

    // Set size and position (will be centered by show_modal)
    dialog->set_size(50, 16);
    dialog->set_position(15, 8);  // Initial position, may be overridden by modal centering

    return dialog;
}

} // namespace widgets_demo_windows
