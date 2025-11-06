/**
 * @file scroll_controller.hh
 * @brief Coordination layer binding scrollable logic with scrollbar visuals
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/containers/scroll/scrollable.hh>
#include <onyxui/widgets/containers/scroll/scrollbar.hh>
#include <onyxui/core/signal.hh>

#include <vector>

namespace onyxui {

    /**
     * @class scroll_controller
     * @brief Coordinates scrollable container with visual scrollbars
     *
     * @details
     * The scroll_controller is the glue layer that binds the scrollable
     * (logic layer) with scrollbar widgets (visual layer), providing
     * bidirectional synchronization:
     *
     * Architecture:
     * ```
     * scrollable (logic)          scroll_controller          scrollbar (visual)
     *        │                           │                           │
     *        │   scroll_changed          │                           │
     *        ├──────────────────────────>│                           │
     *        │                           │   set_scroll_info()       │
     *        │                           ├──────────────────────────>│
     *        │                           │                           │
     *        │                           │   scroll_requested        │
     *        │                           │<──────────────────────────┤
     *        │   scroll_to()             │                           │
     *        │<──────────────────────────┤                           │
     * ```
     *
     * Responsibilities:
     * 1. **Scrollable → Scrollbar**: Update thumb position/size when content scrolls
     * 2. **Scrollbar → Scrollable**: Update scroll offset when user drags thumb
     * 3. **Visibility**: Show/hide scrollbars based on policy and content size
     * 4. **Cleanup**: Automatic signal disconnection via RAII (scoped_connection)
     *
     * Loop Prevention:
     * - Uses guard flags to prevent infinite signal loops
     * - Example: scrollbar drag → update scrollable → scroll_changed → (blocked)
     *
     * Ownership:
     * - Does NOT own scrollable or scrollbars (they're owned by parent container)
     * - Stores raw pointers with lifetime guarantee (controller destroyed first)
     * - Uses scoped_connection for automatic cleanup
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scroll_controller {
    public:
        using scrollable_type = scrollable<Backend>;
        using scrollbar_type = scrollbar<Backend>;
        using scroll_info_type = scroll_info<Backend>;
        using point_type = typename Backend::point_type;

        /**
         * @brief Construct controller with scrollable and optional scrollbars
         * @param scrollable_ptr Pointer to scrollable container (required)
         * @param vscrollbar_ptr Pointer to vertical scrollbar (optional)
         * @param hscrollbar_ptr Pointer to horizontal scrollbar (optional)
         *
         * @note Pointers must remain valid for controller's lifetime
         * @note Controller should be destroyed before widgets
         */
        scroll_controller(
            scrollable_type* scrollable_ptr,
            scrollbar_type* vscrollbar_ptr = nullptr,
            scrollbar_type* hscrollbar_ptr = nullptr
        )
            : m_scrollable(scrollable_ptr)
            , m_vscrollbar(vscrollbar_ptr)
            , m_hscrollbar(hscrollbar_ptr)
        {
            if (!m_scrollable) {
                return;  // Invalid configuration, but don't crash
            }

            // Connect scrollable → scrollbar (content changes update thumbs)
            m_connections.emplace_back(
                m_scrollable->scroll_changed,
                [this](const point_type& offset) {
                    if (m_updating_from_scrollbar) {
                        return;  // Prevent infinite loop
                    }
                    on_scrollable_scrolled(offset);
                }
            );

            m_connections.emplace_back(
                m_scrollable->content_size_changed,
                [this](const auto& size) {
                    (void)size;
                    update_scrollbar_info();
                    update_scrollbar_visibility();
                }
            );

            // Connect scrollable visibility policy changes → update scrollbar widgets
            m_connections.emplace_back(
                m_scrollable->scrollbar_visibility_changed,
                [this](bool h_visible, bool v_visible) {
                    (void)h_visible;
                    (void)v_visible;
                    // CRITICAL FIX: Also update scroll info because viewport size has changed!
                    // When scrollable::do_arrange() sets m_viewport_size, it emits this signal.
                    // We need to update scrollbars with the new viewport size.
                    update_scrollbar_info();
                    update_scrollbar_visibility();
                }
            );

            // Connect vertical scrollbar → scrollable (thumb drag updates content)
            if (m_vscrollbar) {
                m_connections.emplace_back(
                    m_vscrollbar->scroll_requested,
                    [this](int new_offset) {
                        if (m_updating_from_scrollable) {
                            return;  // Prevent infinite loop
                        }
                        on_vscrollbar_changed(new_offset);
                    }
                );
            }

            // Connect horizontal scrollbar → scrollable (thumb drag updates content)
            if (m_hscrollbar) {
                m_connections.emplace_back(
                    m_hscrollbar->scroll_requested,
                    [this](int new_offset) {
                        if (m_updating_from_scrollable) {
                            return;  // Prevent infinite loop
                        }
                        on_hscrollbar_changed(new_offset);
                    }
                );
            }

            // Initial sync
            update_scrollbar_info();
            update_scrollbar_visibility();
        }

        /**
         * @brief Destructor - automatically disconnects signals via scoped_connection
         */
        ~scroll_controller() = default;

        // Delete copy/move to simplify lifetime management
        scroll_controller(const scroll_controller&) = delete;
        scroll_controller& operator=(const scroll_controller&) = delete;
        scroll_controller(scroll_controller&&) = delete;
        scroll_controller& operator=(scroll_controller&&) = delete;

        /**
         * @brief Force update of scrollbar info and visibility
         * @note Useful after manual scrollable configuration changes
         */
        void refresh() {
            update_scrollbar_info();
            update_scrollbar_visibility();
        }

    private:
        /**
         * @brief Handle scrollable scroll_changed event
         * @param offset New scroll offset from scrollable
         */
        void on_scrollable_scrolled([[maybe_unused]] const point_type& offset) {
            m_updating_from_scrollable = true;
            update_scrollbar_info();
            m_updating_from_scrollable = false;
        }

        /**
         * @brief Handle vertical scrollbar scroll_requested event
         * @param new_y_offset New Y scroll position from scrollbar
         */
        void on_vscrollbar_changed(int new_y_offset) {
            if (!m_scrollable) return;

            m_updating_from_scrollbar = true;

            auto const current_offset = m_scrollable->get_scroll_offset();
            int const x = point_utils::get_x(current_offset);

            m_scrollable->scroll_to(x, new_y_offset);

            m_updating_from_scrollbar = false;

            // CRITICAL FIX: Update scrollbar info after scrolling!
            // The guard prevents on_scrollable_scrolled from updating,
            // so we must update manually after resetting the guard.
            update_scrollbar_info();
        }

        /**
         * @brief Handle horizontal scrollbar scroll_requested event
         * @param new_x_offset New X scroll position from scrollbar
         */
        void on_hscrollbar_changed(int new_x_offset) {
            if (!m_scrollable) return;

            m_updating_from_scrollbar = true;

            auto const current_offset = m_scrollable->get_scroll_offset();
            int const y = point_utils::get_y(current_offset);

            m_scrollable->scroll_to(new_x_offset, y);

            m_updating_from_scrollbar = false;

            // CRITICAL FIX: Update scrollbar info after scrolling!
            // The guard prevents on_scrollable_scrolled from updating,
            // so we must update manually after resetting the guard.
            update_scrollbar_info();
        }

        /**
         * @brief Update scrollbar scroll_info from scrollable state
         */
        void update_scrollbar_info() {
            if (!m_scrollable) return;

            auto const info = m_scrollable->get_scroll_info();

            if (m_vscrollbar) {
                m_vscrollbar->set_scroll_info(info);
            }

            if (m_hscrollbar) {
                m_hscrollbar->set_scroll_info(info);
            }
        }

        /**
         * @brief Update scrollbar visibility based on content size
         * @note Uses scrollable's visibility policy
         */
        void update_scrollbar_visibility() {
            if (!m_scrollable) return;

            bool const show_v = m_scrollable->should_show_vertical_scrollbar();
            bool const show_h = m_scrollable->should_show_horizontal_scrollbar();

            if (m_vscrollbar) {
                m_vscrollbar->set_visible(show_v);
            }

            if (m_hscrollbar) {
                m_hscrollbar->set_visible(show_h);
            }
        }

        scrollable_type* m_scrollable = nullptr;       ///< Scrollable container (non-owning)
        scrollbar_type* m_vscrollbar = nullptr;        ///< Vertical scrollbar (non-owning)
        scrollbar_type* m_hscrollbar = nullptr;        ///< Horizontal scrollbar (non-owning)

        std::vector<scoped_connection> m_connections;  ///< RAII signal connections

        bool m_updating_from_scrollable = false;       ///< Guard: scrollable → scrollbar update in progress
        bool m_updating_from_scrollbar = false;        ///< Guard: scrollbar → scrollable update in progress
    };

} // namespace onyxui
