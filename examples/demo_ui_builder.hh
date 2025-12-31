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
#include <onyxui/widgets/progress_bar.hh>
#include <onyxui/widgets/input/slider.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <onyxui/mvc/views/list_view.hh>
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
    central->set_vbox_layout(onyxui::spacing::tiny);  // Vertical layout with 1px spacing
    central->set_padding(onyxui::thickness(onyxui::logical_unit(0)));  // No padding for compact look

    // Title (add to central widget)
    add_label(*central, "DOS Theme Showcase - New Theme System");

    // Theme label (keep pointer for updates)
    theme_label = add_label(*central,
        demo_utils::get_current_theme_display<Backend>(current_theme_index, theme_names));

    // Button section
    add_label(*central, "Button States:");

    auto* screenshot_btn = add_button(*central, "Screenshot");
    screenshot_btn->set_focusable(true);
    screenshot_btn->set_horizontal_align(onyxui::horizontal_alignment::left);
    screenshot_btn->clicked.connect([widget]() {
        widget->take_screenshot();
    });

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
    width_constraint.preferred_size = onyxui::logical_unit(30);
    width_constraint.min_size = onyxui::logical_unit(30);
    width_constraint.max_size = onyxui::logical_unit(30);
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

    // Create button group as a widget container (owns its radio buttons)
    auto* group = central->template emplace_child<onyxui::button_group>();
    group->set_horizontal_align(onyxui::horizontal_alignment::left);
    group->add_option("Small", 0);
    group->add_option("Medium", 1);
    group->add_option("Large", 2);
    group->set_checked_id(1);  // Medium selected by default

    // Keep reference for caller (if needed)
    size_group = std::shared_ptr<onyxui::button_group<Backend>>(group, [](auto*){});

    // Progress Bar Section
    add_label(*central, "Progress Bar:");
    auto* progress1 = central->template emplace_child<onyxui::progress_bar>();
    progress1->set_horizontal_align(onyxui::horizontal_alignment::left);
    progress1->set_range(0, 100);
    progress1->set_value(45);
    progress1->set_text_visible(false);  // Disable text overlay to show track
    progress1->set_text_format("%v%");
    onyxui::size_constraint progress_width;
    progress_width.policy = onyxui::size_policy::content;
    progress_width.preferred_size = onyxui::logical_unit(40);
    progress_width.min_size = onyxui::logical_unit(40);
    progress_width.max_size = onyxui::logical_unit(40);
    progress1->set_width_constraint(progress_width);

    auto* progress2 = central->template emplace_child<onyxui::progress_bar>();
    progress2->set_horizontal_align(onyxui::horizontal_alignment::left);
    progress2->set_range(0, 100);
    progress2->set_value(75);
    progress2->set_text_visible(false);  // Disable text overlay to show track
    progress2->set_text_format("%v/%m");
    progress2->set_width_constraint(progress_width);

    // Slider Section
    add_label(*central, "Slider:");
    auto* slider1 = central->template emplace_child<onyxui::slider>();
    slider1->set_horizontal_align(onyxui::horizontal_alignment::left);
    slider1->set_range(0, 100);
    slider1->set_value(50);
    slider1->set_single_step(5);
    slider1->set_page_step(10);
    onyxui::size_constraint slider_width;
    slider_width.policy = onyxui::size_policy::content;
    slider_width.preferred_size = onyxui::logical_unit(50);
    slider_width.min_size = onyxui::logical_unit(50);
    slider_width.max_size = onyxui::logical_unit(50);
    slider1->set_width_constraint(slider_width);

    // Ensure slider has enough height to render
    onyxui::size_constraint slider_height;
    slider_height.policy = onyxui::size_policy::content;
    slider_height.preferred_size = onyxui::logical_unit(3);
    slider_height.min_size = onyxui::logical_unit(3);
    slider1->set_height_constraint(slider_height);

    // Connect slider to progress bar for interactive demo
    slider1->value_changed.connect([progress1](int value) {
        progress1->set_value(value);
    });

    // Combo Box Section
    add_label(*central, "Combo Box (MVC):");
    static auto combo_model = std::make_shared<onyxui::list_model<std::string, Backend>>();
    static bool combo_model_initialized = false;
    if (!combo_model_initialized) {
        combo_model->set_items({"Small", "Medium", "Large", "X-Large"});
        combo_model_initialized = true;
    }

    auto* combo1 = central->template emplace_child<onyxui::combo_box>();
    combo1->set_horizontal_align(onyxui::horizontal_alignment::left);
    combo1->set_model(combo_model.get());
    combo1->set_current_index(1);  // Select "Medium" by default

    onyxui::size_constraint combo_width;
    combo_width.policy = onyxui::size_policy::content;
    combo_width.preferred_size = onyxui::logical_unit(20);
    combo_width.min_size = onyxui::logical_unit(15);
    combo_width.max_size = onyxui::logical_unit(30);
    combo1->set_width_constraint(combo_width);

    // Tab Widget Section
    add_label(*central, "Tab Widget:");
    auto tabs = std::make_unique<onyxui::tab_widget<Backend>>();
    tabs->set_horizontal_align(onyxui::horizontal_alignment::left);
    tabs->set_tabs_closable(true);

    // Create tab content pages (many tabs to test overflow scrolling)
    auto page1 = std::make_unique<onyxui::label<Backend>>("Content of Tab 1");
    auto page2 = std::make_unique<onyxui::label<Backend>>("Content of Tab 2");
    auto page3 = std::make_unique<onyxui::label<Backend>>("Content of Tab 3");
    auto page4 = std::make_unique<onyxui::label<Backend>>("Content of Tab 4");
    auto page5 = std::make_unique<onyxui::label<Backend>>("Content of Tab 5");
    auto page6 = std::make_unique<onyxui::label<Backend>>("Content of Tab 6");
    auto page7 = std::make_unique<onyxui::label<Backend>>("Content of Tab 7");

    // Create MVC Demo tab with list_view
    auto page8 = std::make_unique<onyxui::panel<Backend>>();
    page8->set_vbox_layout(onyxui::spacing::tiny);

    // Create model with sample data (static to keep it alive)
    static auto mvc_model = std::make_shared<onyxui::list_model<std::string, Backend>>();
    static bool mvc_model_initialized = false;
    if (!mvc_model_initialized) {
        mvc_model->set_items({
            "Apple",
            "Banana",
            "Cherry",
            "Date",
            "Elderberry",
            "Fig",
            "Grape",
            "Honeydew"
        });
        mvc_model_initialized = true;
    }

    // Create list view
    auto mvc_list = std::make_unique<onyxui::list_view<Backend>>();
    mvc_list->set_model(mvc_model.get());

    // Set size constraints for the list view
    onyxui::size_constraint mvc_height;
    mvc_height.policy = onyxui::size_policy::content;
    mvc_height.preferred_size = onyxui::logical_unit(6);  // Show 6 items
    mvc_height.min_size = onyxui::logical_unit(4);
    mvc_height.max_size = onyxui::logical_unit(8);
    mvc_list->set_height_constraint(mvc_height);

    // Add label and list to page
    page8->template emplace_child<onyxui::label>("MVC List View Demo:");
    page8->add_child(std::move(mvc_list));
    page8->template emplace_child<onyxui::label>("Click items to select, use arrow keys to navigate");

    // Create a scrollable tab with lots of content
    auto page9 = std::make_unique<onyxui::scroll_view<Backend>>();
    auto* scroll_content = page9->content();
    for (int i = 1; i <= 20; ++i) {
        scroll_content->template emplace_child<onyxui::label>(
            "Scrollable line " + std::to_string(i)
        );
    }

    tabs->add_tab(std::move(page1), "First");
    tabs->add_tab(std::move(page2), "Second");
    tabs->add_tab(std::move(page3), "Third");
    tabs->add_tab(std::move(page4), "Fourth");
    tabs->add_tab(std::move(page5), "Fifth");
    tabs->add_tab(std::move(page6), "Sixth");
    tabs->add_tab(std::move(page7), "Seventh");
    tabs->add_tab(std::move(page8), "MVC Demo");
    tabs->add_tab(std::move(page9), "Scroll");

    // Connect close button handler
    auto* tabs_ptr = tabs.get();
    tabs->tab_close_requested.connect([tabs_ptr](int index) {
        tabs_ptr->remove_tab(index);
    });

    // Set size constraints for the tab widget
    onyxui::size_constraint tabs_width;
    tabs_width.policy = onyxui::size_policy::content;
    tabs_width.preferred_size = onyxui::logical_unit(50);
    tabs_width.min_size = onyxui::logical_unit(40);
    tabs_width.max_size = onyxui::logical_unit(60);
    tabs->set_width_constraint(tabs_width);

    onyxui::size_constraint tabs_height;
    tabs_height.policy = onyxui::size_policy::content;
    tabs_height.preferred_size = onyxui::logical_unit(10);  // Taller to show scrollable content
    tabs_height.min_size = onyxui::logical_unit(10);
    tabs->set_height_constraint(tabs_height);

    central->add_child(std::move(tabs));

    // Instructions
    add_label(*central, "");
    add_label(*central, "Use Arrow/PgUp/PgDn/Home/End keys to control slider (when focused)");
    add_label(*central, "Press 1-4 to switch themes | F10 for menu | ESC to quit");

    // Set central widget on main_window
    widget->set_central_widget(std::move(central));
}

} // namespace demo_ui_builder
