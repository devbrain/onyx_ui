/**
 * @file window_manager.inl
 * @brief Implementation of window_manager service
 */

#pragma once

// Note: window.hh will be included by files that use window_manager
// Forward declaration is sufficient here to avoid circular dependency

namespace onyxui {

    template<UIBackend Backend>
    std::vector<window<Backend>*> window_manager<Backend>::get_visible_windows() const {
        std::vector<window<Backend>*> visible;
        visible.reserve(m_windows.size());

        for (auto* win : m_windows) {
            if (win && win->is_visible() && win->get_state() != window<Backend>::window_state::minimized) {
                visible.push_back(win);
            }
        }

        return visible;
    }

    template<UIBackend Backend>
    std::vector<window<Backend>*> window_manager<Backend>::get_minimized_windows() const {
        std::vector<window<Backend>*> minimized;
        minimized.reserve(m_windows.size());

        for (auto* win : m_windows) {
            if (win && win->get_state() == window<Backend>::window_state::minimized) {
                minimized.push_back(win);
            }
        }

        return minimized;
    }

    template<UIBackend Backend>
    std::vector<window<Backend>*> window_manager<Backend>::get_modal_windows() const {
        std::vector<window<Backend>*> modal;
        modal.reserve(m_windows.size());

        for (auto* win : m_windows) {
            // TODO Phase 5: Check if window is modal
            // For now, return empty (modal support comes in Phase 5)
            // if (win && win->is_modal()) {
            //     modal.push_back(win);
            // }
        }

        return modal;
    }

    template<UIBackend Backend>
    void window_manager<Backend>::show_window_list() {
        // TODO Phase 4: Implement window_list_dialog
        // For now, this is a stub

        // Future implementation:
        // auto dialog = std::make_unique<window_list_dialog<Backend>>();
        // for (auto* win : m_windows) {
        //     dialog->add_window(win);
        // }
        // dialog->window_selected.connect([](window<Backend>* win) {
        //     if (win->get_state() == window_state::minimized) {
        //         win->restore();
        //     }
        //     // Bring to front and focus
        // });
        // dialog->show_modal();
    }

    template<UIBackend Backend>
    void window_manager<Backend>::register_hotkeys() {
        // TODO Phase 4: Register semantic actions for window management
        // Requires hotkey_manager integration

        // Future implementation:
        // auto* hotkeys = ui_services<Backend>::hotkeys();
        // if (!hotkeys) return;
        //
        // // Ctrl+W - Show window list
        // hotkeys->register_semantic_action(
        //     hotkey_action::show_window_list,
        //     [this]() { this->show_window_list(); }
        // );
        //
        // // Ctrl+F6 - Cycle to next window
        // hotkeys->register_semantic_action(
        //     hotkey_action::next_window,
        //     [this]() { /* cycle to next window */ }
        // );
        //
        // // Ctrl+Shift+F6 - Cycle to previous window
        // hotkeys->register_semantic_action(
        //     hotkey_action::prev_window,
        //     [this]() { /* cycle to previous window */ }
        // );
    }

} // namespace onyxui
