//
// Created by Claude Code on 2025-11-23.
//
// OnyxUI Widgets Demo - Main Application Class
// Comprehensive demonstration of all OnyxUI framework features
//

#pragma once
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <onyxui/widgets/main_window.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/actions/action.hh>

// Tab implementations
#include "tabs/tab_all_widgets.hh"

/**
 * @brief Main application class for OnyxUI Widgets Demo
 *
 * @details
 * Demonstrates:
 * - Complete widget gallery (all widgets, all states)
 * - Layout and scrolling systems
 * - Event system, focus management, hotkeys
 * - Multi-window management (MVC demo, theme editor, debug tools)
 * - Layer management (tooltips, popups, modals)
 * - Theme switching
 * - Screenshot functionality
 * - Performance metrics
 */
template <onyxui::UIBackend Backend>
class widgets_demo : public onyxui::main_window<Backend> {
public:
    /**
     * @brief Construct the widgets demo application
     */
    widgets_demo()
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

        // Find Norton Blue theme index (default theme)
        auto norton_it = std::find(m_theme_names.begin(), m_theme_names.end(), "NU8");
        if (norton_it != m_theme_names.end()) {
            m_current_theme_index = static_cast<std::size_t>(std::distance(m_theme_names.begin(), norton_it));
        } else {
            m_current_theme_index = 0;  // Fallback to first theme
        }

        // Set up actions and hotkeys
        setup_actions();

        // Build UI structure (menu bar, tabs, status bar)
        build_ui();

        // Apply initial theme
        apply_theme_by_name(m_theme_names[m_current_theme_index]);
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
     * @brief Take a screenshot and save to file
     * @param filename Output filename (default: screenshot_<timestamp>.txt)
     */
    void take_screenshot(const std::string& filename = "") {
        if (!m_renderer) {
            std::cerr << "Cannot take screenshot: renderer not set\n";
            return;
        }

        // Generate filename with timestamp if not provided
        std::string output_file = filename;
        if (output_file.empty()) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()
            ).count();
            output_file = "screenshot_" + std::to_string(timestamp) + ".txt";
        }

        // Take screenshot
        std::ofstream file(output_file);
        if (!file) {
            std::cerr << "Failed to open file: " << output_file << "\n";
            return;
        }

        m_renderer->take_screenshot(file);
        std::cout << "Screenshot saved to: " << output_file << "\n";
    }

    /**
     * @brief Switch to theme by index
     * @param index Theme index in sorted theme list
     */
    void switch_to_theme_index(size_t index) {
        if (index >= m_theme_names.size()) {
            return;
        }

        m_current_theme_index = index;
        apply_theme_by_name(m_theme_names[index]);

        // TODO: Update status bar when theme changes
    }

    /**
     * @brief Show MVC Demo window (Ctrl+M)
     */
    void show_mvc_demo() {
        std::cout << "MVC Demo window - Coming soon!\n";
        // TODO: Implement mvc_demo_window
    }

    /**
     * @brief Show Theme Editor window (Ctrl+T)
     */
    void show_theme_editor() {
        std::cout << "Theme Editor window - Coming soon!\n";
        // TODO: Implement theme_editor_window
    }

    /**
     * @brief Show Debug Tools window (F12)
     */
    void show_debug_tools() {
        std::cout << "Debug Tools window - Coming soon!\n";
        // TODO: Implement debug_tools_window
    }

