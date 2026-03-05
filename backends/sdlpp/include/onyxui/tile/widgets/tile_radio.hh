/**
 * @file tile_radio.hh
 * @brief Radio button widget with tile-based rendering
 */

#pragma once

#include <string>
#include <onyxui/widgets/core/stateful_widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>

namespace onyxui::tile {

// Forward declaration for friend class
template<UIBackend Backend>
class tile_radio_group;

/**
 * @class tile_radio
 * @brief Radio button widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_radio displays a radio button using tiles from the atlas.
 * Radio buttons are mutually exclusive - selecting one deselects others
 * in the same group.
 *
 * @example
 * @code
 * auto group = std::make_unique<tile_radio_group<sdlpp_tile_backend>>();
 * group->add_option("Small");
 * group->add_option("Medium");
 * group->add_option("Large");
 * group->set_selected_index(1);  // Default to Medium
 *
 * group->selection_changed.connect([](int index) {
 *     std::cout << "Selected: " << index << "\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_radio : public stateful_widget<Backend> {
public:
    using base = stateful_widget<Backend>;
    using interaction_state = typename base::interaction_state;

    /**
     * @brief Construct a radio button with text
     * @param text Radio button label text
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_radio(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent), m_text(std::move(text))
    {
        this->set_focusable(true);  // Radio buttons are focusable
        this->set_accept_keys_as_click(true);  // Enter/Space selects

        // Connect to base class clicked signal - radio buttons only check, never uncheck
        this->clicked.connect([this]() {
            if (!m_is_checked) {
                set_checked(true);
            }
        });
    }

    /**
     * @brief Destructor
     */
    ~tile_radio() override = default;

    // Rule of Five
    tile_radio(const tile_radio&) = delete;
    tile_radio& operator=(const tile_radio&) = delete;
    tile_radio(tile_radio&&) noexcept = default;
    tile_radio& operator=(tile_radio&&) noexcept = default;

    // ===== State Management =====

    /**
     * @brief Set checked state
     * @param checked true to check, false to uncheck
     *
     * Note: If this radio is in a group, checking it will uncheck others.
     */
    void set_checked(bool checked) {
        if (m_is_checked == checked) {
            return;
        }

        m_is_checked = checked;
        this->mark_dirty();

        // Notify group if checking (for mutual exclusion)
        if (checked && m_group) {
            m_group->notify_radio_checked(this);
        }

        toggled.emit(m_is_checked);
    }

    /**
     * @brief Get checked state
     */
    [[nodiscard]] bool is_checked() const noexcept {
        return m_is_checked;
    }

    // ===== Text Label =====

