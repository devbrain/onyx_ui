/**
 * @file scroll_view_presets.hh
 * @brief Pre-configured scroll_view variants for common use cases
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/containers/scroll_view.hh>
#include <memory>

namespace onyxui {

    /**
     * @brief Create a modern-style scroll_view with auto-hiding scrollbars
     *
     * @tparam Backend UIBackend implementation
     * @return Pre-configured scroll_view with modern appearance
     *
     * @details
     * Configuration:
     * - Scrollbar visibility: auto_hide (appears when needed, hides when not)
     * - Both vertical and horizontal scrolling enabled
     * - Clean, minimal appearance suitable for modern applications
     *
     * Usage:
     * ```cpp
     * auto view = modern_scroll_view<Backend>();
     * view->add_child(create_content());
     * ```
     */
    template<UIBackend Backend>
    std::unique_ptr<scroll_view<Backend>> modern_scroll_view() {
        auto view = std::make_unique<scroll_view<Backend>>();
        view->set_scrollbar_policy(scrollbar_visibility::auto_hide);
        return view;
    }

    /**
     * @brief Create a classic-style scroll_view with always-visible scrollbars
     *
     * @tparam Backend UIBackend implementation
     * @return Pre-configured scroll_view with classic appearance
     *
     * @details
     * Configuration:
     * - Scrollbar visibility: always visible
     * - Both vertical and horizontal scrolling enabled
     * - Traditional desktop application appearance
     *
     * Usage:
     * ```cpp
     * auto view = classic_scroll_view<Backend>();
     * view->add_child(create_content());
     * ```
     */
    template<UIBackend Backend>
    std::unique_ptr<scroll_view<Backend>> classic_scroll_view() {
        auto view = std::make_unique<scroll_view<Backend>>();
        view->set_scrollbar_policy(scrollbar_visibility::always);
        return view;
    }

    /**
     * @brief Create a compact scroll_view with auto-hiding scrollbars
     *
     * @tparam Backend UIBackend implementation
     * @return Pre-configured scroll_view with compact appearance
     *
     * @details
     * Configuration:
     * - Scrollbar visibility: auto_hide
     * - Both vertical and horizontal scrolling enabled
     * - Space-efficient design for cramped layouts
     *
     * Usage:
     * ```cpp
     * auto view = compact_scroll_view<Backend>();
     * view->add_child(create_content());
     * ```
     */
    template<UIBackend Backend>
    std::unique_ptr<scroll_view<Backend>> compact_scroll_view() {
        auto view = std::make_unique<scroll_view<Backend>>();
        view->set_scrollbar_policy(scrollbar_visibility::auto_hide);
        return view;
    }

    /**
     * @brief Create a vertical-only scroll_view
     *
     * @tparam Backend UIBackend implementation
     * @return Pre-configured scroll_view with vertical scrolling only
     *
     * @details
     * Configuration:
     * - Vertical scrolling: enabled, auto_hide
     * - Horizontal scrolling: disabled
     * - Common pattern for text documents, lists, feeds
     *
     * Usage:
     * ```cpp
     * auto view = vertical_only_scroll_view<Backend>();
     * view->add_child(create_list());
     * ```
     */
    template<UIBackend Backend>
    std::unique_ptr<scroll_view<Backend>> vertical_only_scroll_view() {
        auto view = std::make_unique<scroll_view<Backend>>();
        view->set_horizontal_scroll_enabled(false);
        view->set_vertical_scroll_enabled(true);
        return view;
    }

} // namespace onyxui
