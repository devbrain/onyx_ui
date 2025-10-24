//
// Created by igor on 16/10/2025.
//
// DOS theme showcase using the onyx_ui widget system
//
// This demo demonstrates the full widget rendering pipeline with the new theme system:
// - Theme registry via ui_services (service locator pattern)
// - Backend-provided themes (conio_themes)
// - Theme discovery (list_theme_names)
// - Theme switching by name
// - Proper widget tree with panel, label, button widgets
// - Layout system (vbox for vertical stacking)
// - Two-pass layout (measure then arrange)
// - Rendering via conio_renderer with text attribute support
// - Action and hotkey system for theme switching
// - Clean, ergonomic widget creation API
// - Terminal resize support
//
// Hotkeys:
//   1-4 - Switch between available themes
//   ESC / Ctrl+C / Alt+F4 - Quit
//

#include <thread>

#include "include/onyxui/conio/conio_backend.hh"
#include <onyxui/ui_handle.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/action.hh>
#include <onyxui/widgets/menu_bar.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <termbox2.h>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

using namespace onyxui;
using namespace onyxui::conio;

/**
 * @brief Main widget for DOS theme showcase
 *
 * @details
 * This widget encapsulates the entire theme showcase application:
 * - UI structure (panels, labels, buttons)
 * - Theme management via ui_services (service locator)
 * - Theme discovery and switching
 * - Actions and hotkey bindings
 * - Quit functionality
 *
 * Demonstrates the new theme system:
 * - Themes accessed via ui_services<Backend>::themes()
 * - Theme discovery via list_theme_names()
 * - Theme application by name
 */
class main_widget : public panel<conio_backend> {
public:
    /**
     * @brief Construct the main widget
     *
     * @details
     * Discovers available themes from ui_services theme registry.
     * Sets up UI structure, actions, and hotkeys.
     */
    main_widget()
        : panel<conio_backend>(nullptr)
        , m_current_theme_index(0)
        , m_should_quit(false)
    {
        // Get available themes from service locator
        auto* themes = ui_services<conio_backend>::themes();
        if (themes) {
            m_theme_names = themes->list_theme_names();
        }

        // Ensure we have at least one theme
        if (m_theme_names.empty()) {
            throw std::runtime_error("No themes registered! Call conio_themes::register_default_themes() first.");
        }

        // Set up layout
        set_vbox_layout(0);  // Vertical layout with no spacing for compact DOS look
        set_padding(thickness::all(0));  // No internal padding for compact DOS look

        // Set up actions and hotkeys (must come before build_menu_bar)
        setup_actions();

        // Build UI structure (including menu bar)
        build_ui();

        // Apply initial theme by name
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
        demo_panel->set_padding(thickness::all(1));
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
        auto* hotkeys = ui_services<conio_backend>::hotkeys();
        if (!hotkeys) {
            std::cerr << "Error: No hotkey manager available!" << std::endl;
            return;
        }

        // Theme switching actions (1-4 keys for first 4 themes)
        const size_t max_hotkey_themes = std::min(m_theme_names.size(), size_t(4));
        for (size_t i = 0; i < max_hotkey_themes; ++i) {
            auto theme_action = std::make_shared<onyxui::action<conio_backend>>();
            theme_action->set_text(m_theme_names[i]);
            theme_action->set_shortcut(static_cast<char>('1' + i));  // 1-4 keys
            theme_action->triggered.connect([this, i]() {
                switch_to_theme_index(i);
            });
            hotkeys->register_action(theme_action);
            m_theme_actions.push_back(theme_action);  // Keep action alive!
        }

        // File menu actions
        auto new_action = std::make_shared<onyxui::action<conio_backend>>();
        new_action->set_text("New");
        new_action->set_shortcut('n', key_modifier::ctrl);  // Ctrl+N
        new_action->triggered.connect([]() {
            std::cerr << "New action triggered via menu/hotkey!" << std::endl;
        });
        m_new_action = new_action;
        hotkeys->register_action(new_action);

        auto open_action = std::make_shared<onyxui::action<conio_backend>>();
        open_action->set_text("Open");
        open_action->set_shortcut('o', key_modifier::ctrl);  // Ctrl+O
        open_action->triggered.connect([]() {
            std::cerr << "Open action triggered!" << std::endl;
        });
        m_open_action = open_action;
        hotkeys->register_action(open_action);

        // Quit action with Alt+F4 shortcut
        auto quit_action = std::make_shared<onyxui::action<conio_backend>>();
        quit_action->set_text("Quit");
        quit_action->set_shortcut_f(4, key_modifier::alt);  // Alt+F4
        quit_action->triggered.connect([this]() {
            std::cerr << "Quit action triggered - exiting application!" << std::endl;
            quit();
        });
        m_quit_action = quit_action;
        hotkeys->register_action(quit_action);

        // Help menu actions
        auto about_action = std::make_shared<onyxui::action<conio_backend>>();
        about_action->set_text("About");
        about_action->triggered.connect([]() {
            std::cerr << "About: DOS Theme Showcase v2.0 - Theme System Edition" << std::endl;
        });
        m_about_action = about_action;
    }

