//
// Created by igor on 21/10/2025.
//
// Main widget for OnyxUI demo application
// Demonstrates theme switching, menus, buttons, panels, and text view
//

#pragma once
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>
#include <onyxui/widgets/main_window.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/services/ui_services.hh>

// Helper modules (extracted from this file for better organization)
#include "demo_utils.hh"
#include "demo_actions.hh"
#include "demo_windows.hh"
#include "demo_ui_builder.hh"

template <onyxui::UIBackend Backend>
class main_widget : public onyxui::main_window<Backend> {
public:
    /**
     * @brief Construct the main widget
     *
     * @details
     * Discovers available themes from ui_services theme registry.
     * Sets up UI structure, actions, and hotkeys.
     */
    main_widget()
        : onyxui::main_window<Backend>()
        , m_current_theme_index(0)
        , m_should_quit(false)
    {
        // Get available themes from service locator
        auto* themes = onyxui::ui_services<Backend>::themes();
        if (themes) {
            m_theme_names = themes->list_theme_names();
        }

        // Ensure we have at least one theme
        if (m_theme_names.empty()) {
            throw std::runtime_error("No themes registered! Call conio_themes::register_default_themes() first.");
        }

        // Find Norton Blue theme index (default theme, registered first by conio_backend)
        auto norton_it = std::find(m_theme_names.begin(), m_theme_names.end(), "NU8");
        if (norton_it != m_theme_names.end()) {
            m_current_theme_index = static_cast<std::size_t>(std::distance(m_theme_names.begin(), norton_it));
        } else {
            m_current_theme_index = 0;  // Fallback to first theme if Norton Blue not found
        }

        // main_window handles layout - no need for set_vbox_layout or set_padding

        // Set up actions and hotkeys (must come before build_menu_bar)
        setup_actions();

        // Build UI structure (including menu bar)
        build_ui();

        // Apply initial theme: Norton Blue (default conio theme)
        demo_utils::apply_theme_by_name<Backend>(m_theme_names[m_current_theme_index]);

        // Note: text_view will gain focus when clicked with mouse
    }

    /**
     * @brief Check if application should quit
     */
    bool should_quit() const noexcept {
        return m_should_quit;
    }

    /**
     * @brief Trigger quit action
     */
    void quit() {
        m_should_quit = true;
    }

    /**
     * @brief Set renderer for screenshot functionality
     * @param renderer Pointer to the renderer
     */
    void set_renderer(typename Backend::renderer_type* renderer) {
        m_renderer = renderer;
    }

    /**
     * @brief Get renderer for screenshot functionality
     * @return Pointer to the renderer (may be nullptr)
     */
    typename Backend::renderer_type* get_renderer() const noexcept {
        return m_renderer;
    }

    /**
     * @brief Switch to theme by index
     * @param index Theme index in sorted theme list
     */
    void switch_to_theme_index(size_t index) {
        demo_utils::switch_to_theme_index<Backend>(
            index,
            m_current_theme_index,
            m_theme_names,
            m_theme_label
        );
    }

private:
    /**
     * @brief Set up all actions and hotkeys
     */
    void setup_actions() {
        demo_actions::setup_actions<Backend>(
            this,
            m_theme_names,
            m_theme_actions,
            m_new_action,
            m_open_action,
            m_quit_action,
            m_about_action
        );
    }

    /**
     * @brief Build the complete UI structure
     */
    void build_ui() {
        demo_ui_builder::build_ui<Backend>(
            this,
            m_theme_names,
            m_current_theme_index,
            m_theme_actions,
            m_new_action,
            m_open_action,
            m_quit_action,
            m_about_action,
            m_renderer,
            m_theme_label,
            m_menu_bar,
            m_text_view
        );
    }

private:
    std::vector<std::string> m_theme_names;  // Discovered from theme registry
    size_t m_current_theme_index;
    bool m_should_quit;

    onyxui::label<Backend>* m_theme_label = nullptr;
    onyxui::menu_bar<Backend>* m_menu_bar = nullptr;
    onyxui::text_view<Backend>* m_text_view = nullptr;  // For giving focus
    typename Backend::renderer_type* m_renderer = nullptr;  // For screenshots

    // Actions - kept alive as shared_ptrs
    std::shared_ptr<onyxui::action<Backend>> m_new_action;
    std::shared_ptr<onyxui::action<Backend>> m_open_action;
    std::shared_ptr<onyxui::action<Backend>> m_quit_action;
    std::shared_ptr<onyxui::action<Backend>> m_about_action;
    std::vector<std::shared_ptr<onyxui::action<Backend>>> m_theme_actions;  // Keep actions alive!
};
