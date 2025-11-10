/**
 * @file window_content_area.inl
 * @brief Implementation of window_content_area class
 */

#pragma once

#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/layout/linear_layout.hh>

namespace onyxui {

    template<UIBackend Backend>
    window_content_area<Backend>::window_content_area(
        bool scrollable,
        ui_element<Backend>* parent
    )
        : base(
            std::make_unique<linear_layout<Backend>>(
                direction::vertical,
                0  // No spacing for content area - just wraps single child
            ),
            parent
          )
        , m_scrollable(scrollable)
    {
        if (m_scrollable) {
            // Create scroll_view to wrap content
            m_scroll_view = this->template emplace_child<scroll_view>();
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

        // Phase 6: Implement scrollable mode switching
        if (m_scrollable) {
            // Switching to scrollable: create scroll_view and move content into it
            if (!m_scroll_view) {
                m_scroll_view = this->template emplace_child<scroll_view<Backend>>();
            }

            // If we have content, move it from direct child to scroll_view
            if (m_content) {
                // Remove content from direct children
                auto content_unique = this->remove_child(m_content);
                if (content_unique) {
                    // Add to scroll_view
                    m_scroll_view->add_child(std::move(content_unique));
                }
            }
        } else {
            // Switching to non-scrollable: remove scroll_view and move content out
            if (m_scroll_view && m_content) {
                // Remove content from scroll_view
                auto content_unique = m_scroll_view->remove_child(m_content);
                if (content_unique) {
                    // Add as direct child
                    this->add_child(std::move(content_unique));
                }
            }

            // Remove scroll_view
            if (m_scroll_view) {
                [[maybe_unused]] auto removed = this->remove_child(m_scroll_view);
                m_scroll_view = nullptr;
            }
        }

        this->invalidate_measure();
    }

    template<UIBackend Backend>
    void window_content_area<Backend>::do_render(render_context_type& ctx) const {
        if (!this->is_visible()) {
            return;
        }

        // Phase 8: Draw content area background (uses theme.window.content_background)
        // Rendering order: do_render() is called BEFORE children by framework (element.hh:651)
        // This fill provides the background, children render on top automatically

        // RELATIVE COORDINATES: Convert relative bounds to absolute for rendering
        // ctx.position() contains absolute screen position (x,y)
        // this->bounds() contains relative bounds (w,h from parent's content area)
        typename Backend::rect_type absolute_bounds;
        rect_utils::make_absolute_bounds(absolute_bounds, ctx.position(), this->bounds());
        ctx.fill_rect(absolute_bounds);

        // Draw border if enabled (via widget_container base class)
        base::do_render(ctx);

        // Children (content or scroll_view) render automatically via framework
    }

    template<UIBackend Backend>
    resolved_style<Backend> window_content_area<Backend>::get_theme_style(const theme_type& theme) const {
        // Content area uses window content background
        return resolved_style<Backend>{
            .background_color = theme.window.content_background,
            .foreground_color = theme.text_fg,
            .mnemonic_foreground = theme.text_fg,
            .border_color = theme.border_color,
            .box_style = typename Backend::renderer_type::box_style{},  // Content area has no border
            .font = theme.label.font,
            .opacity = 1.0f,
            .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<typename Backend::renderer_type::font>{},
            .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}
        };
    }

} // namespace onyxui
