/**
 * @file window_content_area.inl
 * @brief Implementation of window_content_area class
 */

#pragma once

#include <onyxui/widgets/containers/scroll_view.hh>

namespace onyxui {

    template<UIBackend Backend>
    window_content_area<Backend>::window_content_area(
        bool scrollable,
        ui_element<Backend>* parent
    )
        : base(parent)
        , m_scrollable(scrollable)
    {
        if (m_scrollable) {
            // Create scroll_view to wrap content
            m_scroll_view = this->template emplace_child<scroll_view<Backend>>();
        }
    }

    template<UIBackend Backend>
    void window_content_area<Backend>::set_content(std::unique_ptr<ui_element<Backend>> content) {
        // Remove existing content if any
        if (m_content) {
            if (m_scrollable && m_scroll_view) {
                // Content is inside scroll_view, remove from there
                // Note: scroll_view will handle child removal
            } else {
                // Content is direct child, remove it
                [[maybe_unused]] auto removed = this->remove_child(m_content);
            }
            m_content = nullptr;
        }

        if (!content) {
            return;  // No new content
        }

        // Add new content
        if (m_scrollable && m_scroll_view) {
            // Add to scroll_view
            m_content = content.get();
            m_scroll_view->add_child(std::move(content));
        } else {
            // Add as direct child
            m_content = content.get();
            this->add_child(std::move(content));
        }

        this->invalidate_measure();
    }

    template<UIBackend Backend>
    void window_content_area<Backend>::set_scrollable(bool scrollable) {
        if (m_scrollable == scrollable) {
            return;  // No change
        }

        m_scrollable = scrollable;

        // TODO Phase 6: Implement scrollable mode switching
        // For Phase 1: scrollable flag is set at construction and doesn't change

        this->invalidate_measure();
    }

    template<UIBackend Backend>
    void window_content_area<Backend>::do_render(render_context_type& ctx) const {
        if (!this->is_visible()) {
            return;
        }

        // Phase 1: Content area background
        // Children (content or scroll_view) render automatically via framework
    }

} // namespace onyxui
