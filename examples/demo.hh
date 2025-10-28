//
// Created by igor on 21/10/2025.
//

#pragma once
#include <algorithm>
#include <iostream>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/widgets/menu_bar.hh>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/ui_services.hh>


template <onyxui::UIBackend Backend>
class main_widget : public onyxui::panel<Backend> {
public:
    /**
     * @brief Construct the main widget
     *
     * @details
     * Discovers available themes from ui_services theme registry.
     * Sets up UI structure, actions, and hotkeys.
     */
    main_widget()
        : onyxui::panel<Backend>(nullptr)
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
        auto norton_it = std::find(m_theme_names.begin(), m_theme_names.end(), "Norton Blue");
        if (norton_it != m_theme_names.end()) {
            m_current_theme_index = std::distance(m_theme_names.begin(), norton_it);
        } else {
            m_current_theme_index = 0;  // Fallback to first theme if Norton Blue not found
        }

        // Set up layout
        this->set_vbox_layout(0);  // Vertical layout with no spacing for compact DOS look
        this->set_padding(onyxui::thickness::all(0));  // No internal padding for compact DOS look

        // Set up actions and hotkeys (must come before build_menu_bar)
        setup_actions();

        // Build UI structure (including menu bar)
        build_ui();

        // Apply initial theme: Norton Blue (default conio theme)
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

private:
    void build_ui() {
        // Menu bar (at the top)
        build_menu_bar();

        // Title
        add_label(*this, "DOS Theme Showcase - New Theme System");

        // Theme label (keep pointer for updates)
        m_theme_label = add_label(*this, get_current_theme_display());

        // Spacer
        add_label(*this, "");

        // Demo panel with border
        auto* demo_panel = add_panel(*this);
        demo_panel->set_has_border(true);
        demo_panel->set_padding(onyxui::thickness::all(1));
        demo_panel->set_vbox_layout(1);

        add_label(*demo_panel, "Panel with Border");
        add_label(*demo_panel, "Themes via service locator");

        // Spacer
        add_label(*this, "");

        // Button section
        add_label(*this, "Button States:");
        add_button(*this, "Normal");
        add_button(*this, "Focused");

        auto* disabled_btn = add_button(*this, "Disabled");
        disabled_btn->set_enabled(false);

        // Spacer
        add_label(*this, "");

        // Quit button (test mouse interaction!)
        auto* quit_btn = add_button(*this, "Quit");
        quit_btn->set_focusable(true);
        quit_btn->clicked.connect([this]() {
            quit();
        });

        // Spacer and instructions
        add_label(*this, "");
        add_label(*this, "Press 1-4 to switch themes | ESC, Ctrl+C, or Alt+F4 to quit");
    }

    void setup_actions() {
        // Get global hotkey manager from service locator
        auto* hotkeys = onyxui::ui_services<Backend>::hotkeys();
        if (!hotkeys) {
            std::cerr << "Error: No hotkey manager available!" << std::endl;
            return;
        }

        // Theme switching actions (1-4 keys for first 4 themes)
        const size_t max_hotkey_themes = std::min(m_theme_names.size(), size_t(4));
        for (size_t i = 0; i < max_hotkey_themes; ++i) {
            auto theme_action = std::make_shared<onyxui::action<Backend>>();
            theme_action->set_text(m_theme_names[i]);
            theme_action->set_shortcut(static_cast<char>('1' + i));  // 1-4 keys
            theme_action->triggered.connect([this, i]() {
                switch_to_theme_index(i);
            });
            hotkeys->register_action(theme_action);
            m_theme_actions.push_back(theme_action);  // Keep action alive!
        }

        // File menu actions
        auto new_action = std::make_shared<onyxui::action<Backend>>();
        new_action->set_text("New");
        new_action->set_shortcut('n', onyxui::key_modifier::ctrl);  // Ctrl+N
        new_action->triggered.connect([]() {
            std::cerr << "New action triggered via menu/hotkey!" << std::endl;
        });
        m_new_action = new_action;
        hotkeys->register_action(new_action);

        auto open_action = std::make_shared<onyxui::action<Backend>>();
        open_action->set_text("Open");
        open_action->set_shortcut('o', onyxui::key_modifier::ctrl);  // Ctrl+O
        open_action->triggered.connect([]() {
            std::cerr << "Open action triggered!" << std::endl;
        });
        m_open_action = open_action;
        hotkeys->register_action(open_action);

        // Quit action with Alt+F4 shortcut
        auto quit_action = std::make_shared<onyxui::action<Backend>>();
        quit_action->set_text("Quit");
        quit_action->set_shortcut_f(4, onyxui::key_modifier::alt);  // Alt+F4
        quit_action->triggered.connect([this]() {
            std::cerr << "Quit action triggered - exiting application!" << std::endl;
            quit();
        });
        m_quit_action = quit_action;
        hotkeys->register_action(quit_action);

        // Help menu actions
        auto about_action = std::make_shared<onyxui::action<Backend>>();
        about_action->set_text("About");
        about_action->triggered.connect([]() {
            std::cerr << "About: DOS Theme Showcase v2.0 - Theme System Edition" << std::endl;
        });
        m_about_action = about_action;
    }

