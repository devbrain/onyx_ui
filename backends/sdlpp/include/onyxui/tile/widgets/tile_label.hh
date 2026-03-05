/**
 * @file tile_label.hh
 * @brief Text label widget using bitmap fonts
 */

#pragma once

#include <string>
#include <string_view>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>

namespace onyxui::tile {

/**
 * @class tile_label
 * @brief Text label widget that renders using bitmap fonts
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_label displays text using bitmap fonts from the tile atlas.
 * It's non-focusable and doesn't respond to input events.
 *
 * @example
 * @code
 * auto label = std::make_unique<tile_label<sdlpp_tile_backend>>("Hello World");
 * label->set_text("Updated text");
 * @endcode
 */
template<UIBackend Backend>
class tile_label : public widget<Backend> {
public:
    using base = widget<Backend>;
    using size_type = typename Backend::size_type;

    /**
     * @brief Construct a label with text
     * @param text The text to display
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_label(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent), m_text(std::move(text))
    {
        this->set_focusable(false);  // Labels aren't focusable
    }

    /**
     * @brief Destructor
     */
    ~tile_label() override = default;

    // Rule of Five
    tile_label(const tile_label&) = delete;
    tile_label& operator=(const tile_label&) = delete;
    tile_label(tile_label&&) noexcept = default;
    tile_label& operator=(tile_label&&) noexcept = default;

    /**
     * @brief Set the label text
     * @param text New text to display
     */
    void set_text(const std::string& text) {
        if (m_text != text) {
            m_text = text;
            this->invalidate_measure();  // Size may change
        }
    }

    /**
     * @brief Get the current text
     */
    [[nodiscard]] const std::string& text() const noexcept {
        return m_text;
    }

    /**
     * @brief Set custom font for this label
     * @param font Bitmap font to use (overrides theme)
     */
    void set_font(const bitmap_font* font) {
        m_custom_font = font;
    }

    /**
     * @brief Reset to using theme's default font
     */
    void use_theme_font() {
        m_custom_font = nullptr;
    }

    /**
     * @brief Set whether text should be centered
     * @param centered True to center text within bounds
     */
    void set_centered(bool centered) {
        m_centered = centered;
    }

    /**
     * @brief Check if text is centered
     */
    [[nodiscard]] bool is_centered() const noexcept {
        return m_centered;
    }

protected:
    /**
     * @brief Render the label using tile renderer
     *
     * @details
     * Uses the global tile_renderer to draw bitmap text.
     * The font is taken from the custom font if set, otherwise from the theme.
     */
    void do_render(render_context<Backend>& ctx) const override {
        if (m_text.empty()) {
            return;  // Nothing to render
        }

        auto* renderer = get_renderer();
        if (!renderer) {
            return;  // No renderer available
        }

        // Get the font to use
        const bitmap_font* font = m_custom_font;
        if (!font && has_theme()) {
            font = &get_theme().font_normal;
        }
        if (!font) {
            return;  // No font available
        }

        // Get physical position and size from context
        const auto& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);
        const auto& size = ctx.available_size();
        const int w = size_utils::get_width(size);
        const int h = size_utils::get_height(size);

        if (m_centered) {
            // Create absolute rect for centered text
            tile_renderer::tile_rect abs_bounds{x, y, w, h};
            renderer->draw_bitmap_text_centered(m_text, abs_bounds, *font);
        } else {
            // Draw at top-left
            tile_renderer::tile_point text_pos{x, y};
            renderer->draw_bitmap_text(m_text, text_pos, *font);
        }
    }

    /**
     * @brief Get content size based on text and font
     */
    [[nodiscard]] logical_size get_content_size() const override {
        const bitmap_font* font = m_custom_font;
        if (!font && has_theme()) {
            font = &get_theme().font_normal;
        }

        if (font && !m_text.empty()) {
            return logical_size{
                logical_unit(font->text_width(m_text)),
                logical_unit(font->glyph_height)
            };
        }
        return logical_size{logical_unit(0), logical_unit(0)};
    }

private:
    std::string m_text;                  ///< Text to display
    const bitmap_font* m_custom_font = nullptr;  ///< Custom font (nullptr = use theme)
    bool m_centered = false;             ///< True to center text
};

} // namespace onyxui::tile
