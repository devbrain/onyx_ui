/**
 * @file scrollbar_arrow.hh
 * @brief Arrow button widget for scrollbar
 * @author OnyxUI Framework
 * @date 2025-11-06
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/theming/theme.hh>
#include <onyxui/core/orientation.hh>

namespace onyxui {

    /**
     * @enum arrow_state
     * @brief Visual states for scrollbar arrow
     */
    enum class arrow_state {
        normal,    ///< Default appearance
        hover,     ///< Mouse hovering over arrow
        pressed,   ///< Arrow being clicked
        disabled   ///< Parent scrollbar disabled
    };

    /**
     * @enum arrow_direction
     * @brief Direction arrow points
     */
    enum class arrow_direction {
        up,     ///< Upward pointing (vertical scroll decrement)
        down,   ///< Downward pointing (vertical scroll increment)
        left,   ///< Leftward pointing (horizontal scroll decrement)
        right   ///< Rightward pointing (horizontal scroll increment)
    };

    /**
     * @class scrollbar_arrow
     * @brief Clickable arrow button for line scrolling
     *
     * @details
     * Represents increment/decrement arrow buttons on scrollbars.
     *
     * Styling:
     * - Automatically resolves colors from theme->scrollbar.arrow_{state}
     * - Supports hover, pressed, disabled states
     * - Renders background fill + centered arrow icon
     *
     * Parent Interaction:
     * - Parent scrollbar positions arrow via arrange()
     * - Parent handles click events and emits scroll_requested
     * - Arrow only handles visual state (hover/pressed)
     *
     * Icon Selection:
     * - Direction determines which theme icon to use
     * - theme->scrollbar.arrow_decrement_icon (up/left)
     * - theme->scrollbar.arrow_increment_icon (down/right)
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scrollbar_arrow : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;
        using size_type = typename Backend::size_type;
        using theme_type = ui_theme<Backend>;
        using icon_type = typename Backend::renderer_type::icon_style;
        using renderer_type = typename Backend::renderer_type;

        /**
         * @brief Construct arrow button
         * @param dir Direction arrow points
         */
        explicit scrollbar_arrow(arrow_direction dir)
            : m_direction(dir)
        {
        }

        /**
         * @brief Set arrow visual state
         * @param state New state (normal/hover/pressed/disabled)
         */
        void set_state(arrow_state state) {
            if (m_state == state) {
                return;
            }

            m_state = state;
            this->mark_dirty();  // Visual state changed
        }

        /**
         * @brief Get current arrow state
         * @return Current visual state
         */
        [[nodiscard]] arrow_state get_state() const noexcept {
            return m_state;
        }

        /**
         * @brief Get arrow direction
         * @return Direction arrow points
         */
        [[nodiscard]] arrow_direction get_direction() const noexcept {
            return m_direction;
        }

        /**
         * @brief Override style resolution to use arrow colors from theme
         * @param theme Global theme pointer
         * @param parent_style Parent's resolved style
         * @return Resolved style with arrow colors
         */
        [[nodiscard]] resolved_style<Backend> resolve_style(
            const theme_type* theme,
            const resolved_style<Backend>& parent_style
        ) const override {
            auto style = base::resolve_style(theme, parent_style);

            if (theme) {
                auto const& arrow_style = get_current_arrow_style(*theme);
                style.foreground_color = arrow_style.foreground;
                style.background_color = arrow_style.background;
            }

            return style;
        }

    protected:
        /**
         * @brief Render arrow button (background + centered icon)
         * @param ctx Render context (measurement or drawing)
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto const bounds = this->bounds();
            auto const* theme = ctx.theme();

            int const width = rect_utils::get_width(bounds);
            int const height = rect_utils::get_height(bounds);

            // Don't render if arrow has no size
            if (width <= 0 || height <= 0 || !theme) {
                return;
            }

            // Get absolute position for rendering
            int const base_x = point_utils::get_x(ctx.position());
            int const base_y = point_utils::get_y(ctx.position());

            // Create absolute bounds for rendering
            rect_type abs_bounds;
            rect_utils::set_bounds(abs_bounds, base_x, base_y, width, height);

            // 1. Background fill (uses arrow box_style from theme)
            auto const& arrow_style = get_current_arrow_style(*theme);
            ctx.draw_rect(abs_bounds, arrow_style.box_style);

            // 2. Draw arrow icon (centered in bounds)
            icon_type const icon = get_icon_for_direction(*theme);
            auto const icon_size = renderer_type::get_icon_size(icon);

            int const icon_w = size_utils::get_width(icon_size);
            int const icon_h = size_utils::get_height(icon_size);

            // Calculate centered position
            int const icon_x = base_x + (width - icon_w) / 2;
            int const icon_y = base_y + (height - icon_h) / 2;

            point_type const icon_pos{icon_x, icon_y};
            ctx.draw_icon(icon, icon_pos);
        }

    private:
        arrow_direction m_direction;
        arrow_state m_state = arrow_state::normal;

        /**
         * @brief Get theme style for current state
         * @param theme Theme containing style definitions
         * @return Component style for current state
         */
        [[nodiscard]] auto const& get_current_arrow_style(const theme_type& theme) const {
            switch (m_state) {
                case arrow_state::disabled:
                    // Fall back to normal state for disabled (theme doesn't have arrow_disabled)
                    return theme.scrollbar.arrow_normal;
                case arrow_state::pressed:
                    return theme.scrollbar.arrow_pressed;
                case arrow_state::hover:
                    return theme.scrollbar.arrow_hover;
                default:
                    return theme.scrollbar.arrow_normal;
            }
        }

        /**
         * @brief Get icon based on arrow direction
         * @param theme Theme containing icon definitions
         * @return Icon to render
         */
        [[nodiscard]] icon_type get_icon_for_direction(const theme_type& theme) const {
            switch (m_direction) {
                case arrow_direction::up:
                    return theme.scrollbar.arrow_up_icon;
                case arrow_direction::down:
                    return theme.scrollbar.arrow_down_icon;
                case arrow_direction::left:
                    return theme.scrollbar.arrow_left_icon;
                case arrow_direction::right:
                    return theme.scrollbar.arrow_right_icon;
            }

            // Unreachable, but satisfy compiler
            return theme.scrollbar.arrow_up_icon;
        }
    };

} // namespace onyxui
