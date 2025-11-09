/**
 * @file window_list_dialog.hh
 * @brief Turbo Vision style window list dialog
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * Modal dialog that shows all windows with state indicators.
 * Allows user to select window to activate/restore.
 *
 * Appearance:
 * ┌─ Windows ─────────────────────────┐
 * │ [1] Editor - document.txt         │
 * │ [2] Calculator                    │
 * │ [3] Settings (minimized)          │
 * │ [4] Help (modal)                  │
 * │ [5] File Manager (maximized)      │
 * └───────────────────────────────────┘
 */

#pragma once

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/label.hh>
#include <vector>
#include <string>

namespace onyxui {

    /**
     * @class window_list_dialog
     * @brief Turbo Vision style window list dialog
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Modal window that displays a list of all windows in the application.
     * Provides keyboard navigation and state indicators (minimized, maximized, modal).
     *
     * Usage:
     * @code
     * auto* mgr = ui_services<Backend>::window_manager();
     * auto dialog = std::make_unique<window_list_dialog<Backend>>();
     *
     * // Add all windows from manager
     * for (auto* win : mgr->get_all_windows()) {
     *     dialog->add_window(win);
     * }
     *
     * // Handle selection
     * dialog->window_selected.connect([](window<Backend>* win) {
     *     if (win->get_state() == window<Backend>::window_state::minimized) {
     *         win->restore();
     *     }
     *     // Bring to front and focus
     * });
     *
     * dialog->show_modal();
     * @endcode
     */
    template<UIBackend Backend>
    class window_list_dialog : public window<Backend> {
    public:
        using base = window<Backend>;
        using window_ptr = window<Backend>*;

        /**
         * @brief Filter mode for window list
         */
        enum class filter_mode : uint8_t {
            all,            ///< Show all windows
            visible_only,   ///< Show only visible (not minimized)
            minimized_only  ///< Show only minimized
        };

        /**
         * @brief Construct window list dialog
         * @param filter Initial filter mode (default: all)
         */
        explicit window_list_dialog(filter_mode filter = filter_mode::all)
            : base("Windows", get_dialog_flags())
            , m_filter(filter)
            , m_selected_index(0)
        {
            // Create content area with vbox
            auto content = std::make_unique<vbox<Backend>>(2);  // 2px spacing
            m_content_vbox = content.get();
            this->set_content(std::move(content));

            // Set reasonable default size
            this->set_size(50, 15);
        }

        /**
         * @brief Add window to the list
         * @param win Window to add (non-owning pointer)
         */
        void add_window(window_ptr win) {
            if (!win) return;

            m_windows.push_back(win);
            refresh_list();
        }

        /**
         * @brief Clear all windows from list
         */
        void clear_windows() {
            m_windows.clear();
            refresh_list();
        }

        /**
         * @brief Set filter mode
         * @param filter New filter mode
         */
        void set_filter(filter_mode filter) {
            if (m_filter != filter) {
                m_filter = filter;
                m_selected_index = 0;
                refresh_list();
            }
        }

        /**
         * @brief Get current filter mode
         */
        [[nodiscard]] filter_mode get_filter() const noexcept {
            return m_filter;
        }

        /**
         * @brief Get selected window index
         */
        [[nodiscard]] int get_selected_index() const noexcept {
            return m_selected_index;
        }

        /**
         * @brief Set selected window index
         * @param index Index to select (clamped to valid range)
         */
        void set_selected_index(int index) {
            auto filtered = get_filtered_windows();
            if (filtered.empty()) {
                m_selected_index = 0;
                return;
            }

            m_selected_index = std::clamp(index, 0, static_cast<int>(filtered.size()) - 1);
            refresh_list();
        }

        /**
         * @brief Select next window in list
         */
        void select_next() {
            auto filtered = get_filtered_windows();
            if (filtered.empty()) return;

            m_selected_index = (m_selected_index + 1) % static_cast<int>(filtered.size());
            refresh_list();
        }