    void build_menu_bar() {
        // Create menu bar
        auto menu_bar_ptr = std::make_unique<menu_bar<conio_backend>>(this);

        // File menu
        auto file_menu = std::make_unique<menu<conio_backend>>();

        auto new_item = std::make_unique<menu_item<conio_backend>>("New");
        new_item->set_action(m_new_action);
        file_menu->add_item(std::move(new_item));

        auto open_item = std::make_unique<menu_item<conio_backend>>("Open");
        open_item->set_action(m_open_action);
        file_menu->add_item(std::move(open_item));

        file_menu->add_separator();

        auto quit_item = std::make_unique<menu_item<conio_backend>>("Quit");
        quit_item->set_action(m_quit_action);
        file_menu->add_item(std::move(quit_item));

        menu_bar_ptr->add_menu("&File", std::move(file_menu));

        // Theme menu - dynamically built from theme registry
        auto theme_menu = std::make_unique<menu<conio_backend>>();

        for (size_t i = 0; i < m_theme_actions.size(); ++i) {
            auto theme_item = std::make_unique<menu_item<conio_backend>>(m_theme_names[i]);
            theme_item->set_action(m_theme_actions[i]);
            theme_menu->add_item(std::move(theme_item));
        }

        menu_bar_ptr->add_menu("&Theme", std::move(theme_menu));

        // Help menu
        auto help_menu = std::make_unique<menu<conio_backend>>();

        auto about_item = std::make_unique<menu_item<conio_backend>>("About");
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
     * @brief Apply theme by name using theme registry
     * @param theme_name Name of theme to apply
     *
     * @details
     * Uses the v2.0 three-way theme API with by-name registration.
     * This is the recommended approach: zero overhead, registry owns the theme.
     */
    void apply_theme_by_name(const std::string& theme_name) {
        auto* themes = ui_services<conio_backend>::themes();
        if (!themes) {
            std::cerr << "Error: Theme registry not available!" << std::endl;
            return;
        }

        // Use the v2.0 by-name API (recommended)
        if (this->apply_theme(theme_name, *themes)) {
            // Theme applied successfully
            if (auto* theme = themes->get_theme(theme_name)) {
                // Log theme switch with description
                std::cerr << "Switched to: " << theme->name;
                if (!theme->description.empty()) {
                    std::cerr << " - " << theme->description;
                }
                std::cerr << std::endl;
            }
        } else {
            std::cerr << "Error: Theme '" << theme_name << "' not found!" << std::endl;
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

    label<conio_backend>* m_theme_label = nullptr;
    menu_bar<conio_backend>* m_menu_bar = nullptr;

    // Actions - kept alive as shared_ptrs
    std::shared_ptr<onyxui::action<conio_backend>> m_new_action;
    std::shared_ptr<onyxui::action<conio_backend>> m_open_action;
    std::shared_ptr<onyxui::action<conio_backend>> m_quit_action;
    std::shared_ptr<onyxui::action<conio_backend>> m_about_action;
    std::vector<std::shared_ptr<onyxui::action<conio_backend>>> m_theme_actions;  // Keep actions alive!
};

static void wait_for_debug() {
    volatile bool flag = false;
    while (!flag) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    try {
        // 1. Create UI context (provides layer manager, focus manager, theme registry)
        //    Themes are automatically registered by conio_backend::register_themes()
        scoped_ui_context<conio_backend> ui_ctx;

        // 2. Display available themes (auto-registered by backend)
        std::cerr << "Available themes:" << std::endl;
        for (const auto& name : ui_ctx.themes().list_theme_names()) {
            if (const auto* theme = ui_ctx.themes().get_theme(name)) {
                std::cerr << "  - " << theme->name;
                if (!theme->description.empty()) {
                    std::cerr << " (" << theme->description << ")";
                }
                std::cerr << std::endl;
            }
        }
        std::cerr << std::endl;

        // 3. Create vram instance (handles termbox2 initialization)



        // 5. Create main widget (discovers themes from ui_services)
        auto widget = std::make_unique<main_widget>();

        // Keep reference to widget for event handling before moving it
        auto* widget_ptr = widget.get();

        // 6. Create UI handle with main widget and renderer
        ui_handle<conio_backend> ui(std::move(widget));

       // wait_for_debug();

        // 8. Main loop - event-driven design
        while (!widget_ptr->should_quit()) {
            ui.display();
            ui.present();
            // Wait for event (blocking)
            tb_event ev;
            int poll_result = conio_poll_event(&ev);

            // Handle polling errors gracefully
            if (poll_result != TB_OK) {
                continue;  // Next poll will have the actual event
            }

            // Handle Ctrl+C specially (always quit)
            if (ev.type == TB_EVENT_KEY && ev.ch == 'c' && (ev.mod & TB_MOD_CTRL)) {
                widget_ptr->quit();
                continue;
            }

            // Route all events through ui_handle's event system
            bool handled = ui.handle_event(ev);

            // If ESC wasn't handled by UI (no menu open), quit
            if (ev.type == TB_EVENT_KEY && ev.key == TB_KEY_ESC && !handled) {
                widget_ptr->quit();
                continue;
            }
        }

        // Context automatically cleaned up on scope exit
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;

        // Context automatically cleaned up on scope exit
        return 1;
    }
}
