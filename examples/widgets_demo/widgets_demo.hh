//
// OnyxUI Widgets Demo — comprehensive showcase of framework features.
//
// Ported onto the simple-shell sdlpp bundle under WAR-57: the class
// no longer owns a run loop or a layer_manager. `build_widgets_demo`
// is the factory main.cc calls; it constructs the demo with a
// reference back to its hosting `ui::app_window` so
// spawned dialogs can route through `app_window::show_modal` and
// `app_window::host().present(...)`.
//

#pragma once

#include "backend_config.hh"  // Pulls <onyxui/for/sdlpp.hh> + Backend alias.

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include <onyxui/actions/action.hh>

// Tab implementations
#include "tabs/tab_all_widgets.hh"
#include "tabs/tab_layout_scrolling.hh"
#include "tabs/tab_events_interaction.hh"

// Window implementations
#include "windows/mvc_demo_window.hh"
#include "windows/modal_dialog_example.hh"
#include "windows/modeless_dialog_example.hh"
#include "windows/about_dialog.hh"

/**
 * @brief Widgets demo main window.
 *
 * Inherits `ui::main_window` — the root widget tree that
 * `app_window::set_content` mounts. All service access (themes,
 * hotkeys) goes through the parent `app_window`'s `ui_host`, which
 * exposes them directly as member accessors (no ambient-scope
 * requirement).
 */
class widgets_demo : public ui::main_window {
public:
    /**
     * @param parent The hosting simple-shell app_window. Stored by
     *               reference — its lifetime must strictly exceed
     *               this widget's.
     */
    explicit widgets_demo(ui::app_window& parent)
        : ui::main_window()
        , m_parent(parent)
        , m_current_theme_index(0)
    {
        // Pull themes from the host's registry directly — no
        // thread-local service lookup needed. ui_host exposes this
        // as a member accessor regardless of whether we're inside a
        // render/event call.
        auto& themes = m_parent.host().themes();
        m_theme_names = themes.list_theme_names();

        if (m_theme_names.empty()) {
            throw std::runtime_error(
                "No themes registered! Call "
                "sdlpp_themes::register_default_themes() first.");
        }

        auto norton_it = std::find(
            m_theme_names.begin(), m_theme_names.end(), "NU8");
        m_current_theme_index = (norton_it != m_theme_names.end())
            ? static_cast<std::size_t>(
                std::distance(m_theme_names.begin(), norton_it))
            : 0;

        setup_actions();
        build_ui();
        apply_theme_by_name(m_theme_names[m_current_theme_index]);
    }

    void take_screenshot(const std::string& filename = "") {
        std::string output_file = filename;
        if (output_file.empty()) {
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            output_file = "screenshot_" + std::to_string(timestamp) + ".txt";
        }

        std::ofstream file(output_file);
        if (!file) {
            std::cerr << "Failed to open file: " << output_file << "\n";
            return;
        }
        m_parent.take_screenshot(file);
    }

    void switch_to_theme_index(std::size_t index) {
        if (index >= m_theme_names.size()) return;
        m_current_theme_index = index;
        apply_theme_by_name(m_theme_names[index]);
    }

    void show_mvc_demo() {
        present_modeless(widgets_demo_windows::create_mvc_demo_window());
    }

    void show_theme_editor() {
        std::cout << "Theme Editor window - Coming soon!\n";
    }

    void show_debug_tools() {
        std::cout << "Debug Tools window - Coming soon!\n";
    }

private:
    void apply_theme_by_name(const std::string& theme_name) {
        m_parent.host().themes().set_current_theme(theme_name);
    }

