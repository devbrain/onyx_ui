/**
 * @file icon.hh
 * @brief Icon widget for rendering backend-specific glyphs
 * @author Claude Code
 * @date 2025-11-09
 *
 * @details
 * Lightweight widget for rendering icons using backend's icon_style enum.
 * Similar to Qt's QIcon but adapted to OnyxUI's architecture.
 *
 * Icons are rendered using the backend's draw_icon() method, which maps
 * icon_style enum values to appropriate glyphs (Unicode, ASCII, bitmaps).
 */

#pragma once

#include "onyxui/concepts/backend.hh"
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/core/rendering/render_context.hh>

namespace onyxui {

    /**
     * @class icon
     * @brief Widget for rendering backend-specific icon glyphs
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The icon widget renders a single glyph using the backend's icon_style enum.
     * The actual visual representation is determined by the backend:
     * - conio: Unicode glyphs (✓, ×, ↑, ≡, etc.)
     * - SDL2: Rendered graphics
     * - test: Character representation
     *
     * ## Usage
     *
     * @code
     * // Standalone icon
     * auto icon_widget = std::make_unique<icon<Backend>>(icon_style::check);
     *
     * // Icon in button
     * auto icon_content = std::make_unique<icon<Backend>>(icon_style::minimize);
     * button->set_content(std::move(icon_content));
     *
     * // Change icon dynamically
     * icon_widget->set_icon(icon_style::maximize);
     * @endcode
     *
     * ## Theme Integration
     *
     * Icons respect theme colors through inheritance:
     * - Foreground color from parent or theme.text.fg_color
     * - Background color from parent (typically transparent)
     *
     * ## Size
     *
     * Icon size is determined by backend's get_icon_size() method.
     * Most backends return single-character size (1x1 cells in TUI).
     */
    template<UIBackend Backend>
    class icon : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using icon_type = typename Backend::renderer_type::icon_style;
        using render_context_type = render_context<Backend>;
        using size_type = typename Backend::size_type;

        /**
         * @brief Construct icon widget
         * @param style Icon style to display
         */
        explicit icon(icon_type style)
            : base()
            , m_icon_style(style)
        {
        }

        /**
         * @brief Set icon style
         * @param style New icon style
         */
        void set_icon(icon_type style) {
            if (m_icon_style != style) {
                m_icon_style = style;
                this->invalidate_measure();
            }
        }

        /**
         * @brief Get current icon style
         */
        [[nodiscard]] icon_type get_icon() const noexcept {
            return m_icon_style;
        }

    protected:
        /**
         * @brief Render icon using backend's draw_icon() method
         */
        void do_render(render_context_type& ctx) const override {
            if (!this->is_visible()) {
                return;
            }

            // Draw icon at widget's position
            auto bounds = this->bounds();
            typename Backend::point_type position(bounds.x, bounds.y);
            ctx.draw_icon(m_icon_style, position);
        }

    private:
        icon_type m_icon_style;
    };

} // namespace onyxui
