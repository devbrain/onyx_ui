/**
 * @file main_window.hh
 * @brief Qt-style main application window with menu bar, central widget, and status bar
 * @author Claude Code
 * @date 2025-11-11
 *
 * @details
 * main_window provides a standard application layout pattern:
 * - Optional menu bar at top
 * - Central widget in middle (fills remaining space)
 * - Optional status bar at bottom
 *
 * Overlay windows hosted in the main_window are constructed directly by
 * the consumer. Pair each window to `central_widget()` via
 * `window::set_workspace()` so maximize() fills the central area
 * (respecting menu and status bars) rather than the whole viewport.
 *
 * ## Usage
 *
 * @code
 * auto main = std::make_unique<main_window<Backend>>();
 * main->set_menu_bar(std::make_unique<menu_bar<Backend>>());
 *
 * auto win = std::make_unique<window<Backend>>("Document 1");
 * win->set_workspace(main->central_widget());
 * auto presenter = std::make_unique<presented_window<Backend>>(
 *     layers, std::move(win));
 *
 * main->set_status_bar(std::make_unique<status_bar<Backend>>());
 * @endcode
 */

#pragma once

#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/widgets/window/window.hh>
#include <memory>

namespace onyxui {

    /**
     * @class main_window
     * @brief Main application window with standard layout (menu, central, status)
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Provides Qt QMainWindow-style layout with three regions:
     * 1. Menu bar (optional, top, content-sized)
     * 2. Central widget (required, middle, fills remaining space)
     * 3. Status bar (optional, bottom, content-sized)
     *
     * The central widget serves as the *workspace* for overlay windows
     * hosted in the main_window — consumers pair each window to it via
     * `window::set_workspace(central_widget())` so maximize() fills the
     * central area instead of the whole viewport. Windows are not tree
     * children of the central widget; they are overlay windows living in
     * the layer_manager.
     *
     * ## Signals
     *
     * (none - delegates to child widgets)
     */
    template<UIBackend Backend>
    class main_window : public vbox<Backend> {
    public:
        using base = vbox<Backend>;
        using window_type = window<Backend>;
        using menu_bar_type = menu_bar<Backend>;
        using status_bar_type = status_bar<Backend>;
        using panel_type = panel<Backend>;

        /**
         * @brief Construct main window with vertical layout
         *
         * @details
         * Creates a main window with:
         * - Vertical layout (vbox with no spacing)
         * - Empty central widget (panel)
         * - No menu or status bar (set via set_menu_bar/set_status_bar)
         */
        main_window();

        /**
         * @brief Destructor
         */
        ~main_window() override = default;

        // Rule of Five
        main_window(const main_window&) = delete;
        main_window& operator=(const main_window&) = delete;
        main_window(main_window&&) noexcept = default;
        main_window& operator=(main_window&&) noexcept = default;

        /**
         * @brief Set menu bar at top of window
         * @param menu Menu bar widget (takes ownership)
         *
         * @details
         * Menu bar is positioned at top and sized to content height.
         * If a menu bar already exists, it is replaced.
         */
        void set_menu_bar(std::unique_ptr<menu_bar_type> menu);

        /**
         * @brief Set status bar at bottom of window
         * @param status Status bar widget (takes ownership)
         *
         * @details
         * Status bar is positioned at bottom and sized to content height.
         * If a status bar already exists, it is replaced.
         */
        void set_status_bar(std::unique_ptr<status_bar_type> status);

        /**
         * @brief Set central widget (main content area)
         * @param widget Central widget (takes ownership)
         *
         * @details
         * Central widget fills space between menu and status bar.
         * If a central widget already exists, it is replaced.
         * Pair overlay windows to this via `window::set_workspace()` so
         * maximize() fills the central area.
         */
        void set_central_widget(std::unique_ptr<ui_element<Backend>> widget);

        /**
         * @brief Get central widget (the workspace for overlay windows)
         * @return Pointer to central widget (created lazily, may be nullptr before first use)
         */
        [[nodiscard]] ui_element<Backend>* central_widget() noexcept {
            ensure_central_widget();
            return m_central_widget;
        }

        /**
         * @brief Get central widget (const version)
         * @return Pointer to central widget (may be nullptr)
         */
        [[nodiscard]] const ui_element<Backend>* central_widget() const noexcept {
            return m_central_widget;
        }

        /**
         * @brief Get menu bar
         * @return Pointer to menu bar, or nullptr if not set
         */
        [[nodiscard]] menu_bar_type* get_menu_bar() noexcept {
            return m_menu_bar;
        }

        /**
         * @brief Get status bar
         * @return Pointer to status bar, or nullptr if not set
         */
        [[nodiscard]] status_bar_type* get_status_bar() noexcept {
            return m_status_bar;
        }

    private:
        menu_bar_type* m_menu_bar = nullptr;         ///< Menu bar at top (optional)
        ui_element<Backend>* m_central_widget = nullptr;  ///< Central widget (required)
        status_bar_type* m_status_bar = nullptr;     ///< Status bar at bottom (optional)

        /**
         * @brief Ensure central widget exists (lazy initialization)
         *
         * @details
         * Creates default empty panel as central widget if not already
         * set. Called by the constructor and by the non-const
         * `central_widget()` accessor.
         */
        void ensure_central_widget();
    };

} // namespace onyxui

// Implementation
#include <onyxui/widgets/main_window.inl>
