//
// Created by igor on 16/10/2025.
//
// DOS theme showcase using the onyx_ui widget system
//
// This demo demonstrates the full widget rendering pipeline:
// - Proper widget tree with panel, label, button widgets
// - Layout system (vbox for vertical stacking)
// - Two-pass layout (measure then arrange)
// - Rendering via conio_renderer with text attribute support
// - Action and hotkey system for theme switching
// - Backend-agnostic UI creation with templated function
// - Clean, ergonomic widget creation API
// - Terminal resize support
//
// Hotkeys:
//   1 - Switch to DOS Blue theme (Classic)
//   2 - Switch to DOS Dark theme (Norton Commander)
//   3 - Switch to DOS Monochrome theme (Hercules)
//   4 - Switch to Norton Utilities theme (Amber)
//   ESC / Ctrl+C - Quit

#include "dos_theme.hh"
#include "vram.hh"
#include "conio_backend.hh"
#include <onyxui/ui_handle.hh>
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

// Compile-time check: does tb_event satisfy WindowEvent?
static_assert(WindowEvent<tb_event>, "tb_event must satisfy WindowEvent concept");

/**
 * @brief Main widget for DOS theme showcase
 *
 * @details
 * This widget encapsulates the entire theme showcase application:
 * - UI structure (panels, labels, buttons)
 * - Theme management and switching
 * - Actions and hotkey bindings
 * - Quit functionality
 *
 * This demonstrates the pattern of encapsulating application logic
 * in a widget class rather than in main().
 */
