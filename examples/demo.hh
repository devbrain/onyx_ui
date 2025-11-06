//
// Created by igor on 21/10/2025.
//

#pragma once
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/containers/scroll/scroll_view_presets.hh>
#include <onyxui/services/ui_services.hh>


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
        auto norton_it = std::find(m_theme_names.begin(), m_theme_names.end(), "NU8");
        if (norton_it != m_theme_names.end()) {
            m_current_theme_index = static_cast<std::size_t>(std::distance(m_theme_names.begin(), norton_it));
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

        // Note: text_view will gain focus when clicked with mouse
        if (m_text_view) {
            std::cerr << "[demo] text_view created at: " << m_text_view << "\n";
            std::cerr << "[demo] Click on the text_view to give it focus for keyboard scrolling\n";
        }
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

        auto* normal_btn = add_button(*this, "Normal");
        normal_btn->set_horizontal_align(onyxui::horizontal_alignment::left);

        auto* screenshot_btn = add_button(*this, "Screenshot");
        screenshot_btn->set_horizontal_align(onyxui::horizontal_alignment::left);
        screenshot_btn->clicked.connect([this]() {
            if (!m_renderer) {
                std::cerr << "Error: No renderer available!" << std::endl;
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
                std::cerr << "Error: Could not open file for screenshot: " << filename << std::endl;
                return;
            }

            m_renderer->take_screenshot(file);
            file.close();

            std::cerr << "Screenshot saved to: " << filename << std::endl;
        });

        auto* disabled_btn = add_button(*this, "Disabled");
        disabled_btn->set_enabled(false);
        disabled_btn->set_horizontal_align(onyxui::horizontal_alignment::left);

        // Spacer
        add_label(*this, "");

        // Quit button (test mouse interaction!)
        auto* quit_btn = add_button(*this, "Quit");
        quit_btn->set_focusable(true);
        quit_btn->set_horizontal_align(onyxui::horizontal_alignment::left);
        quit_btn->clicked.connect([this]() {
            quit();
        });

        // Spacer
        add_label(*this, "");

        // Text View Section (Scrollable text display)
        add_label(*this, "Scrollable Text View (Arrow keys, PgUp/PgDn, Home/End):");

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
        //   For 80-line terminal with text_view starting around line 20-21:
        //   - Available space: ~26-28 lines for text_view
        //   - Border: 2px (top + bottom)
        //   - Grid needs: content area + 2 scrollbar rows
        //   - Min size for scrollbars: 8px each (to avoid corruption)
        onyxui::size_constraint height_constraint;
        height_constraint.policy = onyxui::size_policy::content;  // Size based on content
        height_constraint.preferred_size = 20;  // Preferred height
        height_constraint.min_size = 10;        // Minimum: border (2) + scrollbar min_render_size (8)
        height_constraint.max_size = 28;        // Maximum: reasonable size for 55-line terminal
        text_view_widget->set_height_constraint(height_constraint);

        // Save pointer before moving
        m_text_view = text_view_widget.get();
        this->add_child(std::move(text_view_widget));

        // Spacer and instructions
        add_label(*this, "");
        add_label(*this, "Press 1-4 to switch themes | F10 for menu (try File->Open!)");
        add_label(*this, "ESC, Ctrl+C, or Alt+F4 to quit");
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

        // Phase 5: Open submenu - demonstrates cascading menu navigation!
        auto open_item = std::make_unique<onyxui::menu_item<Backend>>("Open");

        // Create Open submenu with multiple options
        auto open_submenu = std::make_unique<onyxui::menu<Backend>>();

        // Open File option (using existing open_action)
        auto open_file_item = std::make_unique<onyxui::menu_item<Backend>>("");
        open_file_item->set_mnemonic_text("Open &File...");
        open_file_item->set_action(m_open_action);  // Ctrl+O
        open_submenu->add_item(std::move(open_file_item));

        // Open Folder option
        auto open_folder_item = std::make_unique<onyxui::menu_item<Backend>>("");
        open_folder_item->set_mnemonic_text("Open F&older...");
        open_folder_item->clicked.connect([]() {
            std::cerr << "Open Folder clicked!" << std::endl;
        });
        open_submenu->add_item(std::move(open_folder_item));

        open_submenu->add_separator();

        // Recent Files submenu (nested submenu - demonstrates arbitrary depth!)
        auto recent_item = std::make_unique<onyxui::menu_item<Backend>>("");
        recent_item->set_mnemonic_text("&Recent Files");

        auto recent_submenu = std::make_unique<onyxui::menu<Backend>>();
        auto recent1 = std::make_unique<onyxui::menu_item<Backend>>("demo.cc");
        recent1->clicked.connect([]() {
            std::cerr << "Opening recent file: demo.cc" << std::endl;
        });
        recent_submenu->add_item(std::move(recent1));

        auto recent2 = std::make_unique<onyxui::menu_item<Backend>>("main.cc");
        recent2->clicked.connect([]() {
            std::cerr << "Opening recent file: main.cc" << std::endl;
        });
        recent_submenu->add_item(std::move(recent2));

        auto recent3 = std::make_unique<onyxui::menu_item<Backend>>("test.cc");
        recent3->clicked.connect([]() {
            std::cerr << "Opening recent file: test.cc" << std::endl;
        });
        recent_submenu->add_item(std::move(recent3));

        // Attach Recent Files submenu to Recent item
        recent_item->set_submenu(std::move(recent_submenu));
        open_submenu->add_item(std::move(recent_item));

        // Attach Open submenu to Open item
        open_item->set_submenu(std::move(open_submenu));
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
    onyxui::text_view<Backend>* m_text_view = nullptr;  // For giving focus
    typename Backend::renderer_type* m_renderer = nullptr;  // For screenshots

    // Actions - kept alive as shared_ptrs
    std::shared_ptr<onyxui::action<Backend>> m_new_action;
    std::shared_ptr<onyxui::action<Backend>> m_open_action;
    std::shared_ptr<onyxui::action<Backend>> m_quit_action;
    std::shared_ptr<onyxui::action<Backend>> m_about_action;
    std::vector<std::shared_ptr<onyxui::action<Backend>>> m_theme_actions;  // Keep actions alive!
};