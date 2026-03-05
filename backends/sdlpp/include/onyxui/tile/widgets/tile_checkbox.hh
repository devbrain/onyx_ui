/**
 * @file tile_checkbox.hh
 * @brief Checkbox widget with tile-based rendering
 */

#pragma once

#include <string>
#include <onyxui/widgets/core/stateful_widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>

namespace onyxui::tile {

/// Checkbox state enumeration (two-state or tri-state)
enum class checkbox_state : std::uint8_t {
    unchecked = 0,      ///< Box is empty
    checked = 1,        ///< Box has checkmark
    indeterminate = 2   ///< Box has partial indicator (tri-state mode only)
};

/**
 * @class tile_checkbox
 * @brief Checkbox widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_checkbox displays a checkable box using tiles from the atlas.
 * It supports two-state (unchecked/checked) or optional tri-state mode
 * (unchecked/checked/indeterminate).
 *
 * @example
 * @code
 * auto cb = std::make_unique<tile_checkbox<sdlpp_tile_backend>>("Enable sound");
 * cb->toggled.connect([](bool checked) {
 *     std::cout << "Sound: " << (checked ? "on" : "off") << "\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_checkbox : public stateful_widget<Backend> {
public:
    using base = stateful_widget<Backend>;
    using interaction_state = typename base::interaction_state;

    /**
     * @brief Construct a checkbox with text
     * @param text Checkbox label text
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_checkbox(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent), m_text(std::move(text))
    {
        this->set_focusable(true);  // Checkboxes are focusable
        this->set_accept_keys_as_click(true);  // Enter/Space toggles

        // Connect to base class clicked signal for toggle behavior
        this->clicked.connect([this]() {
            toggle();
        });
    }

    /**
     * @brief Destructor
     */
    ~tile_checkbox() override = default;

    // Rule of Five
    tile_checkbox(const tile_checkbox&) = delete;
    tile_checkbox& operator=(const tile_checkbox&) = delete;
    tile_checkbox(tile_checkbox&&) noexcept = default;
    tile_checkbox& operator=(tile_checkbox&&) noexcept = default;

    // ===== State Management =====

    /**
     * @brief Set checked state (two-state mode)
     * @param checked true to check, false to uncheck
     */
    void set_checked(bool checked) {
        const checkbox_state new_state = checked ? checkbox_state::checked : checkbox_state::unchecked;
        if (m_state == new_state) {
            return;
        }

        m_state = new_state;
        this->mark_dirty();

        toggled.emit(checked);
        state_changed.emit(m_state);
    }

    /**
     * @brief Get checked state
     * @return true if state == checkbox_state::checked
     */
    [[nodiscard]] bool is_checked() const noexcept {
        return m_state == checkbox_state::checked;
    }

    /**
     * @brief Set tri-state value
     * @param state New tri-state value
     */
    void set_state(checkbox_state state) {
        if (!m_tri_state_enabled && state == checkbox_state::indeterminate) {
            return;  // Ignore invalid state
        }

        if (m_state == state) {
            return;
        }

        const checkbox_state old_state = m_state;
        m_state = state;
        this->mark_dirty();

        state_changed.emit(m_state);

        // Emit toggled only for unchecked <-> checked transitions
        if ((old_state == checkbox_state::unchecked && state == checkbox_state::checked) ||
            (old_state == checkbox_state::checked && state == checkbox_state::unchecked)) {
            toggled.emit(state == checkbox_state::checked);
        }
    }

    /**
     * @brief Get current tri-state value
     */
    [[nodiscard]] checkbox_state get_state() const noexcept {
        return m_state;
    }

    /**
     * @brief Enable or disable tri-state mode
     * @param enabled true to enable tri-state mode
     */
    void set_tri_state_enabled(bool enabled) {
        m_tri_state_enabled = enabled;
    }

    /**
     * @brief Check if tri-state mode is enabled
     */
    [[nodiscard]] bool is_tri_state_enabled() const noexcept {
        return m_tri_state_enabled;
    }

    // ===== Text Label =====