class main_widget : public panel<conio_backend> {
public:
    /**
     * @brief Construct the main widget with themes
     * @param themes Available themes
     * @param theme_names Human-readable theme names
     * @param initial_theme Index of initial theme to apply
     */
    main_widget(std::vector<ui_theme<conio_backend>> themes,
                std::vector<std::string> theme_names,
                size_t initial_theme = 0)
        : panel<conio_backend>(nullptr)
        , m_themes(std::move(themes))
        , m_theme_names(std::move(theme_names))
        , m_current_theme(initial_theme)
        , m_should_quit(false)
    {
        // Set up layout
        set_vbox_layout(0);  // Vertical layout with no spacing for compact DOS look
        set_padding(thickness::all(0));  // No internal padding for compact DOS look

        // Set up actions and hotkeys (must come before build_menu_bar)
        setup_actions();

        // Build UI structure (including menu bar)
        build_ui();

        // Apply initial theme
        apply_theme(m_themes[m_current_theme]);
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
     * @brief Set UI handle reference for menu bar
     * @param ui Pointer to UI handle
     *
     * @details
     * Must be called after ui_handle is created to enable menu popups.
     */
    void set_ui_handle(ui_handle<conio_backend>* ui) {
        m_ui_handle = ui;

        // No longer needed - menu_bar uses ui_services automatically
    }

private:
    void build_ui() {
        // Menu bar (at the top)
        build_menu_bar();

        // Title
        add_label(*this, "DOS Theme Showcase");

        // Theme label (keep pointer for updates)
        m_theme_label = add_label(*this, m_theme_names[m_current_theme]);

        // Spacer
        add_label(*this, "");

        // Demo panel with border
        auto* demo_panel = add_panel(*this);
        demo_panel->set_has_border(true);
        demo_panel->set_padding(thickness::all(1));
        demo_panel->set_vbox_layout(1);

        add_label(*demo_panel, "Panel with Border");
        add_label(*demo_panel, "This is a panel example");

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
        add_label(*this, "Press 1-4 to switch themes | ESC, Ctrl+C, or click Quit");
    }

    void setup_actions() {
        // Theme switching actions
        for (size_t i = 0; i < m_themes.size() && i < 4; ++i) {
            auto theme_action = std::make_shared<onyxui::action<conio_backend>>();
            theme_action->set_text(m_theme_names[i]);
            // Set numeric shortcuts for themes (1-4)
            theme_action->set_shortcut(static_cast<char>('1' + i));
            theme_action->triggered.connect([this, i]() {
                switch_theme(i);
            });
            this->hotkeys().register_action(theme_action);
            m_theme_actions.push_back(theme_action);  // Keep action alive!
        }

        // File menu actions
        auto new_action = std::make_shared<onyxui::action<conio_backend>>();
        new_action->set_text("New");
        new_action->set_shortcut('n', key_modifier::ctrl);  // Ctrl+N
        new_action->triggered.connect([]() {
            // Placeholder - in real app would create new document
            std::cerr << "New action triggered via menu/hotkey!" << std::endl;
        });
        m_new_action = new_action;
        this->hotkeys().register_action(new_action);

        auto open_action = std::make_shared<onyxui::action<conio_backend>>();
        open_action->set_text("Open");
        open_action->set_shortcut('o', key_modifier::ctrl);  // Ctrl+O
        open_action->triggered.connect([]() {
            // Placeholder - in real app would open file dialog
            std::cerr << "Open action triggered!" << std::endl;
        });
        m_open_action = open_action;
        this->hotkeys().register_action(open_action);

        // Quit action with Alt+F4 shortcut
        auto quit_action = std::make_shared<onyxui::action<conio_backend>>();
        quit_action->set_text("Quit");
        quit_action->set_shortcut_f(4, key_modifier::alt);  // Alt+F4
        quit_action->triggered.connect([this]() {
            std::cerr << "Quit action triggered - exiting application!" << std::endl;
            quit();
        });
        m_quit_action = quit_action;
        this->hotkeys().register_action(quit_action);

        // Help menu actions
        auto about_action = std::make_shared<onyxui::action<conio_backend>>();
        about_action->set_text("About");
        about_action->triggered.connect([]() {
            std::cerr << "About: DOS Theme Showcase v1.0" << std::endl;
        });
        m_about_action = about_action;
    }

    void build_menu_bar() {
        // Create menu bar
        auto menu_bar_ptr = std::make_unique<menu_bar<conio_backend>>(this);

        // File menu
        auto file_menu = std::make_unique<menu<conio_backend>>();

        // New item with action
        auto new_item = std::make_unique<menu_item<conio_backend>>("New");
        new_item->set_action(m_new_action);
        file_menu->add_item(std::move(new_item));

        // Open item with action
        auto open_item = std::make_unique<menu_item<conio_backend>>("Open");
        open_item->set_action(m_open_action);
        file_menu->add_item(std::move(open_item));

        file_menu->add_separator();

        // Quit item with action
        auto quit_item = std::make_unique<menu_item<conio_backend>>("Quit");
        quit_item->set_action(m_quit_action);
        file_menu->add_item(std::move(quit_item));

        // Apply theme to file menu BEFORE adding to menu_bar
        file_menu->apply_theme(m_themes[m_current_theme]);

        menu_bar_ptr->add_menu("&File", std::move(file_menu));

        // Theme menu
        auto theme_menu = std::make_unique<menu<conio_backend>>();

        for (size_t i = 0; i < m_theme_actions.size(); ++i) {
            // Initialize with theme name as placeholder text
            auto theme_item = std::make_unique<menu_item<conio_backend>>(m_theme_names[i]);
            theme_item->set_action(m_theme_actions[i]);
            theme_menu->add_item(std::move(theme_item));
        }

        // Apply theme to theme menu BEFORE adding to menu_bar
        theme_menu->apply_theme(m_themes[m_current_theme]);

        menu_bar_ptr->add_menu("&Theme", std::move(theme_menu));

        // Help menu
        auto help_menu = std::make_unique<menu<conio_backend>>();

        auto about_item = std::make_unique<menu_item<conio_backend>>("About");
        about_item->set_action(m_about_action);
        help_menu->add_item(std::move(about_item));

        // Apply theme to help menu BEFORE adding to menu_bar
        help_menu->apply_theme(m_themes[m_current_theme]);

        menu_bar_ptr->add_menu("&Help", std::move(help_menu));

        // Store pointer before moving
        m_menu_bar = menu_bar_ptr.get();

        // Add menu bar to UI
        this->add_child(std::move(menu_bar_ptr));
    }

    void switch_theme(size_t theme_index) {
        if (theme_index >= m_themes.size()) return;

        std::cerr << "Switching to theme " << theme_index << ": " << m_theme_names[theme_index] << std::endl;
        m_current_theme = theme_index;
        m_theme_label->set_text(m_theme_names[m_current_theme]);
        apply_theme(m_themes[m_current_theme]);
    }

private:
    std::vector<ui_theme<conio_backend>> m_themes;
    std::vector<std::string> m_theme_names;
    size_t m_current_theme;
    bool m_should_quit;

    label<conio_backend>* m_theme_label = nullptr;
    menu_bar<conio_backend>* m_menu_bar = nullptr;
    ui_handle<conio_backend>* m_ui_handle = nullptr;

    // Actions - kept alive as shared_ptrs
    std::shared_ptr<onyxui::action<conio_backend>> m_new_action;
    std::shared_ptr<onyxui::action<conio_backend>> m_open_action;
    std::shared_ptr<onyxui::action<conio_backend>> m_quit_action;
    std::shared_ptr<onyxui::action<conio_backend>> m_about_action;
    std::vector<std::shared_ptr<onyxui::action<conio_backend>>> m_theme_actions;  // Keep actions alive!
};

int main() {
    try {
        // Create vram instance (handles termbox2 initialization)
        auto vram_ptr = std::make_shared<vram>();

        // Create renderer
        conio_renderer renderer(vram_ptr);

        // Initialize themes
        std::vector<ui_theme<conio_backend>> themes;
        std::vector<std::string> theme_names;

        themes.push_back(create_dos_blue_theme());
        theme_names.push_back("DOS Blue (Classic)");

        themes.push_back(create_dos_dark_theme());
        theme_names.push_back("DOS Dark (Norton Commander)");

        themes.push_back(create_dos_monochrome_theme());
        theme_names.push_back("DOS Monochrome (Hercules)");

        themes.push_back(create_norton_utilities_theme());
        theme_names.push_back("Norton Utilities (Amber)");

        // Create main widget with all themes and logic
        auto widget = std::make_unique<main_widget>(
            std::move(themes),
            std::move(theme_names),
            0  // Initial theme index
        );

        // Keep reference to widget for event handling before moving it
        auto* widget_ptr = widget.get();

        // Create UI handle with main widget and renderer
        ui_handle<conio_backend> ui(std::move(widget), renderer);

        // Give widget access to UI handle for menu popups
        widget_ptr->set_ui_handle(&ui);

        // Initial render - display UI once before entering event loop
        // (otherwise screen is blank until first event arrives)
        {
            const int width = vram_ptr->get_width();
            const int height = vram_ptr->get_height();
            conio_backend::rect_type bounds;
            rect_utils::set_bounds(bounds, 0, 0, width, height);
            ui.display(bounds);
            ui.present();
        }

        // Main loop - event-driven design
        // Poll events, handle them (including resize), then render
        while (!widget_ptr->should_quit()) {
            // Wait for event (blocking)
            tb_event ev;
            int poll_result = tb_poll_event(&ev);

            // Handle polling errors gracefully
            // Note: On terminal resize, termbox2 may return an error on the first poll,
            // then return TB_EVENT_RESIZE on the second poll. We must continue to get
            // the actual resize event.
            if (poll_result != TB_OK) {
                continue;  // Don't break - next poll will have the actual event
            }

            // Handle Ctrl+C specially (always quit)
            if (ev.type == TB_EVENT_KEY && ev.ch == 'c' && (ev.mod & TB_MOD_CTRL)) {
                widget_ptr->quit();
                continue;
            }

            // Route all events through ui_handle's event system
            // (including resize events - ui_handle will call renderer.on_resize())
            // menu_bar now handles its own popups, focus, and click-outside behavior
            bool handled = ui.handle_event(ev);

            // If ESC wasn't handled by UI (no menu open), quit
            if (ev.type == TB_EVENT_KEY && ev.key == TB_KEY_ESC && !handled) {
                widget_ptr->quit();
                continue;
            }

            // NOW safe to render - vram buffer has been resized if needed
            // Get terminal size
            const int width = vram_ptr->get_width();
            const int height = vram_ptr->get_height();

            // Display UI within terminal bounds
            conio_backend::rect_type bounds;
            rect_utils::set_bounds(bounds, 0, 0, width, height);
            ui.display(bounds);

            // Present the frame
            ui.present();
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
