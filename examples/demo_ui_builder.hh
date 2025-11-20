//
// Created by igor on 21/10/2025.
//
// UI builder for main_widget demo
// Constructs the complete UI layout with menu, buttons, panels, and text view
//

#pragma once
#include <chrono>
#include <fstream>
#include <string>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/button_group.hh>
#include <onyxui/widgets/containers/panel.hh>
#include "demo_menu_builder.hh"
#include "demo_utils.hh"

namespace demo_ui_builder {

/**
 * @brief Build the complete UI for the demo widget
 *
 * @details
 * Constructs a vertical layout containing:
 * - Menu bar (File, Theme, Help)
 * - Title and theme display labels
 * - Demo panel with border
 * - Button section (Normal, Screenshot, Disabled, Quit)
 * - Scrollable text view with keyboard navigation
 * - Instructions footer
 *
 * @tparam Backend UI backend type
 * @tparam Widget The main widget type
 * @param widget Pointer to the main widget instance
 * @param theme_names List of available theme names
 * @param current_theme_index Current theme index
 * @param theme_actions List of theme switching actions
 * @param new_action Action for New menu item
 * @param open_action Action for Open menu item
 * @param quit_action Action for Quit menu item
 * @param about_action Action for About menu item
 * @param theme_label Output pointer to the theme label widget
 * @param menu_bar Output pointer to the menu bar widget
 * @param size_group Button group for radio buttons (kept alive by caller)
 */
template<typename Backend, typename Widget>
void build_ui(
    Widget* widget,
    const std::vector<std::string>& theme_names,
    size_t current_theme_index,
    const std::vector<std::shared_ptr<onyxui::action<Backend>>>& theme_actions,
    std::shared_ptr<onyxui::action<Backend>> new_action,
    std::shared_ptr<onyxui::action<Backend>> open_action,
    std::shared_ptr<onyxui::action<Backend>> quit_action,
    std::shared_ptr<onyxui::action<Backend>> about_action,
    onyxui::label<Backend>*& theme_label,
    onyxui::menu_bar<Backend>*& menu_bar,
    std::shared_ptr<onyxui::button_group<Backend>>& size_group) {


    // Menu bar (at the top) - use main_window API
    auto menu_bar_widget = demo_menu_builder::build_menu_bar_widget<Backend>(
        widget,
        theme_names,
        theme_actions,
        new_action,
        open_action,
        quit_action,
        about_action
    );
    menu_bar = menu_bar_widget.get();
    widget->set_menu_bar(std::move(menu_bar_widget));

    // Create central widget for all content
    auto central = std::make_unique<onyxui::panel<Backend>>();
    central->set_vbox_layout(1);  // Vertical layout with 1px spacing
    central->set_padding(onyxui::thickness::all(0));  // No padding for compact look

    // Title (add to central widget)
    add_label(*central, "DOS Theme Showcase - New Theme System");

    // Theme label (keep pointer for updates)
    theme_label = add_label(*central,
        demo_utils::get_current_theme_display<Backend>(current_theme_index, theme_names));

    // Button section
    add_label(*central, "Button States:");

    auto* quit_btn = add_button(*central, "Quit");
    quit_btn->set_focusable(true);
    quit_btn->set_horizontal_align(onyxui::horizontal_alignment::left);
    quit_btn->clicked.connect([widget]() {
        widget->quit();
    });

    // Line Edit Section
    add_label(*central, "Line Edit:");
    auto edit1 = std::make_unique<onyxui::line_edit<Backend>>("Type here...");
    edit1->set_horizontal_align(onyxui::horizontal_alignment::left);
    edit1->set_has_border(true);
    onyxui::size_constraint width_constraint;
    width_constraint.policy = onyxui::size_policy::content;
    width_constraint.preferred_size = 30;
    width_constraint.min_size = 30;
    width_constraint.max_size = 30;
    edit1->set_width_constraint(width_constraint);
    central->add_child(std::move(edit1));

    // Checkbox Section
    add_label(*central, "Checkbox:");
    auto* check1 = central->template emplace_child<onyxui::checkbox>("Enable notifications");
    check1->set_horizontal_align(onyxui::horizontal_alignment::left);
    auto* check2 = central->template emplace_child<onyxui::checkbox>("Remember me", true);
    check2->set_horizontal_align(onyxui::horizontal_alignment::left);

    // Radio Button Section
    add_label(*central, "Radio Buttons:");

    // Create button group (kept alive by caller)
    size_group = std::make_shared<onyxui::button_group<Backend>>();

    auto* radio_small = central->template emplace_child<onyxui::radio_button>("Small");
    radio_small->set_horizontal_align(onyxui::horizontal_alignment::left);
    size_group->add_button(radio_small, 0);

    auto* radio_medium = central->template emplace_child<onyxui::radio_button>("Medium");
    radio_medium->set_horizontal_align(onyxui::horizontal_alignment::left);
    size_group->add_button(radio_medium, 1);

    auto* radio_large = central->template emplace_child<onyxui::radio_button>("Large");
    radio_large->set_horizontal_align(onyxui::horizontal_alignment::left);
    size_group->add_button(radio_large, 2);

    size_group->set_checked_id(1);  // Medium selected by default

    // Instructions
    add_label(*central, "");
    add_label(*central, "Press 1-4 to switch themes | F10 for menu | ESC to quit");

    // Set central widget on main_window
    widget->set_central_widget(std::move(central));
}

} // namespace demo_ui_builder