    void setup_actions() {
        auto& hotkeys = m_parent.host().hotkeys();

        auto screenshot_f9 = std::make_shared<ui::action>();
        screenshot_f9->set_text("Screenshot");
        screenshot_f9->set_shortcut_f(9);
        screenshot_f9->triggered.connect([this]() { take_screenshot(); });
        hotkeys.register_action(screenshot_f9);
        m_actions.push_back(screenshot_f9);

        m_screenshot_action = std::make_shared<ui::action>();
        m_screenshot_action->set_text("Save Screenshot");
        m_screenshot_action->set_shortcut_f(2);
        m_screenshot_action->triggered.connect([this]() { take_screenshot(); });
        hotkeys.register_action(m_screenshot_action);
        m_actions.push_back(m_screenshot_action);

        m_mvc_action = std::make_shared<ui::action>();
        m_mvc_action->set_text("MVC Demo");
        m_mvc_action->set_shortcut('m', onyxui::key_modifier::ctrl);
        m_mvc_action->triggered.connect([this]() { show_mvc_demo(); });
        hotkeys.register_action(m_mvc_action);
        m_actions.push_back(m_mvc_action);

        m_theme_editor_action = std::make_shared<ui::action>();
        m_theme_editor_action->set_text("Theme Editor");
        m_theme_editor_action->set_shortcut('t', onyxui::key_modifier::ctrl);
        m_theme_editor_action->triggered.connect([this]() { show_theme_editor(); });
        hotkeys.register_action(m_theme_editor_action);
        m_actions.push_back(m_theme_editor_action);

        m_debug_tools_action = std::make_shared<ui::action>();
        m_debug_tools_action->set_text("Debug Tools");
        m_debug_tools_action->set_shortcut_f(12);
        m_debug_tools_action->triggered.connect([this]() { show_debug_tools(); });
        hotkeys.register_action(m_debug_tools_action);
        m_actions.push_back(m_debug_tools_action);

        // Quit goes through ui::quit() — no more
        // m_should_quit flag; the run loop exits when requested.
        m_quit_action = std::make_shared<ui::action>();
        m_quit_action->set_text("Quit");
        m_quit_action->set_shortcut_f(4, onyxui::key_modifier::alt);
        m_quit_action->triggered.connect([]() { ui::quit(); });
        hotkeys.register_action(m_quit_action);
        m_actions.push_back(m_quit_action);

        auto quit_alt = std::make_shared<ui::action>();
        quit_alt->set_text("Quit");
        quit_alt->set_shortcut('q', onyxui::key_modifier::alt);
        quit_alt->triggered.connect([]() { ui::quit(); });
        hotkeys.register_action(quit_alt);
        m_actions.push_back(quit_alt);
    }

    void build_ui() {
        build_menu_bar();
        this->set_central_widget(std::make_unique<ui::panel>());
        build_tabs();
        build_status_bar();
    }

    void build_menu_bar() {
        auto menu_bar_widget = std::make_unique<ui::menu_bar>();

        if (auto* theme = m_parent.host().themes().get_current_theme()) {
            menu_bar_widget->set_spacing(theme->menu_bar.item_spacing);
        }

        // File menu
        auto file_menu = std::make_unique<ui::menu>();
        {
            auto screenshot_item = std::make_unique<ui::menu_item>("");
            screenshot_item->set_mnemonic_text("&Save Screenshot");
            screenshot_item->set_action(m_screenshot_action);
            file_menu->add_item(std::move(screenshot_item));

            file_menu->add_separator();

            auto exit_item = std::make_unique<ui::menu_item>("");
            exit_item->set_mnemonic_text("E&xit");
            exit_item->set_action(m_quit_action);
            file_menu->add_item(std::move(exit_item));
        }
        menu_bar_widget->add_menu("&File", std::move(file_menu));

        // Windows menu
        auto windows_menu = std::make_unique<ui::menu>();
        {
            auto mvc_item = std::make_unique<ui::menu_item>("");
            mvc_item->set_mnemonic_text("&MVC Demo...");
            mvc_item->set_action(m_mvc_action);
            windows_menu->add_item(std::move(mvc_item));

            auto theme_editor_item = std::make_unique<ui::menu_item>("");
            theme_editor_item->set_mnemonic_text("&Theme Editor...");
            theme_editor_item->set_action(m_theme_editor_action);
            windows_menu->add_item(std::move(theme_editor_item));

            auto debug_item = std::make_unique<ui::menu_item>("");
            debug_item->set_mnemonic_text("&Debug Tools...");
            debug_item->set_action(m_debug_tools_action);
            windows_menu->add_item(std::move(debug_item));

            windows_menu->add_separator();

            auto modal_item = std::make_unique<ui::menu_item>("");
            modal_item->set_mnemonic_text("M&odal Dialog Example...");
            modal_item->clicked.connect([this]() { show_modal_dialog(); });
            windows_menu->add_item(std::move(modal_item));

            auto modeless_item = std::make_unique<ui::menu_item>("");
            modeless_item->set_mnemonic_text("Mode&less Dialog Example...");
            modeless_item->clicked.connect([this]() { show_modeless_dialog(); });
            windows_menu->add_item(std::move(modeless_item));
        }
        menu_bar_widget->add_menu("&Windows", std::move(windows_menu));

        // Theme menu
        auto theme_menu = std::make_unique<ui::menu>();
        for (std::size_t i = 0; i < m_theme_names.size(); ++i) {
            auto theme_item = std::make_unique<ui::menu_item>(
                m_theme_names[i]);
            theme_item->clicked.connect([this, i]() { switch_to_theme_index(i); });
            theme_menu->add_item(std::move(theme_item));
        }
        menu_bar_widget->add_menu("&Theme", std::move(theme_menu));

        // Help menu
        auto help_menu = std::make_unique<ui::menu>();
        {
            auto about_item = std::make_unique<ui::menu_item>("");
            about_item->set_mnemonic_text("&About OnyxUI...");
            about_item->clicked.connect([this]() { show_about_dialog(); });
            help_menu->add_item(std::move(about_item));
        }
        menu_bar_widget->add_menu("&Help", std::move(help_menu));

        this->set_menu_bar(std::move(menu_bar_widget));
    }

