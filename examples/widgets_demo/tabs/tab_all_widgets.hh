//
// Created by Claude Code on 2025-11-23.
//
// Tab 1: All Widgets - Complete widget gallery
// Demonstrates every widget type with various states and configurations
//

#pragma once

#include "../backend_config.hh"  // Defines concrete Backend type

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
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/button_group.hh>
#include <onyxui/widgets/input/slider.hh>
#include <onyxui/widgets/progress_bar.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/icon.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <memory>
#include <functional>

namespace widgets_demo_tabs {

// Callback type for actions
using action_callback = std::function<void()>;

/**
 * @brief Create Tab 1: All Widgets
 *
 * @details
 * Complete widget gallery demonstrating:
 * - Basic widgets (button, label, spacer, spring)
 * - Container widgets (panel, vbox/hbox, grid, group_box)
 * - Input widgets (line_edit, checkbox, radio_button, slider, progress_bar, combo_box)
 * - Text display (text_view - multi-line scrollable text)
 * - Icons (icon widget with various icon_style values)
 * - Actions (screenshot, theme editor)
 *
 * All content is scrollable using scroll_view.
 *
 * Uses concrete Backend type from backend_config.hh - no template syntax needed!
 *
 * @param on_screenshot Callback for screenshot action
 * @param on_theme_editor Callback for theme editor action
 * @param on_mvc_demo Callback for MVC demo action
 * @return Panel containing the complete tab content
 */
inline std::unique_ptr<onyxui::panel<Backend>> create_tab_all_widgets(
    action_callback on_screenshot,
    action_callback on_theme_editor,
    action_callback on_mvc_demo) {
    auto tab = std::make_unique<onyxui::panel<Backend>>();

    // Make entire tab scrollable
    auto* scroll = tab->emplace_child<onyxui::scroll_view>();

    // Use vbox to hold title and grid
    auto* layout = scroll->emplace_child<onyxui::vbox>(onyxui::spacing::tiny);

    // Title
    auto* title = layout->emplace_child<onyxui::label>("All Widgets Gallery");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    // Status label to show button click events
    auto* status_label = layout->emplace_child<onyxui::label>("[Click buttons to see events here]");
    status_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    layout->emplace_child<onyxui::label>("");  // Spacer

    // Main content container (2-column grid layout)
    auto* content = layout->emplace_child<onyxui::grid>(
        2,                     // 2 columns
        -1,                    // auto rows
        onyxui::spacing::medium,  // column spacing
        onyxui::spacing::medium,  // row spacing
        true                   // auto-size based on content
    );

    // ========== BASIC WIDGETS SECTION ==========
    auto* basic_section = content->emplace_child<onyxui::group_box>();
    basic_section->set_title("Basic Widgets");
    basic_section->set_vbox_layout(onyxui::spacing::tiny);
    basic_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    // Buttons
    basic_section->emplace_child<onyxui::label>("Buttons:");

    auto* btn_row = basic_section->emplace_child<onyxui::hbox>(onyxui::spacing::medium);
    auto* normal_btn = btn_row->emplace_child<onyxui::button>("Normal Button");
    normal_btn->clicked.connect([status_label]() {
        status_label->set_text("Normal button clicked!");
    });

    auto* disabled_btn = btn_row->emplace_child<onyxui::button>("Disabled Button");
    disabled_btn->set_enabled(false);

    auto* mnemonic_btn = btn_row->emplace_child<onyxui::button>("");
    mnemonic_btn->set_mnemonic_text("&Mnemonic (Alt+M)");
    mnemonic_btn->clicked.connect([status_label]() {
        status_label->set_text("Mnemonic button clicked! (Alt+M)");
    });

    // Labels
    basic_section->emplace_child<onyxui::label>("");  // Spacer
    basic_section->emplace_child<onyxui::label>("Labels (Horizontal Alignment):");

    auto* label_left = basic_section->emplace_child<onyxui::label>("Left Aligned");
    label_left->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* label_center = basic_section->emplace_child<onyxui::label>("Center Aligned");
    label_center->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* label_right = basic_section->emplace_child<onyxui::label>("Right Aligned");
    label_right->set_horizontal_align(onyxui::horizontal_alignment::right);

    // Spacer and Spring
    basic_section->emplace_child<onyxui::label>("");  // Spacer
    basic_section->emplace_child<onyxui::label>("Layout Helpers:");

    auto* spacer_demo = basic_section->emplace_child<onyxui::hbox>(onyxui::spacing::none);
    spacer_demo->emplace_child<onyxui::label>("[Left]");
    spacer_demo->emplace_child<onyxui::spacer>(20);  // 20 char fixed space
    spacer_demo->emplace_child<onyxui::label>("[20-char spacer]");
    spacer_demo->emplace_child<onyxui::spring>();  // Flexible space
    spacer_demo->emplace_child<onyxui::label>("[Right with spring]");

    // ========== CONTAINERS SECTION ==========
    auto* container_section = content->emplace_child<onyxui::group_box>();
    container_section->set_title("Container Widgets");
    container_section->set_vbox_layout(onyxui::spacing::tiny);
    container_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    // HBox example
    container_section->emplace_child<onyxui::label>("HBox (Horizontal Layout):");
    auto* hbox_demo = container_section->emplace_child<onyxui::hbox>(onyxui::spacing::small);
    hbox_demo->emplace_child<onyxui::button>("Item 1");
    hbox_demo->emplace_child<onyxui::button>("Item 2");
    hbox_demo->emplace_child<onyxui::button>("Item 3");

    // VBox example
    container_section->emplace_child<onyxui::label>("");  // Spacer
    container_section->emplace_child<onyxui::label>("VBox (Vertical Layout):");
    auto* vbox_demo = container_section->emplace_child<onyxui::vbox>(onyxui::spacing::tiny);
    vbox_demo->emplace_child<onyxui::label>("  - First item");
    vbox_demo->emplace_child<onyxui::label>("  - Second item");
    vbox_demo->emplace_child<onyxui::label>("  - Third item");

    // Grid example
    container_section->emplace_child<onyxui::label>("");  // Spacer
    container_section->emplace_child<onyxui::label>("Grid (2x2):");
    auto* grid_demo = container_section->emplace_child<onyxui::grid>(2, 2, onyxui::spacing::none, onyxui::spacing::none);  // 2 cols, 2 rows
    grid_demo->emplace_child<onyxui::button>("(0,0)");
    grid_demo->emplace_child<onyxui::button>("(0,1)");
    grid_demo->emplace_child<onyxui::button>("(1,0)");
    grid_demo->emplace_child<onyxui::button>("(1,1)");

    // Nested group box
    container_section->emplace_child<onyxui::label>("");  // Spacer
    container_section->emplace_child<onyxui::label>("Nested Group Box:");
    auto* nested_group = container_section->emplace_child<onyxui::group_box>();
    nested_group->set_title("Inner Group");
    nested_group->set_vbox_layout(onyxui::spacing::tiny);
    nested_group->emplace_child<onyxui::label>("Content inside nested group box");

    // ========== INPUT WIDGETS SECTION ==========
    auto* input_section = content->emplace_child<onyxui::group_box>();
    input_section->set_title("Input Widgets");
    input_section->set_vbox_layout(onyxui::spacing::tiny);
    input_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    // Line Edit
    input_section->emplace_child<onyxui::label>("Line Edit (Text Input - 30 chars):");
    auto* edit = input_section->emplace_child<onyxui::line_edit>();
    edit->set_text("Type here...");
    edit->set_visible_chars(30);  // Backend-agnostic width sizing

    // Checkbox
    input_section->emplace_child<onyxui::label>("");  // Spacer
    input_section->emplace_child<onyxui::label>("Checkboxes:");
    auto* cb1 = input_section->emplace_child<onyxui::checkbox>("Enable feature A");
    cb1->set_checked(true);
    cb1->toggled.connect([status_label](bool checked) {
        status_label->set_text(checked ? "Feature A enabled" : "Feature A disabled");
    });
    auto* cb2 = input_section->emplace_child<onyxui::checkbox>("Enable feature B");
    cb2->set_checked(false);
    cb2->toggled.connect([status_label](bool checked) {
        status_label->set_text(checked ? "Feature B enabled" : "Feature B disabled");
    });

    // Radio Buttons (Vertical)
    input_section->emplace_child<onyxui::label>("");  // Spacer
    input_section->emplace_child<onyxui::label>("Radio Buttons (Vertical):");
    // Create vertical button group (default orientation)
    auto* v_button_group = input_section->emplace_child<onyxui::button_group>();
    v_button_group->add_option("Option 1", 1);
    v_button_group->add_option("Option 2", 2);
    v_button_group->add_option("Option 3", 3);
    v_button_group->set_checked_id(1);  // Select first by default
    v_button_group->button_toggled.connect([status_label](int id, bool checked) {
        if (checked) {
            status_label->set_text("Vertical group: Option " + std::to_string(id) + " selected");
        }
    });

    // Radio Buttons (Horizontal)
    input_section->emplace_child<onyxui::label>("");  // Spacer
    input_section->emplace_child<onyxui::label>("Radio Buttons (Horizontal):");
    // Create horizontal button group with medium spacing (theme-resolved)
    auto* h_button_group = input_section->emplace_child<onyxui::button_group>(
        onyxui::button_group_orientation::horizontal, onyxui::spacing::medium
    );
    h_button_group->add_option("Red", 10);
    h_button_group->add_option("Green", 11);
    h_button_group->add_option("Blue", 12);
    h_button_group->set_checked_id(11);  // Select Green by default
    h_button_group->button_toggled.connect([status_label](int id, bool checked) {
        if (checked) {
            const char* colors[] = {"Red", "Green", "Blue"};
            status_label->set_text(std::string("Horizontal group: ") + colors[id - 10] + " selected");
        }
    });

    // Slider
    input_section->emplace_child<onyxui::label>("");  // Spacer
    input_section->emplace_child<onyxui::label>("Slider (0-100):");
    auto* slider = input_section->emplace_child<onyxui::slider>(onyxui::slider_orientation::horizontal);
    slider->set_range(0, 100);
    slider->set_value(50);
    slider->set_track_length(20);  // Width in logical units (height defaults to 1)

    auto* slider_label = input_section->emplace_child<onyxui::label>("Value: 50");
    slider->value_changed.connect([slider_label, status_label](int value) {
        slider_label->set_text("Value: " + std::to_string(value));
        status_label->set_text("Slider changed to: " + std::to_string(value));
    });

    // Progress Bar
    input_section->emplace_child<onyxui::label>("");  // Spacer
    input_section->emplace_child<onyxui::label>("Progress Bar (75%, width=30):");
    auto* progress = input_section->emplace_child<onyxui::progress_bar>();
    progress->set_range(0, 100);
    progress->set_value(75);
    progress->set_bar_width(30);  // Backend-agnostic bar sizing

    // Combo Box (using simple API - no MVC knowledge needed)
    input_section->emplace_child<onyxui::label>("");  // Spacer
    input_section->emplace_child<onyxui::label>("Combo Box (Dropdown):");
    auto* combo = input_section->emplace_child<onyxui::combo_box>();
    combo->set_items({"Apple", "Banana", "Cherry", "Date", "Elderberry",
                       "Fig", "Grape", "Honeydew", "Jackfruit", "Kiwi",
                       "Lemon", "Mango"});
    combo->set_current_index(0);
    combo->current_index_changed.connect([status_label, combo](int index) {
        std::string text = combo->current_text();
        if (!text.empty()) {
            status_label->set_text("Combo box: " + text + " selected");
        }
    });

    // ========== TEXT VIEW SECTION ==========
    auto* textview_section = content->emplace_child<onyxui::group_box>();
    textview_section->set_title("Text View");
    textview_section->set_vbox_layout(onyxui::spacing::tiny);
    textview_section->set_vertical_align(onyxui::vertical_alignment::top);

    textview_section->emplace_child<onyxui::label>("Multi-line scrollable text (10 lines):");

    auto* text_display = textview_section->emplace_child<onyxui::text_view>();
    text_display->set_has_border(true);
    text_display->set_horizontal_align(onyxui::horizontal_alignment::stretch);
    // Backend-agnostic height sizing - shows 10 lines with automatic scrolling
    text_display->set_visible_lines(10);
    text_display->set_text(
        "This is a text_view widget.\n"
        "It supports multiple lines of text.\n"
        "Content automatically scrolls when\n"
        "it exceeds the visible area.\n"
        "\n"
        "Features:\n"
        "- Line-based scrolling\n"
        "- Keyboard navigation\n"
        "- Page Up/Down support\n"
        "- Home/End to jump\n"
        "- Efficient rendering\n"
        "- Theme integration\n"
        "\n"
        "Try scrolling with arrow keys!\n"
        "Or use Page Up/Down.\n"
        "Press Home to go to top.\n"
        "Press End to go to bottom."
    );

    // ========== ICONS SECTION ==========
    auto* icons_section = content->emplace_child<onyxui::group_box>();
    icons_section->set_title("Icons");
    icons_section->set_vbox_layout(onyxui::spacing::tiny);
    icons_section->set_vertical_align(onyxui::vertical_alignment::top);

    icons_section->emplace_child<onyxui::label>("Available icon styles:");

    // Use the backend's icon_style type
    using icon_style = typename Backend::renderer_type::icon_style;

    // General purpose icons row
    auto* icons_row1 = icons_section->emplace_child<onyxui::hbox>(onyxui::spacing::large);
    icons_row1->emplace_child<onyxui::icon>(icon_style::check);
    icons_row1->emplace_child<onyxui::label>("check");
    icons_row1->emplace_child<onyxui::icon>(icon_style::cross);
    icons_row1->emplace_child<onyxui::label>("cross");
    icons_row1->emplace_child<onyxui::icon>(icon_style::bullet);
    icons_row1->emplace_child<onyxui::label>("bullet");
    icons_row1->emplace_child<onyxui::icon>(icon_style::folder);
    icons_row1->emplace_child<onyxui::label>("folder");

    // Navigation icons row
    auto* icons_row2 = icons_section->emplace_child<onyxui::hbox>(onyxui::spacing::large);
    icons_row2->emplace_child<onyxui::icon>(icon_style::arrow_up);
    icons_row2->emplace_child<onyxui::label>("up");
    icons_row2->emplace_child<onyxui::icon>(icon_style::arrow_down);
    icons_row2->emplace_child<onyxui::label>("down");
    icons_row2->emplace_child<onyxui::icon>(icon_style::arrow_left);
    icons_row2->emplace_child<onyxui::label>("left");
    icons_row2->emplace_child<onyxui::icon>(icon_style::arrow_right);
    icons_row2->emplace_child<onyxui::label>("right");

    // Window icons row
    auto* icons_row3 = icons_section->emplace_child<onyxui::hbox>(onyxui::spacing::large);
    icons_row3->emplace_child<onyxui::icon>(icon_style::menu);
    icons_row3->emplace_child<onyxui::label>("menu");
    icons_row3->emplace_child<onyxui::icon>(icon_style::minimize);
    icons_row3->emplace_child<onyxui::label>("min");
    icons_row3->emplace_child<onyxui::icon>(icon_style::maximize);
    icons_row3->emplace_child<onyxui::label>("max");
    icons_row3->emplace_child<onyxui::icon>(icon_style::close_x);
    icons_row3->emplace_child<onyxui::label>("close");

    // ========== ACTIONS SECTION ==========
    auto* actions_section = content->emplace_child<onyxui::group_box>();
    actions_section->set_title("Actions");
    int actions_spacing = 0;
    if (auto* themes = onyxui::ui_services<Backend>::themes()) {
        if (auto* theme = themes->get_current_theme()) {
            actions_spacing = theme->spacing.resolve(onyxui::spacing::small);
        }
    }
    actions_section->set_layout_strategy(
        std::make_unique<onyxui::linear_layout<Backend>>(
            onyxui::direction::horizontal,
            actions_spacing,
            onyxui::horizontal_alignment::left,
            onyxui::vertical_alignment::top
        )
    );
    actions_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    auto* screenshot_btn = actions_section->emplace_child<onyxui::button>("Take Screenshot (F9)");
    screenshot_btn->clicked.connect([on_screenshot, status_label]() {
        if (on_screenshot) on_screenshot();
        status_label->set_text("Screenshot saved!");
    });

    auto* theme_btn = actions_section->emplace_child<onyxui::button>("Theme Editor (Ctrl+T)");
    theme_btn->clicked.connect([on_theme_editor, status_label]() {
        if (on_theme_editor) on_theme_editor();
        status_label->set_text("Opening theme editor...");
    });

    auto* mvc_btn = actions_section->emplace_child<onyxui::button>("MVC Demo (Ctrl+M)");
    mvc_btn->clicked.connect([on_mvc_demo, status_label]() {
        if (on_mvc_demo) on_mvc_demo();
        status_label->set_text("Opening MVC demo...");
    });

    return tab;
}

} // namespace widgets_demo_tabs