private:
    /**
     * @brief Apply theme by name to the entire UI
     */
    void apply_theme_by_name(const std::string& theme_name) {
        auto* themes = onyxui::ui_services<Backend>::themes();
        if (!themes) {
            return;
        }

        // Use the global theming API (sets theme for entire application)
        themes->set_current_theme(theme_name);
    }

    /**
     * @brief Set up all actions and hotkeys
     */
    void setup_actions() {
        auto* hotkeys = onyxui::ui_services<Backend>::hotkeys();
        if (!hotkeys) {
            return;
        }

        // Screenshot actions (F9 and Ctrl+S)
        auto screenshot_f9 = std::make_shared<onyxui::action<Backend>>();
        screenshot_f9->set_text("Screenshot");
        screenshot_f9->set_shortcut_f(9);  // F9
        screenshot_f9->triggered.connect([this]() {
            take_screenshot();
        });
        hotkeys->register_action(screenshot_f9);
        m_actions.push_back(screenshot_f9);

        auto screenshot_ctrl_s = std::make_shared<onyxui::action<Backend>>();
        screenshot_ctrl_s->set_text("Save Screenshot");
        screenshot_ctrl_s->set_shortcut('s', onyxui::key_modifier::ctrl);  // Ctrl+S
        screenshot_ctrl_s->triggered.connect([this]() {
            take_screenshot();
        });
        hotkeys->register_action(screenshot_ctrl_s);
        m_actions.push_back(screenshot_ctrl_s);

        // Window spawning actions
        auto mvc_action = std::make_shared<onyxui::action<Backend>>();
        mvc_action->set_text("MVC Demo");
        mvc_action->set_shortcut('m', onyxui::key_modifier::ctrl);  // Ctrl+M
        mvc_action->triggered.connect([this]() {
            show_mvc_demo();
        });
        hotkeys->register_action(mvc_action);
        m_actions.push_back(mvc_action);

        auto theme_editor_action = std::make_shared<onyxui::action<Backend>>();
        theme_editor_action->set_text("Theme Editor");
        theme_editor_action->set_shortcut('t', onyxui::key_modifier::ctrl);  // Ctrl+T
        theme_editor_action->triggered.connect([this]() {
            show_theme_editor();
        });
        hotkeys->register_action(theme_editor_action);
        m_actions.push_back(theme_editor_action);

        auto debug_tools_action = std::make_shared<onyxui::action<Backend>>();
        debug_tools_action->set_text("Debug Tools");
        debug_tools_action->set_shortcut_f(12);  // F12
        debug_tools_action->triggered.connect([this]() {
            show_debug_tools();
        });
        hotkeys->register_action(debug_tools_action);
        m_actions.push_back(debug_tools_action);

        // Quit action (Alt+F4)
        auto quit_action = std::make_shared<onyxui::action<Backend>>();
        quit_action->set_text("Quit");
        quit_action->set_shortcut_f(4, onyxui::key_modifier::alt);  // Alt+F4
        quit_action->triggered.connect([this]() {
            quit();
        });
        hotkeys->register_action(quit_action);
        m_actions.push_back(quit_action);
    }

    /**
     * @brief Build the complete UI structure
     */
    void build_ui() {
        // Build menu bar
        build_menu_bar();

        // Build central content (tab widget with 3 tabs)
        build_tabs();

        // Build status bar
        build_status_bar();
    }

    /**
     * @brief Build the menu bar
     */
    void build_menu_bar() {
        auto menu_bar_widget = std::make_unique<onyxui::menu_bar<Backend>>();

        // File menu
        auto file_menu = std::make_unique<onyxui::menu<Backend>>();

        auto screenshot_item = std::make_unique<onyxui::menu_item<Backend>>("");
        screenshot_item->set_mnemonic_text("&Save Screenshot\\tCtrl+S");
        screenshot_item->clicked.connect([this]() {
            take_screenshot();
        });
        file_menu->add_item(std::move(screenshot_item));

        file_menu->add_separator();

        auto exit_item = std::make_unique<onyxui::menu_item<Backend>>("");
        exit_item->set_mnemonic_text("E&xit\\tAlt+F4");
        exit_item->clicked.connect([this]() {
            quit();
        });
        file_menu->add_item(std::move(exit_item));

        menu_bar_widget->add_menu("&File", std::move(file_menu));

        // Windows menu
        auto windows_menu = std::make_unique<onyxui::menu<Backend>>();

        auto mvc_item = std::make_unique<onyxui::menu_item<Backend>>("");
        mvc_item->set_mnemonic_text("&MVC Demo...\\tCtrl+M");
        mvc_item->clicked.connect([this]() {
            show_mvc_demo();
        });
        windows_menu->add_item(std::move(mvc_item));

        auto theme_editor_item = std::make_unique<onyxui::menu_item<Backend>>("");
        theme_editor_item->set_mnemonic_text("&Theme Editor...\\tCtrl+T");
        theme_editor_item->clicked.connect([this]() {
            show_theme_editor();
        });
        windows_menu->add_item(std::move(theme_editor_item));

        auto debug_item = std::make_unique<onyxui::menu_item<Backend>>("");
        debug_item->set_mnemonic_text("&Debug Tools...\\tF12");
        debug_item->clicked.connect([this]() {
            show_debug_tools();
        });
        windows_menu->add_item(std::move(debug_item));

        windows_menu->add_separator();

        auto modal_item = std::make_unique<onyxui::menu_item<Backend>>("");
        modal_item->set_mnemonic_text("M&odal Dialog Example...");
        modal_item->clicked.connect([this]() {
            show_modal_dialog();
        });
        windows_menu->add_item(std::move(modal_item));

        auto modeless_item = std::make_unique<onyxui::menu_item<Backend>>("");
        modeless_item->set_mnemonic_text("Mode&less Dialog Example...");
        modeless_item->clicked.connect([this]() {
            show_modeless_dialog();
        });
        windows_menu->add_item(std::move(modeless_item));

        menu_bar_widget->add_menu("&Windows", std::move(windows_menu));

        // Theme menu
        auto theme_menu = std::make_unique<onyxui::menu<Backend>>();

        for (size_t i = 0; i < m_theme_names.size(); ++i) {
            const auto& theme_name = m_theme_names[i];
            auto theme_item = std::make_unique<onyxui::menu_item<Backend>>(theme_name);
            theme_item->clicked.connect([this, i]() {
                switch_to_theme_index(i);
            });
            theme_menu->add_item(std::move(theme_item));
        }

        menu_bar_widget->add_menu("&Theme", std::move(theme_menu));

        // Help menu
        auto help_menu = std::make_unique<onyxui::menu<Backend>>();

        auto about_item = std::make_unique<onyxui::menu_item<Backend>>("");
        about_item->set_mnemonic_text("&About OnyxUI...");
        about_item->clicked.connect([this]() {
            show_about_dialog();
        });
        help_menu->add_item(std::move(about_item));

        menu_bar_widget->add_menu("&Help", std::move(help_menu));

        // Set menu bar on main_window
        this->set_menu_bar(std::move(menu_bar_widget));
    }

    /**
     * @brief Build the tab widget with 3 tabs
     */
    void build_tabs() {
        auto* central = this->central_widget();
        if (!central) {
            return;
        }

        // Create tab widget
        m_tab_widget = central->template emplace_child<onyxui::tab_widget>();

        // Tab 1: All Widgets (complete implementation)
        auto tab1 = widgets_demo_tabs::create_tab_all_widgets<Backend>(this);
        m_tab_widget->add_tab(std::move(tab1), "All Widgets");

        // Tab 2: Layout & Scrolling (placeholder)
        auto tab2 = std::make_unique<onyxui::panel<Backend>>();
        auto* tab2_label = tab2->template emplace_child<onyxui::label>("Tab 2: Layout & Scrolling - Coming soon!");
        tab2_label->set_horizontal_align(onyxui::horizontal_alignment::center);
        tab2_label->set_vertical_align(onyxui::vertical_alignment::center);
        m_tab_widget->add_tab(std::move(tab2), "Layout & Scrolling");

        // Tab 3: Events & Interaction (placeholder)
        auto tab3 = std::make_unique<onyxui::panel<Backend>>();
        auto* tab3_label = tab3->template emplace_child<onyxui::label>("Tab 3: Events & Interaction - Coming soon!");
        tab3_label->set_horizontal_align(onyxui::horizontal_alignment::center);
        tab3_label->set_vertical_align(onyxui::vertical_alignment::center);
        m_tab_widget->add_tab(std::move(tab3), "Events & Interaction");

        // Set default tab
        m_tab_widget->set_current_index(0);
    }

    /**
     * @brief Build the status bar
     */
    void build_status_bar() {
        auto status_bar_widget = std::make_unique<onyxui::status_bar<Backend>>();

        // Set initial status text (left side)
        std::string status_text = "Theme: " + m_theme_names[m_current_theme_index] +
                                  " | FPS: -- | Focus: -- | Widgets: --";
        status_bar_widget->set_left_text(status_text);

        // Set right side text
        status_bar_widget->set_right_text("OnyxUI Widgets Demo v1.0");

        // Set status bar on main_window
        this->set_status_bar(std::move(status_bar_widget));
    }

    /**
     * @brief Show Modal Dialog Example
     */
    void show_modal_dialog() {
        std::cout << "Modal Dialog Example - Coming soon!\n";
        // TODO: Implement modal dialog
    }

    /**
     * @brief Show Modeless Dialog Example
     */
    void show_modeless_dialog() {
        std::cout << "Modeless Dialog Example - Coming soon!\n";
        // TODO: Implement modeless dialog
    }

    /**
     * @brief Show About Dialog
     */
    void show_about_dialog() {
        std::cout << "About OnyxUI - Widgets Demo v1.0\n";
        std::cout << "Comprehensive framework demonstration\n";
        // TODO: Implement about dialog
    }

private:
    std::vector<std::string> m_theme_names;  // Discovered from theme registry
    size_t m_current_theme_index;
    bool m_should_quit;

    onyxui::tab_widget<Backend>* m_tab_widget = nullptr;
    typename Backend::renderer_type* m_renderer = nullptr;  // For screenshots

    // Actions - kept alive as shared_ptrs
    std::vector<std::shared_ptr<onyxui::action<Backend>>> m_actions;
};
