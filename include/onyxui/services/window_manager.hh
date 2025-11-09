/**
 * @file window_manager.hh
 * @brief Service for tracking and managing all windows in the application
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * The window_manager is a ui_services service that maintains a registry of all
 * windows in the application. It provides:
 * - Window enumeration (all, visible, minimized, modal)
 * - Window list dialog (Turbo Vision style)
 * - Ctrl+W hotkey registration
 * - Custom minimize behavior override support
 *
 * Windows automatically register/unregister on construction/destruction.
 */

#pragma once

#include <onyxui/core/signal.hh>
#include <onyxui/concepts/backend.hh>
#include <vector>
#include <functional>
#include <algorithm>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class window;
    template<UIBackend Backend> class window_list_dialog;

    /**
     * @class window_manager
     * @brief Service that tracks all windows and provides window list functionality
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Provides centralized window tracking and management:
     * - Automatic registration/unregister on window construction/destruction
     * - Window enumeration by various criteria (all, visible, minimized, modal)
     * - Turbo Vision style window list dialog (Ctrl+W)
     * - Custom minimize handler override (for taskbar integration)
     *
     * Usage:
     * @code
     * auto* mgr = ui_services<Backend>::window_manager();
     * if (mgr) {
     *     // Show window list dialog
     *     mgr->show_window_list();
     *
     *     // Get all minimized windows
     *     auto minimized = mgr->get_minimized_windows();
     *
     *     // Custom minimize behavior
     *     mgr->set_minimize_handler([](window<Backend>* win) {
     *         // Add to custom taskbar
     *         my_taskbar->add_minimized_window(win);
     *     });
     * }
     * @endcode
     */
    template<UIBackend Backend>
    class window_manager {
    public:
        /**
         * @brief Default constructor
         */
        window_manager() = default;

        /**
         * @brief Destructor
         */
        ~window_manager() = default;

        // Delete copy/move (singleton-like service)
        window_manager(const window_manager&) = delete;
        window_manager& operator=(const window_manager&) = delete;
        window_manager(window_manager&&) = delete;
        window_manager& operator=(window_manager&&) = delete;

        // ====================================================================
        // Window Registration
        // ====================================================================

        /**
         * @brief Register a window with the manager
         * @param win Pointer to window (non-owning)
         *
         * @details
         * Called automatically by window constructor.
         * Emits window_registered signal.
         */
        void register_window(window<Backend>* win) {
            if (!win) return;

            // Check if already registered
            auto it = std::find(m_windows.begin(), m_windows.end(), win);
            if (it != m_windows.end()) {
                return;  // Already registered
            }

            m_windows.push_back(win);
            window_registered.emit(win);
        }

        /**
         * @brief Unregister a window from the manager
         * @param win Pointer to window
         *
         * @details
         * Called automatically by window destructor.
         * Emits window_unregistered signal.
         */
        void unregister_window(window<Backend>* win) {
            if (!win) return;

            auto it = std::find(m_windows.begin(), m_windows.end(), win);
            if (it != m_windows.end()) {
                m_windows.erase(it);
                window_unregistered.emit(win);
            }
        }

        // ====================================================================
        // Window Enumeration
        // ====================================================================

        /**
         * @brief Get all registered windows
         * @return Vector of window pointers
         */
        [[nodiscard]] std::vector<window<Backend>*> get_all_windows() const {
            return m_windows;
        }

        /**
         * @brief Get all visible (not minimized) windows
         * @return Vector of visible window pointers
         *
         * @note Implementation requires window<Backend> to be fully defined
         */
        [[nodiscard]] std::vector<window<Backend>*> get_visible_windows() const;

        /**
         * @brief Get all minimized windows
         * @return Vector of minimized window pointers
         *
         * @note Implementation requires window<Backend> to be fully defined
         */
        [[nodiscard]] std::vector<window<Backend>*> get_minimized_windows() const;

        /**
         * @brief Get all modal windows
         * @return Vector of modal window pointers
         *
         * @note Implementation requires window<Backend> to be fully defined
         */
        [[nodiscard]] std::vector<window<Backend>*> get_modal_windows() const;

        /**
         * @brief Get window count
         * @return Number of registered windows
         */
        [[nodiscard]] size_t get_window_count() const {
            return m_windows.size();
        }

        // ====================================================================
        // Window List Dialog
        // ====================================================================

        /**
         * @brief Show Turbo Vision style window list dialog
         *
         * @details
         * Shows modal dialog listing all windows with state indicators.
         * User can navigate with arrow keys and Enter to activate/restore.
         * Usually triggered by Ctrl+W hotkey.
         */
        void show_window_list();

        // ====================================================================
        // Window Cycling
        // ====================================================================

        /**
         * @brief Cycle to next window
         *
         * @details
         * Switches focus to the next visible window in registration order.
         * Wraps around to the first window when reaching the end.
         * Triggered by Ctrl+Tab (Windows) or Ctrl+F6 (Norton Commander).
         */
        void cycle_next_window();

        /**
         * @brief Cycle to previous window
         *
         * @details
         * Switches focus to the previous visible window in registration order.
         * Wraps around to the last window when reaching the beginning.
         * Triggered by Ctrl+Shift+Tab (Windows) or Ctrl+Shift+F6 (Norton Commander).
         */
        void cycle_previous_window();

        /**
         * @brief Get currently active/focused window
         * @return Pointer to active window, or nullptr if none
         */
        [[nodiscard]] window<Backend>* get_active_window() const noexcept {
            return m_active_window;
        }

        /**
         * @brief Set currently active/focused window
         * @param win Window to activate (nullptr to deactivate all)
         */
        void set_active_window(window<Backend>* win) {
            m_active_window = win;
        }

        // ====================================================================
        // Custom Minimize Handler
        // ====================================================================

        /**
         * @brief Set custom minimize handler
         * @param handler Function to call when window is minimized
         *
         * @details
         * Override default minimize behavior (hide window).
         * Useful for taskbar integration or custom minimize behavior.
         *
         * Example:
         * @code
         * mgr->set_minimize_handler([](window<Backend>* win) {
         *     my_taskbar->add_minimized_window(win);
         * });
         * @endcode
         */
        void set_minimize_handler(std::function<void(window<Backend>*)> handler) {
            m_custom_minimize_handler = std::move(handler);
        }

        /**
         * @brief Clear custom minimize handler
         *
         * @details
         * Restore default minimize behavior (hide window).
         */
        void clear_minimize_handler() {
            m_custom_minimize_handler = nullptr;
        }

        /**
         * @brief Check if custom minimize handler is set
         */
        [[nodiscard]] bool has_custom_minimize_handler() const {
            return m_custom_minimize_handler != nullptr;
        }

        /**
         * @brief Invoke minimize handler (custom or default)
         * @param win Window to minimize
         *
         * @details
         * Called by window::minimize(). Uses custom handler if set,
         * otherwise uses default behavior (hide window).
         */
        void handle_minimize(window<Backend>* win) {
            if (m_custom_minimize_handler) {
                m_custom_minimize_handler(win);
            }
            // Default behavior (hide) is handled by window::minimize()

            window_minimized.emit(win);
        }

        /**
         * @brief Notify manager that window was restored
         * @param win Window that was restored
         */
        void handle_restore(window<Backend>* win) {
            window_restored.emit(win);
        }

        // ====================================================================
        // Hotkey Registration
        // ====================================================================

        /**
         * @brief Register window manager hotkeys
         *
         * @details
         * Registers semantic actions:
         * - show_window_list (Ctrl+W) - Show window list dialog
         * - next_window (Ctrl+F6) - Cycle to next window
         * - prev_window (Ctrl+Shift+F6) - Cycle to previous window
         */
        void register_hotkeys();

        // ====================================================================
        // Signals
        // ====================================================================

        signal<window<Backend>*> window_registered;    ///< Emitted when window registered
        signal<window<Backend>*> window_unregistered;  ///< Emitted when window unregistered
        signal<window<Backend>*> window_minimized;     ///< Emitted when window minimized
        signal<window<Backend>*> window_restored;      ///< Emitted when window restored

    private:
        std::vector<window<Backend>*> m_windows;  ///< All registered windows (non-owning)
        std::function<void(window<Backend>*)> m_custom_minimize_handler;  ///< Custom minimize handler
        window<Backend>* m_active_window = nullptr;  ///< Currently active/focused window
    };

} // namespace onyxui

// Note: Implementation of get_visible_windows(), get_minimized_windows(), etc.
// is provided in window.inl AFTER window<Backend> is fully defined to avoid
// circular dependencies.
