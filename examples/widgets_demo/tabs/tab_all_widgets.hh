//
// Created by Claude Code on 2025-11-23.
//
// Tab 1: All Widgets - Complete widget gallery
// Demonstrates every widget type with various states and configurations
//

#pragma once
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/grid.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/button_group.hh>
#include <onyxui/widgets/input/slider.hh>
#include <onyxui/widgets/progress_bar.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <memory>

namespace widgets_demo_tabs {

/**
 * @brief Create Tab 1: All Widgets
 *
 * @details
 * Complete widget gallery demonstrating:
 * - Basic widgets (button, label, spacer, spring)
 * - Container widgets (panel, vbox/hbox, grid, group_box)
 * - Input widgets (line_edit, checkbox, radio_button, slider, progress_bar, combo_box)
 * - Actions (screenshot, theme editor)
 *
 * All content is scrollable using scroll_view.
 *
 * @tparam Backend UI backend type
 * @tparam WidgetsDemoType Type of the main widgets_demo class
 * @param demo Pointer to main widgets_demo instance (for action callbacks)
 * @return Panel containing the complete tab content
 */
template<onyxui::UIBackend Backend, typename WidgetsDemoType>
std::unique_ptr<onyxui::panel<Backend>> create_tab_all_widgets(WidgetsDemoType* demo) {
    auto tab = std::make_unique<onyxui::panel<Backend>>();

    // Make entire tab scrollable
    auto* scroll = tab->template emplace_child<onyxui::scroll_view>();

    // Main content container (vertical layout)
    auto* content = scroll->template emplace_child<onyxui::vbox>(1);  // 1px spacing

    // Title
    auto* title = content->template emplace_child<onyxui::label>("All Widgets Gallery");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // ========== BASIC WIDGETS SECTION ==========
    auto* basic_section = content->template emplace_child<onyxui::group_box>();
    basic_section->set_title("Basic Widgets");
    basic_section->set_vbox_layout(1);

    // Buttons
    basic_section->template emplace_child<onyxui::label>("Buttons:");

    auto* btn_row = basic_section->template emplace_child<onyxui::hbox>(2);
    auto* normal_btn = btn_row->template emplace_child<onyxui::button>("Normal Button");
    normal_btn->clicked.connect([]() {
        std::cout << "Normal button clicked!\n";
    });

    auto* disabled_btn = btn_row->template emplace_child<onyxui::button>("Disabled Button");
    disabled_btn->set_enabled(false);

    auto* mnemonic_btn = btn_row->template emplace_child<onyxui::button>("");
    mnemonic_btn->set_mnemonic_text("&Mnemonic (Alt+M)");
    mnemonic_btn->clicked.connect([]() {
        std::cout << "Mnemonic button clicked!\n";
    });

    // Labels
    basic_section->template emplace_child<onyxui::label>("");  // Spacer
    basic_section->template emplace_child<onyxui::label>("Labels (Horizontal Alignment):");

    auto* label_left = basic_section->template emplace_child<onyxui::label>("Left Aligned");
    label_left->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* label_center = basic_section->template emplace_child<onyxui::label>("Center Aligned");
    label_center->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* label_right = basic_section->template emplace_child<onyxui::label>("Right Aligned");
    label_right->set_horizontal_align(onyxui::horizontal_alignment::right);

    // Spacer and Spring
    basic_section->template emplace_child<onyxui::label>("");  // Spacer
    basic_section->template emplace_child<onyxui::label>("Layout Helpers:");

    auto* spacer_demo = basic_section->template emplace_child<onyxui::hbox>(0);
    spacer_demo->template emplace_child<onyxui::label>("[Left]");
    spacer_demo->template emplace_child<onyxui::spacer>(20);  // 20 char fixed space
    spacer_demo->template emplace_child<onyxui::label>("[20-char spacer]");
    spacer_demo->template emplace_child<onyxui::spring>();  // Flexible space
    spacer_demo->template emplace_child<onyxui::label>("[Right with spring]");

    // ========== CONTAINERS SECTION ==========
    auto* container_section = content->template emplace_child<onyxui::group_box>();
    container_section->set_title("Container Widgets");
    container_section->set_vbox_layout(1);

    // HBox example
    container_section->template emplace_child<onyxui::label>("HBox (Horizontal Layout):");
    auto* hbox_demo = container_section->template emplace_child<onyxui::hbox>(2);
    hbox_demo->template emplace_child<onyxui::button>("Item 1");
    hbox_demo->template emplace_child<onyxui::button>("Item 2");
    hbox_demo->template emplace_child<onyxui::button>("Item 3");

    // VBox example
    container_section->template emplace_child<onyxui::label>("");  // Spacer
    container_section->template emplace_child<onyxui::label>("VBox (Vertical Layout):");
    auto* vbox_demo = container_section->template emplace_child<onyxui::vbox>(1);
    vbox_demo->template emplace_child<onyxui::label>("  - First item");
    vbox_demo->template emplace_child<onyxui::label>("  - Second item");
    vbox_demo->template emplace_child<onyxui::label>("  - Third item");

    // Grid example
    container_section->template emplace_child<onyxui::label>("");  // Spacer
    container_section->template emplace_child<onyxui::label>("Grid (2x2):");
    auto* grid_demo = container_section->template emplace_child<onyxui::grid>(2, 2);  // 2 rows, 2 cols
    grid_demo->template emplace_child<onyxui::button>("(0,0)");
    grid_demo->template emplace_child<onyxui::button>("(0,1)");
    grid_demo->template emplace_child<onyxui::button>("(1,0)");
    grid_demo->template emplace_child<onyxui::button>("(1,1)");

    // Nested group box
    container_section->template emplace_child<onyxui::label>("");  // Spacer
    container_section->template emplace_child<onyxui::label>("Nested Group Box:");
    auto* nested_group = container_section->template emplace_child<onyxui::group_box>();
    nested_group->set_title("Inner Group");
    nested_group->set_vbox_layout(1);
    nested_group->template emplace_child<onyxui::label>("Content inside nested group box");

    // ========== INPUT WIDGETS SECTION ==========
    auto* input_section = content->template emplace_child<onyxui::group_box>();
    input_section->set_title("Input Widgets");
    input_section->set_vbox_layout(1);

    // Line Edit
    input_section->template emplace_child<onyxui::label>("Line Edit (Text Input):");
    auto* edit = input_section->template emplace_child<onyxui::line_edit>();
    edit->set_text("Type here...");

    // Checkbox
    input_section->template emplace_child<onyxui::label>("");  // Spacer
    input_section->template emplace_child<onyxui::label>("Checkboxes:");
    auto* cb1 = input_section->template emplace_child<onyxui::checkbox>("Enable feature A");
    cb1->set_checked(true);
    auto* cb2 = input_section->template emplace_child<onyxui::checkbox>("Enable feature B");
    cb2->set_checked(false);

    // Radio Buttons
    input_section->template emplace_child<onyxui::label>("");  // Spacer
    input_section->template emplace_child<onyxui::label>("Radio Buttons (Mutually Exclusive):");
    auto button_group = std::make_shared<onyxui::button_group<Backend>>();
    auto* rb1 = input_section->template emplace_child<onyxui::radio_button>("Option 1", button_group.get());
    auto* rb2 = input_section->template emplace_child<onyxui::radio_button>("Option 2", button_group.get());
    auto* rb3 = input_section->template emplace_child<onyxui::radio_button>("Option 3", button_group.get());
    rb1->set_checked(true);  // Select first by default

    // Slider
    input_section->template emplace_child<onyxui::label>("");  // Spacer
    input_section->template emplace_child<onyxui::label>("Slider (0-100):");
    auto* slider = input_section->template emplace_child<onyxui::slider>(onyxui::slider_orientation::horizontal);
    slider->set_range(0, 100);
    slider->set_value(50);

    auto* slider_label = input_section->template emplace_child<onyxui::label>("Value: 50");
    slider->value_changed.connect([slider_label](int value) {
        slider_label->set_text("Value: " + std::to_string(value));
    });

    // Progress Bar
    input_section->template emplace_child<onyxui::label>("");  // Spacer
    input_section->template emplace_child<onyxui::label>("Progress Bar (75%):");
    auto* progress = input_section->template emplace_child<onyxui::progress_bar>();
    progress->set_range(0, 100);
    progress->set_value(75);

    // Combo Box
    input_section->template emplace_child<onyxui::label>("");  // Spacer
    input_section->template emplace_child<onyxui::label>("Combo Box (Dropdown):");
    auto combo_model = std::make_shared<onyxui::list_model<std::string, Backend>>();
    combo_model->set_items({"Apple", "Banana", "Cherry", "Date", "Elderberry"});
    auto* combo = input_section->template emplace_child<onyxui::combo_box>();
    combo->set_model(combo_model.get());
    combo->set_current_index(0);

    // ========== ACTIONS SECTION ==========
    auto* actions_section = content->template emplace_child<onyxui::group_box>();
    actions_section->set_title("Actions");
    actions_section->set_hbox_layout(2);

    auto* screenshot_btn = actions_section->template emplace_child<onyxui::button>("Take Screenshot (F9)");
    screenshot_btn->clicked.connect([demo]() {
        demo->take_screenshot();
    });

    auto* theme_btn = actions_section->template emplace_child<onyxui::button>("Theme Editor (Ctrl+T)");
    theme_btn->clicked.connect([demo]() {
        demo->show_theme_editor();
    });

    auto* mvc_btn = actions_section->template emplace_child<onyxui::button>("MVC Demo (Ctrl+M)");
    mvc_btn->clicked.connect([demo]() {
        demo->show_mvc_demo();
    });

    return tab;
}

} // namespace widgets_demo_tabs
