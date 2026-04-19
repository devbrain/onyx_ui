/**
 * @file tooltip_widget.hh
 * @brief Tooltip popup widget with proper visual styling
 *
 * @details
 * Renders as a yellow-background box with a thin black border and text.
 * Used by widget::set_tooltip() and list_box::set_tooltip_provider().
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/concepts/size_like.hh>
#include <string>

namespace onyxui {

    template<UIBackend Backend>
    class tooltip_widget : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using render_context_type = render_context<Backend>;
        using color_type = typename Backend::color_type;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;

        explicit tooltip_widget(std::string text)
            : base(nullptr), m_text(std::move(text)) {
            this->set_focusable(false);
        }

        void set_text(const std::string& text) {
            m_text = text;
            this->invalidate_measure();
        }

        [[nodiscard]] const std::string& text() const noexcept {
            return m_text;
        }

    protected:
        void do_render(render_context_type& ctx) const override {
            constexpr int padding_h = 6;
            constexpr int padding_v = 3;
            constexpr int border = 1;

            // Measure text
            auto* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;
            auto font = theme ? theme->label.font : typename renderer_type::font{};
            auto text_size = renderer_type::measure_text(m_text, font);
            int const text_w = size_utils::get_width(text_size);
            int const text_h = size_utils::get_height(text_size);

            int const natural_w = text_w + (padding_h + border) * 2;
            int const natural_h = text_h + (padding_v + border) * 2;
            auto const [final_w, final_h] = ctx.get_final_dims(natural_w, natural_h);

            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            // Draw yellow background
            rect_type bg_rect{x, y, final_w, final_h};
            ctx.fill_rect(bg_rect, m_bg_color);

            // Draw black border (4 lines)
            rect_type top{x, y, final_w, border};
            rect_type bottom{x, y + final_h - border, final_w, border};
            rect_type left{x, y, border, final_h};
            rect_type right_edge{x + final_w - border, y, border, final_h};
            ctx.fill_rect(top, m_border_color);
            ctx.fill_rect(bottom, m_border_color);
            ctx.fill_rect(left, m_border_color);
            ctx.fill_rect(right_edge, m_border_color);

            // Draw text
            int const text_x = x + border + padding_h;
            int const text_y = y + border + padding_v;
            point_type text_pos{text_x, text_y};
            (void)ctx.draw_text(m_text, text_pos, font, m_fg_color);
        }

    private:
        std::string m_text;
        color_type m_bg_color{255, 255, 225};    // light yellow
        color_type m_fg_color{0, 0, 0};          // black text
        color_type m_border_color{0, 0, 0};      // black border
    };

} // namespace onyxui