        /**
         * @brief Select previous window in list
         */
        void select_previous() {
            auto filtered = get_filtered_windows();
            if (filtered.empty()) return;

            m_selected_index--;
            if (m_selected_index < 0) {
                m_selected_index = static_cast<int>(filtered.size()) - 1;
            }
            refresh_list();
        }

        /**
         * @brief Activate/restore selected window and close dialog
         */
        void activate_selected() {
            auto filtered = get_filtered_windows();
            if (m_selected_index >= 0 && m_selected_index < static_cast<int>(filtered.size())) {
                auto* selected = filtered[static_cast<size_t>(m_selected_index)];
                window_selected.emit(selected);
                this->close();
            }
        }

        /**
         * @brief Signal emitted when user selects a window
         */
        signal<window_ptr> window_selected;

    protected:
        /**
         * @brief Handle keyboard events for navigation
         */
        bool handle_event(const ui_event& event, event_phase phase) override {
            // Handle keyboard navigation in bubble phase
            if (phase != event_phase::bubble) {
                return base::handle_event(event, phase);
            }

            auto* key_evt = std::get_if<keyboard_event>(&event);
            if (!key_evt) {
                return base::handle_event(event, phase);
            }

            // Arrow key navigation
            if (key_evt->key == key_code::arrow_down) {
                select_next();
                return true;
            }
            if (key_evt->key == key_code::arrow_up) {
                select_previous();
                return true;
            }

            // Enter - activate selected window
            if (key_evt->key == key_code::enter) {
                activate_selected();
                return true;
            }

            // Escape - close dialog without selecting
            if (key_evt->key == key_code::escape) {
                this->close();
                return true;
            }

            return base::handle_event(event, phase);
        }

    private:
        filter_mode m_filter;
        int m_selected_index;
        std::vector<window_ptr> m_windows;  // Non-owning pointers
        vbox<Backend>* m_content_vbox;      // Non-owning pointer to content

        /**
         * @brief Get window flags for dialog
         */
        static typename window<Backend>::window_flags get_dialog_flags() {
            typename window<Backend>::window_flags flags;
            flags.has_minimize_button = false;
            flags.has_maximize_button = false;
            flags.is_resizable = false;
            flags.is_movable = true;
            // TODO Phase 5: flags.is_modal = true;
            return flags;
        }

        /**
         * @brief Get filtered windows based on current filter mode
         */
        [[nodiscard]] std::vector<window_ptr> get_filtered_windows() const {
            std::vector<window_ptr> filtered;
            filtered.reserve(m_windows.size());

            for (auto* win : m_windows) {
                if (!win) continue;

                bool include = false;
                switch (m_filter) {
                    case filter_mode::all:
                        include = true;
                        break;
                    case filter_mode::visible_only:
                        include = (win->is_visible() &&
                                  win->get_state() != window<Backend>::window_state::minimized);
                        break;
                    case filter_mode::minimized_only:
                        include = (win->get_state() == window<Backend>::window_state::minimized);
                        break;
                }

                if (include) {
                    filtered.push_back(win);
                }
            }

            return filtered;
        }

        /**
         * @brief Refresh the window list display
         */
        void refresh_list() {
            if (!m_content_vbox) return;

            // Clear existing labels
            // Note: This is a simplified implementation
            // A production version would properly manage child widgets

            // Get filtered windows
            auto filtered = get_filtered_windows();

            // TODO: Add labels for each window
            // For now, this is a placeholder
            // Full implementation would create/update label widgets
            // showing: "[N] Title (state)"
        }

        /**
         * @brief Get display label for a window
         * @param win Window to get label for
         * @param index Display index (1-based)
         */
        [[nodiscard]] std::string get_window_label(window_ptr win, int index) const {
            if (!win) return "";

            std::string label = "[" + std::to_string(index) + "] " + win->get_title();

            // Add state indicator
            auto state = win->get_state();
            if (state == window<Backend>::window_state::minimized) {
                label += " (minimized)";
            } else if (state == window<Backend>::window_state::maximized) {
                label += " (maximized)";
            }

            // TODO Phase 5: Add modal indicator
            // if (win->is_modal()) {
            //     label += " (modal)";
            // }

            return label;
        }
    };

} // namespace onyxui