    /**
     * @brief Set the radio button text
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
     * @brief Set custom radio tiles (overrides theme)
     * @param tiles Radio tile configuration
     */
    void set_tiles(const radio_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's radio tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    /**
     * @brief Set custom font for this radio button
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

    /// Emitted when checked state changes
    signal<bool> toggled;

protected:
    /**
     * @brief Get the tile ID for the current state
     */
    [[nodiscard]] int get_current_tile() const {
        const radio_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().radio;
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

        // Select tile based on checked state
        if (m_is_checked) {
            if (is_hover && tiles->checked_hover >= 0) {
                return tiles->checked_hover;
            }
            return tiles->checked;
        } else {
            if (is_hover && tiles->unchecked_hover >= 0) {
                return tiles->unchecked_hover;
            }
            return tiles->unchecked;
        }
    }

    /**
     * @brief Render the radio button using tile renderer
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

        // Draw radio tile
        const int tile_id = get_current_tile();
        if (tile_id >= 0 && has_theme()) {
            const tile_atlas* atlas = get_theme().atlas;
            if (atlas) {
                tile_renderer::tile_point tile_pos{x, y};
                renderer->draw_tile(tile_id, tile_pos);

                // Draw text after radio button (if any)
                if (!m_text.empty() && font) {
                    const int spacing = 2;  // Space between radio and text
                    tile_renderer::tile_point text_pos{x + atlas->tile_width + spacing, y};
                    renderer->draw_bitmap_text(m_text, text_pos, *font);
                }
            }
        }
    }

    /**
     * @brief Get content size based on radio tile and text
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

        // Radio width = tile + spacing + text
        const int spacing = m_text.empty() ? 0 : 2;
        return logical_size{
            logical_unit(tile_width + spacing + text_width),
            logical_unit(std::max(tile_height, text_height))
        };
    }

private:
    std::string m_text;                              ///< Radio button label text
    bool m_is_checked = false;                       ///< Current checked state
    tile_radio_group<Backend>* m_group = nullptr;    ///< Parent group (for mutual exclusion)
    radio_tiles m_custom_tiles{};                    ///< Custom tiles (if set)
    bool m_use_custom_tiles = false;                 ///< True to use custom tiles
    const bitmap_font* m_custom_font = nullptr;      ///< Custom font (nullptr = use theme)

    friend class tile_radio_group<Backend>;
};

/**
 * @class tile_radio_group
 * @brief Container that manages mutual exclusion for tile_radio buttons
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * tile_radio_group is a vertical container that holds tile_radio buttons
 * and ensures only one can be selected at a time.
 */
template<UIBackend Backend>
class tile_radio_group : public widget_container<Backend> {
public:
    using base = widget_container<Backend>;

    /**
     * @brief Construct a radio group with vertical layout
     * @param parent Parent element (nullptr for none)
     * @param spacing Spacing between radio buttons (default 2)
     */
    explicit tile_radio_group(ui_element<Backend>* parent = nullptr, int spacing = 2)
        : base(
            std::make_unique<linear_layout<Backend>>(
                direction::vertical,
                spacing
            ),
            parent
          )
    {
        this->set_focusable(false);  // Group itself isn't focusable
    }

    /**
     * @brief Destructor
     */
    ~tile_radio_group() override = default;

    // Rule of Five
    tile_radio_group(const tile_radio_group&) = delete;
    tile_radio_group& operator=(const tile_radio_group&) = delete;
    tile_radio_group(tile_radio_group&&) noexcept = default;
    tile_radio_group& operator=(tile_radio_group&&) noexcept = default;

    /**
     * @brief Add a radio button option
     * @param text Label text for the option
     * @return Pointer to the created radio button
     */
    tile_radio<Backend>* add_option(const std::string& text) {
        auto radio = std::make_unique<tile_radio<Backend>>(text);
        radio->m_group = this;
        auto* ptr = radio.get();
        m_radios.push_back(ptr);
        this->add_child(std::move(radio));
        return ptr;
    }

    /**
     * @brief Set the selected radio button by index
     * @param index Zero-based index (-1 for none)
     */
    void set_selected_index(int index) {
        for (std::size_t i = 0; i < m_radios.size(); ++i) {
            m_radios[i]->set_checked(static_cast<int>(i) == index);
        }
    }

    /**
     * @brief Get the selected radio button index
     * @return Zero-based index of selected button, or -1 if none
     */
    [[nodiscard]] int get_selected_index() const {
        for (std::size_t i = 0; i < m_radios.size(); ++i) {
            if (m_radios[i]->is_checked()) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /**
     * @brief Get the number of radio buttons
     */
    [[nodiscard]] std::size_t count() const noexcept {
        return m_radios.size();
    }

    // ===== Signals =====

    /// Emitted when selection changes (parameter is new index)
    signal<int> selection_changed;

private:
    std::vector<tile_radio<Backend>*> m_radios;  ///< Radio buttons in group

    /**
     * @brief Called by tile_radio when it becomes checked
     * @param checked_radio The radio button that was just checked
     */
    void notify_radio_checked(tile_radio<Backend>* checked_radio) {
        // Uncheck all other radios
        for (auto* radio : m_radios) {
            if (radio != checked_radio && radio->is_checked()) {
                radio->m_is_checked = false;  // Direct access to avoid recursion
                radio->mark_dirty();
                radio->toggled.emit(false);
            }
        }

        // Emit selection changed signal
        for (std::size_t i = 0; i < m_radios.size(); ++i) {
            if (m_radios[i] == checked_radio) {
                selection_changed.emit(static_cast<int>(i));
                break;
            }
        }
    }

    friend class tile_radio<Backend>;
};

} // namespace onyxui::tile
