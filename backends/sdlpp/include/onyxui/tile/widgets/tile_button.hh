/**
 * @file tile_button.hh
 * @brief Interactive button widget with tile-based rendering
 */

#pragma once

#include <string>
#include <onyxui/widgets/core/stateful_widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>

namespace onyxui::tile {

/**
 * @class tile_button
 * @brief Interactive button widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_button displays text centered on a nine-slice background.
 * It supports multiple visual states (normal, hover, pressed, disabled, focused)
 * with different tile graphics for each state.
 *
 * @example
 * @code
 * auto btn = std::make_unique<tile_button<sdlpp_tile_backend>>("Click Me");
 * btn->clicked.connect([]() {
 *     std::cout << "Button clicked!\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_button : public stateful_widget<Backend> {
public:
    using base = stateful_widget<Backend>;
    using interaction_state = typename base::interaction_state;

    /**
     * @brief Construct a button with text
     * @param text Button label text
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_button(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent), m_text(std::move(text))
    {
        this->set_focusable(true);  // Buttons are focusable
        this->set_accept_keys_as_click(true);  // Enter/Space triggers click
    }

    /**
     * @brief Destructor
     */
    ~tile_button() override = default;

    // Rule of Five
    tile_button(const tile_button&) = delete;
    tile_button& operator=(const tile_button&) = delete;
    tile_button(tile_button&&) noexcept = default;
    tile_button& operator=(tile_button&&) noexcept = default;

    /**
     * @brief Set the button text
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
     * @brief Set custom button tiles (overrides theme)
     * @param tiles Button tile configuration
     */
    void set_tiles(const button_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's button tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    /**
     * @brief Set custom font for this button
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

protected:
    /**
     * @brief Get the nine-slice for the current state
     */
    [[nodiscard]] const nine_slice& get_current_slice() const {
        const button_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().button;
        }

        if (!tiles) {
            static nine_slice empty{};
            return empty;
        }

        // Select slice based on interaction state
        switch (this->get_interaction_state()) {
            case interaction_state::hover:
                return tiles->hover.is_valid() ? tiles->hover : tiles->normal;
            case interaction_state::pressed:
                return tiles->pressed.is_valid() ? tiles->pressed : tiles->normal;
            case interaction_state::disabled:
                return tiles->disabled.is_valid() ? tiles->disabled : tiles->normal;
            default:
                // Check for focused state
                if (this->has_focus() && tiles->focused.is_valid()) {
                    return tiles->focused;
                }
                return tiles->normal;
        }
    }

    /**
     * @brief Render the button using tile renderer
     */
    void do_render(render_context<Backend>& ctx) const override {
        auto* renderer = get_renderer();
        if (!renderer) {
            return;  // No renderer available
        }

        // Get physical position and size from context
        const auto& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);
        const auto& size = ctx.available_size();
        const int w = size_utils::get_width(size);
        const int h = size_utils::get_height(size);

        // Create absolute rect for tile renderer
        tile_renderer::tile_rect abs_bounds{x, y, w, h};

        // Draw background nine-slice
        const nine_slice& slice = get_current_slice();
        if (slice.is_valid()) {
            renderer->draw_nine_slice(slice, abs_bounds);
        }

        // Draw text centered
        if (!m_text.empty()) {
            const bitmap_font* font = m_custom_font;
            if (!font && has_theme()) {
                // Use disabled font when disabled
                if (this->get_interaction_state() == interaction_state::disabled) {
                    font = &get_theme().font_disabled;
                } else {
                    font = &get_theme().font_normal;
                }
            }
            if (font) {
                renderer->draw_bitmap_text_centered(m_text, abs_bounds, *font);
            }
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

        // Button needs space for text plus some padding
        int text_width = 0;
        int text_height = 0;
        if (font && !m_text.empty()) {
            text_width = font->text_width(m_text);
            text_height = font->glyph_height;
        }

        // Add padding (use slice margins if available)
        int padding_h = 8;  // Default horizontal padding
        int padding_v = 4;  // Default vertical padding

        const button_tiles* tiles = m_use_custom_tiles ? &m_custom_tiles :
            (has_theme() ? &get_theme().button : nullptr);
        if (tiles && tiles->normal.is_valid()) {
            padding_h = tiles->normal.margin_h * 2;
            padding_v = tiles->normal.margin_v * 2;
        }

        return logical_size{
            logical_unit(text_width + padding_h),
            logical_unit(text_height + padding_v)
        };
    }

private:
    std::string m_text;                          ///< Button label text
    button_tiles m_custom_tiles{};               ///< Custom tiles (if set)
    bool m_use_custom_tiles = false;             ///< True to use custom tiles
    const bitmap_font* m_custom_font = nullptr;  ///< Custom font (nullptr = use theme)
};

} // namespace onyxui::tile