    void build_menu_bar() {
        // Create menu bar
        auto menu_bar_ptr = std::make_unique<onyxui::menu_bar<Backend>>(this);

        // File menu
        auto file_menu = std::make_unique<onyxui::menu<Backend>>();

        auto new_item = std::make_unique<onyxui::menu_item<Backend>>("New");
        new_item->set_action(m_new_action);
        file_menu->add_item(std::move(new_item));

        auto open_item = std::make_unique<onyxui::menu_item<Backend>>("Open");
        open_item->set_action(m_open_action);
        file_menu->add_item(std::move(open_item));

        file_menu->add_separator();

        auto quit_item = std::make_unique<onyxui::menu_item<Backend>>("Quit");
        quit_item->set_action(m_quit_action);
        file_menu->add_item(std::move(quit_item));

        menu_bar_ptr->add_menu("&File", std::move(file_menu));

        // Theme menu - dynamically built from theme registry
        auto theme_menu = std::make_unique<onyxui::menu<Backend>>();

        for (size_t i = 0; i < m_theme_actions.size(); ++i) {
            auto theme_item = std::make_unique<onyxui::menu_item<Backend>>(m_theme_names[i]);
            theme_item->set_action(m_theme_actions[i]);
            theme_menu->add_item(std::move(theme_item));
        }

        menu_bar_ptr->add_menu("&Theme", std::move(theme_menu));

        // Help menu
        auto help_menu = std::make_unique<onyxui::menu<Backend>>();

        auto about_item = std::make_unique<onyxui::menu_item<Backend>>("About");
        about_item->set_action(m_about_action);
        help_menu->add_item(std::move(about_item));

        menu_bar_ptr->add_menu("&Help", std::move(help_menu));

        // Store pointer before moving
        m_menu_bar = menu_bar_ptr.get();

        // Add menu bar to UI
        this->add_child(std::move(menu_bar_ptr));
    }

    /**
     * @brief Switch to theme by index
     * @param index Theme index in sorted theme list
     */
    void switch_to_theme_index(size_t index) {
        if (index >= m_theme_names.size()) return;

        m_current_theme_index = index;
        apply_theme_by_name(m_theme_names[index]);
        update_theme_display();
    }

    /**
     * @brief Apply theme globally by name
     * @param theme_name Name of theme to apply
     *
     * @details
     * Uses the global theming API: themes.set_current_theme(name).
     * This sets the theme for the entire application (all widgets inherit via CSS).
     */
    void apply_theme_by_name(const std::string& theme_name) {
        auto* themes = onyxui::ui_services<Backend>::themes();
        if (!themes) {
            std::cerr << "Error: Theme registry not available!" << std::endl;
            return;
        }

        // Use the global theming API (sets theme for entire application)
        if (!themes->set_current_theme(theme_name)) {
            std::cerr << "Error: Theme '" << theme_name << "' not found!" << std::endl;
            return;
        }

        // Log theme switch with description
        if (auto* theme = themes->get_theme(theme_name)) {
            std::cerr << "Switched to: " << theme->name;
            if (!theme->description.empty()) {
                std::cerr << " - " << theme->description;
            }
            std::cerr << std::endl;
        }
    }

    /**
     * @brief Update the theme display label
     */
    void update_theme_display() {
        if (m_theme_label) {
            m_theme_label->set_text(get_current_theme_display());
        }
    }

    /**
     * @brief Get current theme display text
     * @return Formatted string showing current theme
     */
    std::string get_current_theme_display() const {
        if (m_current_theme_index >= m_theme_names.size()) {
            return "No theme selected";
        }

        return "Current: " + m_theme_names[m_current_theme_index] +
               " (" + std::to_string(m_current_theme_index + 1) +
               "/" + std::to_string(m_theme_names.size()) + ")";
    }

private:
    std::vector<std::string> m_theme_names;  // Discovered from theme registry
    size_t m_current_theme_index;
    bool m_should_quit;

    onyxui::label<Backend>* m_theme_label = nullptr;
    onyxui::menu_bar<Backend>* m_menu_bar = nullptr;

    // Actions - kept alive as shared_ptrs
    std::shared_ptr<onyxui::action<Backend>> m_new_action;
    std::shared_ptr<onyxui::action<Backend>> m_open_action;
    std::shared_ptr<onyxui::action<Backend>> m_quit_action;
    std::shared_ptr<onyxui::action<Backend>> m_about_action;
    std::vector<std::shared_ptr<onyxui::action<Backend>>> m_theme_actions;  // Keep actions alive!
};