    /**
     * @brief Set the checkbox text
     * @param text New text to display
     */
    void set_text(const std::string& text) {
        if (m_text != text) {
            m_text = text;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get the current text
     */
    [[nodiscard]] const std::string& text() const noexcept {
        return m_text;
    }

    /**
     * @brief Set custom checkbox tiles (overrides theme)
     * @param tiles Checkbox tile configuration
     */
    void set_tiles(const checkbox_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's checkbox tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    /**
     * @brief Set custom font for this checkbox
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

    // ===== Signals =====

    /// Emitted when checkbox toggles between unchecked and checked
    signal<bool> toggled;

    /// Emitted on ANY state change (including indeterminate)
    signal<checkbox_state> state_changed;

protected:
    /**
     * @brief Get the tile ID for the current state
     */
    [[nodiscard]] int get_current_tile() const {
        const checkbox_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().checkbox;
        }

        if (!tiles) {
            return -1;
        }

        // Check for disabled state first
        if (this->get_interaction_state() == interaction_state::disabled) {
            if (tiles->disabled >= 0) {
                return tiles->disabled;
            }
        }

        // Check for hover state
        const bool is_hover = this->get_interaction_state() == interaction_state::hover;

        // Select tile based on checkbox state
        switch (m_state) {
            case checkbox_state::unchecked:
                if (is_hover && tiles->unchecked_hover >= 0) {
                    return tiles->unchecked_hover;
                }
                return tiles->unchecked;

            case checkbox_state::checked:
                if (is_hover && tiles->checked_hover >= 0) {
                    return tiles->checked_hover;
                }
                return tiles->checked;

            case checkbox_state::indeterminate:
                return tiles->indeterminate >= 0 ? tiles->indeterminate : tiles->unchecked;
        }

        return tiles->unchecked;
    }

    /**
     * @brief Render the checkbox using tile renderer
     */
    void do_render(render_context<Backend>& ctx) const override {
        auto* renderer = get_renderer();
        if (!renderer) {
            return;
        }

        // Get absolute position from context
        const auto& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);

        // Get the font to use
        const bitmap_font* font = m_custom_font;
        if (!font && has_theme()) {
            if (this->get_interaction_state() == interaction_state::disabled) {
                font = &get_theme().font_disabled;
            } else {
                font = &get_theme().font_normal;
            }
        }

        // Draw checkbox tile
        const int tile_id = get_current_tile();
        if (tile_id >= 0 && has_theme()) {
            const tile_atlas* atlas = get_theme().atlas;
            if (atlas) {
                tile_renderer::tile_point tile_pos{x, y};
                renderer->draw_tile(tile_id, tile_pos);

                // Draw text after checkbox (if any)
                if (!m_text.empty() && font) {
                    const int spacing = 2;  // Space between checkbox and text
                    tile_renderer::tile_point text_pos{x + atlas->tile_width + spacing, y};
                    renderer->draw_bitmap_text(m_text, text_pos, *font);
                }
            }
        }
    }

    /**
     * @brief Get content size based on checkbox tile and text
     */
    [[nodiscard]] logical_size get_content_size() const override {
        int tile_width = 16;  // Default tile size
        int tile_height = 16;

        if (has_theme() && get_theme().atlas) {
            tile_width = get_theme().atlas->tile_width;
            tile_height = get_theme().atlas->tile_height;
        }

        const bitmap_font* font = m_custom_font;
        if (!font && has_theme()) {
            font = &get_theme().font_normal;
        }

        int text_width = 0;
        int text_height = 0;
        if (font && !m_text.empty()) {
            text_width = font->text_width(m_text);
            text_height = font->glyph_height;
        }

        // Checkbox width = tile + spacing + text
        const int spacing = m_text.empty() ? 0 : 2;
        return logical_size{
            logical_unit(tile_width + spacing + text_width),
            logical_unit(std::max(tile_height, text_height))
        };
    }

private:
    std::string m_text;                              ///< Checkbox label text
    checkbox_state m_state = checkbox_state::unchecked;  ///< Current state
    bool m_tri_state_enabled = false;                ///< Allow indeterminate state?
    checkbox_tiles m_custom_tiles{};                 ///< Custom tiles (if set)
    bool m_use_custom_tiles = false;                 ///< True to use custom tiles
    const bitmap_font* m_custom_font = nullptr;      ///< Custom font (nullptr = use theme)

    /**
     * @brief Toggle to next state
     */
    void toggle() {
        if (m_tri_state_enabled && m_state == checkbox_state::indeterminate) {
            // Indeterminate -> Checked
            set_state(checkbox_state::checked);
        } else if (m_state == checkbox_state::unchecked) {
            // Unchecked -> Checked
            set_checked(true);
        } else {
            // Checked -> Unchecked
            set_checked(false);
        }
    }
};

} // namespace onyxui::tile
