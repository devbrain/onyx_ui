//
// Created by igor on 21/10/2025.
//
// Action setup for main_widget demo
// Registers hotkeys and creates actions for theme switching, file operations, and quit
//

#pragma once
#include <onyxui/actions/action.hh>
#include <onyxui/services/ui_services.hh>

namespace demo_actions {

/**
 * @brief Set up all actions and hotkeys for the demo
 *
 * @details
 * Creates and registers:
 * - Theme switching actions (1-4 keys for first 4 themes)
 * - File menu actions (New, Open, Quit)
 * - Help menu actions (About)
 *
 * @tparam Backend UI backend type
 * @tparam Widget The main widget type (must have quit() method and theme members)
 * @param widget Pointer to the main widget instance
 * @param theme_names List of available theme names
 * @param theme_actions Vector to store theme action shared pointers (keeps them alive)
 * @param new_action Shared pointer to store New action
 * @param open_action Shared pointer to store Open action
 * @param quit_action Shared pointer to store Quit action
 * @param about_action Shared pointer to store About action
 */
template<typename Backend, typename Widget>
void setup_actions(Widget* widget,
                   const std::vector<std::string>& theme_names,
                   std::vector<std::shared_ptr<onyxui::action<Backend>>>& theme_actions,
                   std::shared_ptr<onyxui::action<Backend>>& new_action,
                   std::shared_ptr<onyxui::action<Backend>>& open_action,
                   std::shared_ptr<onyxui::action<Backend>>& quit_action,
                   std::shared_ptr<onyxui::action<Backend>>& about_action) {
    // Get global hotkey manager from service locator
    auto* hotkeys = onyxui::ui_services<Backend>::hotkeys();
    if (!hotkeys) {
        return;
    }

    // Theme switching actions (1-4 keys for first 4 themes)
    const size_t max_hotkey_themes = std::min(theme_names.size(), size_t(4));
    for (size_t i = 0; i < max_hotkey_themes; ++i) {
        auto theme_action = std::make_shared<onyxui::action<Backend>>();
        theme_action->set_text(theme_names[i]);
        theme_action->set_shortcut(static_cast<char>('1' + i));  // 1-4 keys
        theme_action->triggered.connect([widget, i]() {
            widget->switch_to_theme_index(i);
        });
        hotkeys->register_action(theme_action);
        theme_actions.push_back(theme_action);  // Keep action alive!
    }

    // File menu actions
    auto new_act = std::make_shared<onyxui::action<Backend>>();
    new_act->set_text("New");
    new_act->set_shortcut('n', onyxui::key_modifier::ctrl);  // Ctrl+N
    new_act->triggered.connect([]() {
        // New action triggered
    });
    new_action = new_act;
    hotkeys->register_action(new_act);

    auto open_act = std::make_shared<onyxui::action<Backend>>();
    open_act->set_text("Open");
    open_act->set_shortcut('o', onyxui::key_modifier::ctrl);  // Ctrl+O
    open_act->triggered.connect([]() {
        // Open action triggered
    });
    open_action = open_act;
    hotkeys->register_action(open_act);

    // Quit action with Alt+F4 shortcut
    auto quit_act = std::make_shared<onyxui::action<Backend>>();
    quit_act->set_text("Quit");
    quit_act->set_shortcut_f(4, onyxui::key_modifier::alt);  // Alt+F4
    quit_act->triggered.connect([widget]() {
        widget->quit();
    });
    quit_action = quit_act;
    hotkeys->register_action(quit_act);

    // Help menu actions
    auto about_act = std::make_shared<onyxui::action<Backend>>();
    about_act->set_text("About");
    about_act->triggered.connect([]() {
        // About: DOS Theme Showcase v2.0 - Theme System Edition
    });
    about_action = about_act;
}

} // namespace demo_actions
