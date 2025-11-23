//
// Created by Claude Code on 2025-11-23.
//
// Window registry helper for widgets_demo
// Manages lifetime of spawned windows with automatic cleanup
//

#pragma once
#include <onyxui/widgets/window/window.hh>
#include <vector>
#include <memory>
#include <algorithm>

namespace widgets_demo_windows {

    /**
     * @brief Get global storage for active windows
     * @tparam Backend UI backend type
     * @return Reference to static vector of active windows
     *
     * @details
     * Provides singleton storage for window shared_ptrs, keeping them alive.
     * When windows close, they're automatically removed from this vector.
     */
    template<onyxui::UIBackend Backend>
    inline std::vector<std::shared_ptr<onyxui::window<Backend>>>& get_active_windows() {
        static std::vector<std::shared_ptr<onyxui::window<Backend>>> windows;
        return windows;
    }

    /**
     * @brief Register a window for automatic lifetime management
     * @tparam Backend UI backend type
     * @param win Shared pointer to window to register
     *
     * @details
     * Adds window to global storage and connects to its closed signal for
     * automatic cleanup when the window is closed by the user.
     *
     * Pattern:
     * @code
     * auto win = std::make_shared<window<Backend>>("Title", flags);
     * // ... configure window ...
     * register_window(win);
     * win->show();
     * @endcode
     */
    template<onyxui::UIBackend Backend>
    void register_window(std::shared_ptr<onyxui::window<Backend>> win) {
        auto& windows = get_active_windows<Backend>();
        windows.push_back(win);

        // Auto-cleanup when window closes
        win->closed.connect([win]() {
            auto& windows = get_active_windows<Backend>();
            auto it = std::find(windows.begin(), windows.end(), win);
            if (it != windows.end()) {
                windows.erase(it);
            }
        });
    }

} // namespace widgets_demo_windows