    void build_tabs() {
        auto* central = this->central_widget();
        if (!central) return;

        m_tab_widget = central->emplace_child<ui::tab_widget>();

        m_tab_widget->add_tab(
            widgets_demo_tabs::create_tab_all_widgets(
                [this]() { take_screenshot(); },
                [this]() { show_theme_editor(); },
                [this]() { show_mvc_demo(); }),
            "All Widgets");
        m_tab_widget->add_tab(
            widgets_demo_tabs::create_tab_layout_scrolling(),
            "Layout & Scrolling");
        m_tab_widget->add_tab(
            widgets_demo_tabs::create_tab_events_interaction(),
            "Events & Interaction");

        m_tab_widget->set_current_index(0);
    }

    void build_status_bar() {
        auto status = std::make_unique<ui::status_bar>();
        status->set_left_text(
            "Theme: " + m_theme_names[m_current_theme_index] +
            " | FPS: -- | Focus: -- | Widgets: --");
        status->set_right_text("OnyxUI Widgets Demo v1.0");
        this->set_status_bar(std::move(status));
    }

    // ---- spawn helpers ----

    // Modal spawns hand ownership to the host app_window, which owns
    // the presenter and drops it when `window::closed` fires.
    void show_modal_dialog() {
        m_parent.show_modal(widgets_demo_windows::create_modal_dialog(
            "This is a modal dialog!\n\n"
            "Try clicking the main window - it's blocked.\n"
            "Focus is trapped within this dialog."));
    }

    void show_about_dialog() {
        m_parent.show_modal(widgets_demo_windows::create_about_dialog());
    }

    // Modeless spawns: app_window::show_modal is modal-only, so we
    // reach through to ui_host::present and own the presenter
    // ourselves. Same auto-drop pattern as the old present_window.
    void show_modeless_dialog() {
        present_modeless(widgets_demo_windows::create_modeless_dialog());
    }

    void present_modeless(std::unique_ptr<ui::window> win) {
        if (!win) return;

        auto presenter = std::make_unique<
            ui::presented_window>(
            m_parent.host().present(std::move(win)));
        auto* presenter_ptr = presenter.get();

        presenter_ptr->get()->closed.connect([this, presenter_ptr]() {
            auto it = std::find_if(
                m_modeless.begin(), m_modeless.end(),
                [presenter_ptr](const auto& p) {
                    return p.get() == presenter_ptr;
                });
            if (it != m_modeless.end()) m_modeless.erase(it);
        });

        m_modeless.push_back(std::move(presenter));
    }

private:
    ui::app_window& m_parent;

    std::vector<std::string> m_theme_names;
    std::size_t m_current_theme_index;

    ui::tab_widget* m_tab_widget = nullptr;

    std::vector<std::shared_ptr<ui::action>> m_actions;
    std::shared_ptr<ui::action> m_screenshot_action;
    std::shared_ptr<ui::action> m_quit_action;
    std::shared_ptr<ui::action> m_mvc_action;
    std::shared_ptr<ui::action> m_theme_editor_action;
    std::shared_ptr<ui::action> m_debug_tools_action;

    // Modeless presenters we own — modal ones are owned by m_parent.
    std::vector<std::unique_ptr<ui::presented_window>> m_modeless;
};

// Factory called from main.cc — constructs the demo with a reference
// back to the hosting app_window and returns it as the root widget
// tree for `app_window::set_content`.
//
// No scope wrapping is needed: WAR-64 moved constructor-time
// ambient service lookups (menu_bar's hotkey registration,
// text_view's theme padding) onto the `on_attached` hook, which
// the host dispatches with a scope already pushed. Widget
// construction is now pure.
inline std::unique_ptr<ui::main_window>
build_widgets_demo(ui::app_window& parent) {
    return std::make_unique<widgets_demo>(parent);
}
