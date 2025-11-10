//
// Created by igor on 21/10/2025.
//
// Utility functions for main_widget demo
// Provides theme management and display helpers
//

#pragma once

namespace demo_utils {

/**
 * @brief Get current theme display text
 * @param current_theme_index Current theme index
 * @param theme_names List of available theme names
 * @return Formatted string showing current theme
 */
template<typename Backend>
std::string get_current_theme_display(size_t current_theme_index,
                                       const std::vector<std::string>& theme_names) {
    if (current_theme_index >= theme_names.size()) {
        return "No theme selected";
    }

    return "Current: " + theme_names[current_theme_index] +
           " (" + std::to_string(current_theme_index + 1) +
           "/" + std::to_string(theme_names.size()) + ")";
}

/**
 * @brief Apply theme globally by name
 * @param theme_name Name of theme to apply
 *
 * @details
 * Uses the global theming API: themes.set_current_theme(name).
 * This sets the theme for the entire application (all widgets inherit via CSS).
 */
template<typename Backend>
void apply_theme_by_name(const std::string& theme_name) {
    auto* themes = onyxui::ui_services<Backend>::themes();
    if (!themes) {
        return;
    }

    // Use the global theming API (sets theme for entire application)
    themes->set_current_theme(theme_name);
}

/**
 * @brief Update the theme display label
 * @param theme_label Pointer to the label widget to update
 * @param current_theme_index Current theme index
 * @param theme_names List of available theme names
 */
template<typename Backend>
void update_theme_display(onyxui::label<Backend>* theme_label,
                          size_t current_theme_index,
                          const std::vector<std::string>& theme_names) {
    if (theme_label) {
        theme_label->set_text(get_current_theme_display<Backend>(current_theme_index, theme_names));
    }
}

/**
 * @brief Switch to theme by index
 * @param index Theme index in sorted theme list
 * @param current_theme_index Reference to current theme index (will be updated)
 * @param theme_names List of available theme names
 * @param theme_label Pointer to the label widget to update
 */
template<typename Backend>
void switch_to_theme_index(size_t index,
                           size_t& current_theme_index,
                           const std::vector<std::string>& theme_names,
                           onyxui::label<Backend>* theme_label) {
    if (index >= theme_names.size()) return;

    current_theme_index = index;
    apply_theme_by_name<Backend>(theme_names[index]);
    update_theme_display<Backend>(theme_label, current_theme_index, theme_names);
}

} // namespace demo_utils
