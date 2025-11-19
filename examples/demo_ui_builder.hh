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
 * @param renderer Pointer to renderer (for screenshot functionality)
 * @param theme_label Output pointer to the theme label widget
 * @param menu_bar Output pointer to the menu bar widget
 * @param text_view Output pointer to the text view widget
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
    typename Backend::renderer_type* renderer,
    onyxui::label<Backend>*& theme_label,
    onyxui::menu_bar<Backend>*& menu_bar,
    onyxui::text_view<Backend>*& text_view) {


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

    // Spacer
    add_label(*central, "");

    // Demo panel with border
    auto* demo_panel = add_panel(*central);
    demo_panel->set_has_border(true);
    demo_panel->set_padding(onyxui::thickness::all(1));
    demo_panel->set_vbox_layout(1);

    add_label(*demo_panel, "Panel with Border");
    add_label(*demo_panel, "Themes via service locator");

    // Spacer
    add_label(*central, "");

    // Button section
    add_label(*central, "Button States:");

    auto* normal_btn = add_button(*central, "Normal");
    normal_btn->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* screenshot_btn = add_button(*central, "Screenshot");
    screenshot_btn->set_horizontal_align(onyxui::horizontal_alignment::left);
    screenshot_btn->clicked.connect([widget]() {

        auto* renderer = widget->get_renderer();
        if (!renderer) {
            return;
        }

        // Generate filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count();
        std::string filename = "screenshot_" + std::to_string(timestamp) + ".txt";

        // Take screenshot
        std::ofstream file(filename);
        if (!file) {
            return;
        }

        renderer->take_screenshot(file);
        file.close();
    });

    auto* disabled_btn = add_button(*central, "Disabled");
    disabled_btn->set_enabled(false);
    disabled_btn->set_horizontal_align(onyxui::horizontal_alignment::left);

    // Spacer
    add_label(*central, "");

    // Quit button (test mouse interaction!)
    auto* quit_btn = add_button(*central, "Quit");
    quit_btn->set_focusable(true);
    quit_btn->set_horizontal_align(onyxui::horizontal_alignment::left);
    quit_btn->clicked.connect([widget]() {
        widget->quit();
    });

    // Spacer
    add_label(*central, "");

    // Line Edit Section (Text input demonstration)
    add_label(*central, "Line Edit (Text Input):");

    // Basic text input with border (fixed width to test text scrolling)
    auto edit1 = std::make_unique<onyxui::line_edit<Backend>>("Type here...");
    edit1->set_horizontal_align(onyxui::horizontal_alignment::left);
    edit1->set_has_border(true);

    // Set fixed width to test horizontal text scrolling
    onyxui::size_constraint width_constraint;
    width_constraint.policy = onyxui::size_policy::content;
    width_constraint.preferred_size = 30;  // 30 characters wide
    width_constraint.min_size = 30;
    width_constraint.max_size = 30;
    edit1->set_width_constraint(width_constraint);

    auto* edit1_ptr = edit1.get();
    central->add_child(std::move(edit1));

    // Status label to show what was typed
    auto* status_label = add_label(*central, "Type something and press Enter");
    edit1_ptr->text_changed.connect([status_label](const std::string& text) {
        status_label->set_text("Current text: " + (text.empty() ? "(empty)" : text));
    });

    // Password mode example
    auto edit2 = std::make_unique<onyxui::line_edit<Backend>>();
    edit2->set_placeholder("Password (dots shown)");
    edit2->set_password_mode(true);
    edit2->set_horizontal_align(onyxui::horizontal_alignment::stretch);
    edit2->set_has_border(true);
    central->add_child(std::move(edit2));

    // Read-only example with pre-filled text
    auto edit3 = std::make_unique<onyxui::line_edit<Backend>>("Read-only text (cannot edit)");
    edit3->set_read_only(true);
    edit3->set_horizontal_align(onyxui::horizontal_alignment::stretch);
    edit3->set_has_border(true);
    central->add_child(std::move(edit3));

    // Spacer
    add_label(*central, "");

    // Text View Section (Scrollable text display)
    add_label(*central, "Scrollable Text View (Arrow keys, PgUp/PgDn, Home/End):");

    // Create text view with demo content
    auto text_view_widget = std::make_unique<onyxui::text_view<Backend>>();
    text_view_widget->set_has_border(true);

    // Generate demo text content
    std::string demo_text =
        "Welcome to OnyxUI Text View Demo!\n"
        "\n"
        "This widget demonstrates:\n"
        "  * Multi-line text display\n"
        "  * Automatic scrolling\n"
        "  * Keyboard navigation:\n"
        "    - Arrow Up/Down: Scroll line by line\n"
        "    - Page Up/Down: Scroll by viewport\n"
        "    - Home: Jump to top\n"
        "    - End: Jump to bottom\n"
        "\n"
        "The text view uses scroll_view internally,\n"
        "so it inherits all scrolling features!\n"
        "\n"
        "Try switching themes (1-4 keys) to see\n"
        "how the scrollbar and text colors adapt.\n"
        "\n"
        "Log Entries:\n";

    // Add simulated log entries to demonstrate scrolling
    for (int i = 1; i <= 15; i++) {
        demo_text += "[LOG " + std::to_string(i) + "] Entry at timestamp " +
                    std::to_string(1000 + i * 100) + " ms\n";
    }

    text_view_widget->set_text(demo_text);

    // Set alignment - stretch horizontally but not vertically
    text_view_widget->set_horizontal_align(onyxui::horizontal_alignment::stretch);
    text_view_widget->set_vertical_align(onyxui::vertical_alignment::center);

    // Constrain height to prevent overlap with menu and other elements
    // NOTE: text_view uses always-visible scrollbars with "classic" style (arrow buttons).
    //   Terminal is typically 25 lines tall (or larger).
    //   With menu bar, labels, buttons, and instructions, we limit text_view height.
    //   - Border: 2px (top + bottom)
    //   - Grid needs: content area + 2 scrollbar rows
    //   - Min size for scrollbars: 8px each (to avoid corruption)
    onyxui::size_constraint height_constraint;
    height_constraint.policy = onyxui::size_policy::content;  // Size based on content
    height_constraint.preferred_size = 8;   // Fit in remaining space
    height_constraint.min_size = 8;         // Minimum: ensure scrollbars can render properly
    height_constraint.max_size = 8;         // Maximum: prevent overflow
    text_view_widget->set_height_constraint(height_constraint);

    // Save pointer before moving
    text_view = text_view_widget.get();
    central->add_child(std::move(text_view_widget));

    // Spacer and instructions
    add_label(*central, "");
    add_label(*central, "Press 1-4 to switch themes | F10 for menu (try File->Open!)");
    add_label(*central, "ESC, Ctrl+C, or Alt+F4 to quit");

    // Set central widget on main_window
    widget->set_central_widget(std::move(central));
}

} // namespace demo_ui_builder
