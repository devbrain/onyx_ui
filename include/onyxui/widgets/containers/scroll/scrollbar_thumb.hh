/**
 * @file scrollbar_thumb.hh
 * @brief Draggable thumb widget for scrollbar
 * @author OnyxUI Framework
 * @date 2025-11-06
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/theming/theme.hh>

namespace onyxui {

    /**
     * @enum thumb_state
     * @brief Visual states for scrollbar thumb
     */
    enum class thumb_state {
        normal,    ///< Default appearance
        hover,     ///< Mouse hovering over thumb
        pressed,   ///< Thumb being dragged
        disabled   ///< Parent scrollbar disabled
    };

    /**
     * @class scrollbar_thumb
     * @brief Draggable thumb indicator widget
     *
     * @details
     * Represents the draggable portion of a scrollbar that indicates:
     * - Current scroll position (thumb location)
     * - Viewport size relative to content (thumb size)
     *
     * Styling:
     * - Automatically resolves colors from theme->scrollbar.thumb_{state}
     * - Supports hover, pressed, disabled states
     * - Renders as simple solid fill (no border for 1-char text UI)
     *
     * Parent Interaction:
     * - Parent scrollbar positions thumb via arrange()
     * - Parent handles drag logic and emits scroll_requested
     * - Thumb only handles visual state (hover/pressed)
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scrollbar_thumb : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using rect_type = typename Backend::rect_type;
        using theme_type = ui_theme<Backend>;

        /**
         * @brief Construct thumb widget
         */
        scrollbar_thumb() = default;

        /**
         * @brief Set thumb visual state
         * @param state New state (normal/hover/pressed/disabled)
         */
        void set_state(thumb_state state) {
            if (m_state == state) {
                return;
            }

            m_state = state;
            this->mark_dirty();  // Visual state changed
        }

        /**
         * @brief Get current thumb state
         * @return Current visual state
         */
        [[nodiscard]] thumb_state get_state() const noexcept {
            return m_state;
        }

        /**
         * @brief Override style resolution to use thumb colors from theme
         * @param theme Global theme pointer
         * @param parent_style Parent's resolved style
         * @return Resolved style with thumb colors
         */
        [[nodiscard]] resolved_style<Backend> resolve_style(
            const theme_type* theme,
            const resolved_style<Backend>& parent_style
        ) const override {
            auto style = base::resolve_style(theme, parent_style);

            if (theme) {
                auto const& thumb_style = get_current_thumb_style(*theme);
                style.foreground_color = thumb_style.foreground;
                style.background_color = thumb_style.background;
            }

            return style;
        }

    protected:
        /**
         * @brief Render thumb as simple solid fill
         * @param ctx Render context (measurement or drawing)
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Simple solid fill - no border for 1-char thumb in text UI
            // ctx.style() contains our resolved foreground/background colors

            // Get physical position from render context
            int const x = point_utils::get_x(ctx.position());
            int const y = point_utils::get_y(ctx.position());

            // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
            auto logical_bounds = this->bounds();
            auto const [width, height] = ctx.get_final_dims(
                logical_bounds.width.to_int(), logical_bounds.height.to_int());

            // Don't render if thumb has no size
            if (width <= 0 || height <= 0) {
                return;
            }

            // Create absolute bounds for rendering (physical coordinates)
            rect_type abs_bounds;
            rect_utils::set_bounds(abs_bounds, x, y, width, height);

            // For text UI, we fill with a solid color
            // For GUI, this could draw a more complex thumb with gradients/shadows
            // Use the component_style's box_style from theme
            auto const* theme = ctx.theme();
            if (theme) {
                auto const& thumb_style = get_current_thumb_style(*theme);
                ctx.draw_rect(abs_bounds, thumb_style.box_style);
            }
        }

    private:
        thumb_state m_state = thumb_state::normal;

        /**
         * @brief Get theme style for current state
         * @param theme Theme containing style definitions
         * @return Component style for current state
         */
        [[nodiscard]] auto const& get_current_thumb_style(const theme_type& theme) const {
            switch (m_state) {
                case thumb_state::disabled:
                    return theme.scrollbar.thumb_disabled;
                case thumb_state::pressed:
                    return theme.scrollbar.thumb_pressed;
                case thumb_state::hover:
                    return theme.scrollbar.thumb_hover;
                default:
                    return theme.scrollbar.thumb_normal;
            }
        }
    };

} // namespace onyxui
