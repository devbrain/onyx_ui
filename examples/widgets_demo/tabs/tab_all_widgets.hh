//
// Created by Claude Code on 2025-11-23.
//
// Tab 1: All Widgets - Complete widget gallery
// Demonstrates every widget type with various states and configurations
//

#pragma once

#include "../backend_config.hh"  // Pulls the simple-shell bundle + all widget headers.

// Headers outside the registry's include set still need explicit pulls.
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
inline std::unique_ptr<ui::panel> create_tab_all_widgets(
    const action_callback& on_screenshot,
    const action_callback& on_theme_editor,
    const action_callback& on_mvc_demo) {
    auto tab = std::make_unique<ui::panel>();
    // Outer margin so the tab's scroll viewport doesn't butt against the
    // window frame. Logical units: 1 LU ≈ 8 px on sdlpp, 1 cell on conio.
    tab->set_padding(onyxui::logical_thickness(onyxui::logical_unit(1.0)));

    // Make entire tab scrollable
    auto* scroll = tab->emplace_child<ui::scroll_view>();

    // Use vbox to hold title and grid
    auto* layout = scroll->content_emplace_child<ui::vbox>(onyxui::spacing::tiny);

    // Title
    auto* title = layout->emplace_child<ui::label>("All Widgets Gallery");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    // Status label to show button click events
    auto* status_label = layout->emplace_child<ui::label>("[Click buttons to see events here]");
    status_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    layout->emplace_child<ui::label>("");  // Spacer

    // Main content container (2-column grid layout). Use `large` spacing
    // so adjacent group-box borders don't visually merge into a single line.
    auto* content = layout->emplace_child<ui::grid>(
        2,                     // 2 columns
        -1,                    // auto rows
        onyxui::spacing::large,  // column spacing
        onyxui::spacing::large,  // row spacing
        true                   // auto-size based on content
    );

    // ========== BASIC WIDGETS SECTION ==========
    auto* basic_section = content->emplace_child<ui::group_box>();
    basic_section->set_title("Basic Widgets");
    basic_section->set_vbox_layout(onyxui::spacing::tiny);
    basic_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    // Buttons
    basic_section->emplace_child<ui::label>("Buttons:");

    auto* btn_row = basic_section->emplace_child<ui::hbox>(onyxui::spacing::medium);
    auto* normal_btn = btn_row->emplace_child<ui::button>("Normal Button");
    normal_btn->clicked.connect([status_label]() {
        status_label->set_text("Normal button clicked!");
    });

    auto* disabled_btn = btn_row->emplace_child<ui::button>("Disabled Button");
    disabled_btn->set_enabled(false);

    auto* mnemonic_btn = btn_row->emplace_child<ui::button>("");
    mnemonic_btn->set_mnemonic_text("&Mnemonic (Alt+M)");
    mnemonic_btn->clicked.connect([status_label]() {
        status_label->set_text("Mnemonic button clicked! (Alt+M)");
    });

    // Labels
    basic_section->emplace_child<ui::label>("");  // Spacer
    basic_section->emplace_child<ui::label>("Labels (Horizontal Alignment):");

    auto* label_left = basic_section->emplace_child<ui::label>("Left Aligned");
    label_left->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* label_center = basic_section->emplace_child<ui::label>("Center Aligned");
    label_center->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* label_right = basic_section->emplace_child<ui::label>("Right Aligned");
    label_right->set_horizontal_align(onyxui::horizontal_alignment::right);

    // Spacer and Spring
    basic_section->emplace_child<ui::label>("");  // Spacer
    basic_section->emplace_child<ui::label>("Layout Helpers:");

    auto* spacer_demo = basic_section->emplace_child<ui::hbox>(onyxui::spacing::none);
    spacer_demo->emplace_child<ui::label>("[Left]");
    spacer_demo->emplace_child<ui::spacer>(20);  // 20 char fixed space
    spacer_demo->emplace_child<ui::label>("[20-char spacer]");
    spacer_demo->emplace_child<ui::spring>();  // Flexible space
    spacer_demo->emplace_child<ui::label>("[Right with spring]");

    // ========== CONTAINERS SECTION ==========
    auto* container_section = content->emplace_child<ui::group_box>();
    container_section->set_title("Container Widgets");
    container_section->set_vbox_layout(onyxui::spacing::tiny);
    container_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    // HBox example
    container_section->emplace_child<ui::label>("HBox (Horizontal Layout):");
    auto* hbox_demo = container_section->emplace_child<ui::hbox>(onyxui::spacing::small);
    hbox_demo->emplace_child<ui::button>("Item 1");
    hbox_demo->emplace_child<ui::button>("Item 2");
    hbox_demo->emplace_child<ui::button>("Item 3");

    // VBox example
    container_section->emplace_child<ui::label>("");  // Spacer
    container_section->emplace_child<ui::label>("VBox (Vertical Layout):");
    auto* vbox_demo = container_section->emplace_child<ui::vbox>(onyxui::spacing::tiny);
    vbox_demo->emplace_child<ui::label>("  - First item");
    vbox_demo->emplace_child<ui::label>("  - Second item");
    vbox_demo->emplace_child<ui::label>("  - Third item");

    // Grid example
    container_section->emplace_child<ui::label>("");  // Spacer
    container_section->emplace_child<ui::label>("Grid (2x2):");
    auto* grid_demo = container_section->emplace_child<ui::grid>(2, 2, onyxui::spacing::none, onyxui::spacing::none);  // 2 cols, 2 rows
    grid_demo->emplace_child<ui::button>("(0,0)");
    grid_demo->emplace_child<ui::button>("(0,1)");
    grid_demo->emplace_child<ui::button>("(1,0)");
    grid_demo->emplace_child<ui::button>("(1,1)");

    // Nested group box
    container_section->emplace_child<ui::label>("");  // Spacer
    container_section->emplace_child<ui::label>("Nested Group Box:");
    auto* nested_group = container_section->emplace_child<ui::group_box>();
    nested_group->set_title("Inner Group");
    nested_group->set_vbox_layout(onyxui::spacing::tiny);
    nested_group->emplace_child<ui::label>("Content inside nested group box");

    // ========== INPUT WIDGETS SECTION ==========
    auto* input_section = content->emplace_child<ui::group_box>();
    input_section->set_title("Input Widgets");
    input_section->set_vbox_layout(onyxui::spacing::tiny);
    input_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    // Line Edit
    input_section->emplace_child<ui::label>("Line Edit (Text Input - 30 chars):");
    auto* edit = input_section->emplace_child<ui::line_edit>();
    edit->set_text("Type here...");
    edit->set_visible_chars(30);  // Backend-agnostic width sizing

    // Checkbox
    input_section->emplace_child<ui::label>("");  // Spacer
    input_section->emplace_child<ui::label>("Checkboxes:");
    auto* cb1 = input_section->emplace_child<ui::checkbox>("Enable feature A");
    cb1->set_checked(true);
    cb1->toggled.connect([status_label](bool checked) {
        status_label->set_text(checked ? "Feature A enabled" : "Feature A disabled");
    });
    auto* cb2 = input_section->emplace_child<ui::checkbox>("Enable feature B");
    cb2->set_checked(false);
    cb2->toggled.connect([status_label](bool checked) {
        status_label->set_text(checked ? "Feature B enabled" : "Feature B disabled");
    });

    // Radio Buttons (Vertical)
    input_section->emplace_child<ui::label>("");  // Spacer
    input_section->emplace_child<ui::label>("Radio Buttons (Vertical):");
    // Create vertical button group (default orientation)
    auto* v_button_group = input_section->emplace_child<ui::button_group>();
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
    input_section->emplace_child<ui::label>("");  // Spacer
    input_section->emplace_child<ui::label>("Radio Buttons (Horizontal):");
    // Create horizontal button group with medium spacing (theme-resolved)
    auto* h_button_group = input_section->emplace_child<ui::button_group>(
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
    input_section->emplace_child<ui::label>("");  // Spacer
    input_section->emplace_child<ui::label>("Slider (0-100):");
    auto* slider = input_section->emplace_child<ui::slider>(onyxui::slider_orientation::horizontal);
    slider->set_range(0, 100);
    slider->set_value(50);
    slider->set_track_length(20);  // Width in logical units (height defaults to 1)

    auto* slider_label = input_section->emplace_child<ui::label>("Value: 50");
    slider->value_changed.connect([slider_label, status_label](int value) {
        slider_label->set_text("Value: " + std::to_string(value));
        status_label->set_text("Slider changed to: " + std::to_string(value));
    });

    // Progress Bar
    input_section->emplace_child<ui::label>("");  // Spacer
    input_section->emplace_child<ui::label>("Progress Bar (75%, width=30):");
    auto* progress = input_section->emplace_child<ui::progress_bar>();
    progress->set_range(0, 100);
    progress->set_value(75);
    progress->set_bar_width(30);  // Backend-agnostic bar sizing

    // Combo Box (using simple API - no MVC knowledge needed)
    input_section->emplace_child<ui::label>("");  // Spacer
    input_section->emplace_child<ui::label>("Combo Box (Dropdown):");
    auto* combo = input_section->emplace_child<ui::combo_box>();
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
    auto* textview_section = content->emplace_child<ui::group_box>();
    textview_section->set_title("Text View");
    textview_section->set_vbox_layout(onyxui::spacing::tiny);
    textview_section->set_vertical_align(onyxui::vertical_alignment::top);

    textview_section->emplace_child<ui::label>("Multi-line scrollable text (10 lines):");

    auto* text_display = textview_section->emplace_child<ui::text_view>();
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
    auto* icons_section = content->emplace_child<ui::group_box>();
    icons_section->set_title("Icons");
    icons_section->set_vbox_layout(onyxui::spacing::tiny);
    icons_section->set_vertical_align(onyxui::vertical_alignment::top);

    icons_section->emplace_child<ui::label>("Available icon styles:");

    // Use the backend's icon_style type
    using icon_style = typename Backend::renderer_type::icon_style;

    // General purpose icons row
    auto* icons_row1 = icons_section->emplace_child<ui::hbox>(onyxui::spacing::large);
    icons_row1->emplace_child<ui::icon>(icon_style::check);
    icons_row1->emplace_child<ui::label>("check");
    icons_row1->emplace_child<ui::icon>(icon_style::cross);
    icons_row1->emplace_child<ui::label>("cross");
    icons_row1->emplace_child<ui::icon>(icon_style::bullet);
    icons_row1->emplace_child<ui::label>("bullet");
    icons_row1->emplace_child<ui::icon>(icon_style::folder);
    icons_row1->emplace_child<ui::label>("folder");

    // Navigation icons row
    auto* icons_row2 = icons_section->emplace_child<ui::hbox>(onyxui::spacing::large);
    icons_row2->emplace_child<ui::icon>(icon_style::arrow_up);
    icons_row2->emplace_child<ui::label>("up");
    icons_row2->emplace_child<ui::icon>(icon_style::arrow_down);
    icons_row2->emplace_child<ui::label>("down");
    icons_row2->emplace_child<ui::icon>(icon_style::arrow_left);
    icons_row2->emplace_child<ui::label>("left");
    icons_row2->emplace_child<ui::icon>(icon_style::arrow_right);
    icons_row2->emplace_child<ui::label>("right");

    // Window icons row
    auto* icons_row3 = icons_section->emplace_child<ui::hbox>(onyxui::spacing::large);
    icons_row3->emplace_child<ui::icon>(icon_style::menu);
    icons_row3->emplace_child<ui::label>("menu");
    icons_row3->emplace_child<ui::icon>(icon_style::minimize);
    icons_row3->emplace_child<ui::label>("min");
    icons_row3->emplace_child<ui::icon>(icon_style::maximize);
    icons_row3->emplace_child<ui::label>("max");
    icons_row3->emplace_child<ui::icon>(icon_style::close_x);
    icons_row3->emplace_child<ui::label>("close");

    // ========== ACTIONS SECTION ==========
    auto* actions_section = content->emplace_child<ui::group_box>();
    actions_section->set_title("Actions");
    int actions_spacing = 0;
    if (auto* themes = ui::ui_services::themes()) {
        if (auto* theme = themes->get_current_theme()) {
            actions_spacing = theme->spacing.resolve(onyxui::spacing::small);
        }
    }
    actions_section->set_layout_strategy(
        std::make_unique<ui::linear_layout>(
            onyxui::direction::horizontal,
            actions_spacing,
            onyxui::horizontal_alignment::left,
            onyxui::vertical_alignment::top
        )
    );
    actions_section->set_vertical_align(onyxui::vertical_alignment::top);  // Don't stretch vertically

    auto* screenshot_btn = actions_section->emplace_child<ui::button>("Take Screenshot (F9)");
    screenshot_btn->clicked.connect([on_screenshot, status_label]() {
        if (on_screenshot) on_screenshot();
        status_label->set_text("Screenshot saved!");
    });

    auto* theme_btn = actions_section->emplace_child<ui::button>("Theme Editor (Ctrl+T)");
    theme_btn->clicked.connect([on_theme_editor, status_label]() {
        if (on_theme_editor) on_theme_editor();
        status_label->set_text("Opening theme editor...");
    });

    auto* mvc_btn = actions_section->emplace_child<ui::button>("MVC Demo (Ctrl+M)");
    mvc_btn->clicked.connect([on_mvc_demo, status_label]() {
        if (on_mvc_demo) on_mvc_demo();
        status_label->set_text("Opening MVC demo...");
    });

    return tab;
}

} // namespace widgets_demo_tabs
