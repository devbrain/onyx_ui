/**
 * @file tile_combo_box.hh
 * @brief Combo box / dropdown widget with tile-based rendering
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>
#include <onyxui/events/ui_event.hh>

namespace onyxui::tile {

/**
 * @class tile_combo_box
 * @brief Dropdown selection widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_combo_box displays a button with the currently selected item
 * and opens a dropdown popup to select from available items.
 *
 * ## Features
 * - Button with selected item text and dropdown arrow
 * - Popup list with hover highlighting
 * - Keyboard navigation (arrows, Enter, Escape)
 * - Mouse selection
 *
 * @example
 * @code
 * auto combo = std::make_unique<tile_combo_box<sdlpp_tile_backend>>();
 * combo->add_item("Small");
 * combo->add_item("Medium");
 * combo->add_item("Large");
 * combo->set_current_index(1);  // Select "Medium"
 *
 * combo->current_index_changed.connect([](int index) {
 *     std::cout << "Selected: " << index << "\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_combo_box : public widget<Backend> {
public:
    using base = widget<Backend>;

    /**
     * @brief Construct an empty combo box
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_combo_box(ui_element<Backend>* parent = nullptr)
        : base(parent)
    {
        this->set_focusable(true);
        this->set_accept_keys_as_click(true);
        this->clicked.connect([this]() {
            toggle_popup();
        });
    }

    /**
     * @brief Destructor
     */
    ~tile_combo_box() override = default;

    // Rule of Five
    tile_combo_box(const tile_combo_box&) = delete;
    tile_combo_box& operator=(const tile_combo_box&) = delete;
    tile_combo_box(tile_combo_box&&) noexcept = default;
    tile_combo_box& operator=(tile_combo_box&&) noexcept = default;

    // ===== Item Management =====

    /**
     * @brief Add an item to the combo box
     * @param item Item text to add
     */
    void add_item(const std::string& item) {
        m_items.push_back(item);
        this->invalidate_measure();
    }

    /**
     * @brief Insert an item at a specific position
     * @param index Position to insert at
     * @param item Item text to insert
     */
    void insert_item(int index, const std::string& item) {
        if (index >= 0 && index <= static_cast<int>(m_items.size())) {
            m_items.insert(m_items.begin() + index, item);
            if (m_current_index >= index) {
                m_current_index++;
            }
            this->invalidate_measure();
        }
    }

    /**
     * @brief Remove an item at a specific position
     * @param index Position to remove
     */
    void remove_item(int index) {
        if (index >= 0 && index < static_cast<int>(m_items.size())) {
            int old_index = m_current_index;
            std::string old_text = current_text();

            m_items.erase(m_items.begin() + index);

            // Adjust current index if needed
            if (m_current_index == index) {
                // Selected item was removed - select previous or -1 if empty
                m_current_index = m_items.empty() ? -1 :
                    std::min(m_current_index, static_cast<int>(m_items.size()) - 1);
            } else if (m_current_index > index) {
                // Selection was after removed item - shift down
                m_current_index--;
            }

            this->invalidate_measure();
            this->mark_dirty();

            // Emit signals if selection changed
            if (m_current_index != old_index) {
                current_index_changed.emit(m_current_index);
            }
            std::string new_text = current_text();
            if (new_text != old_text) {
                current_text_changed.emit(new_text);
            }
        }
    }

    /**
     * @brief Clear all items
     */
    void clear() {
        m_items.clear();
        m_current_index = -1;
        m_hover_index = -1;
        this->invalidate_measure();
        this->mark_dirty();
    }

    /**
     * @brief Get item at index
     * @param index Item index
     * @return Item text, or empty string if invalid index
     */
    [[nodiscard]] std::string item_at(int index) const {
        if (index >= 0 && index < static_cast<int>(m_items.size())) {
            return m_items[static_cast<std::size_t>(index)];
        }
        return "";
    }

    /**
     * @brief Get number of items
     */
    [[nodiscard]] int count() const noexcept {
        return static_cast<int>(m_items.size());
    }

    // ===== Selection =====

    /**
     * @brief Set current selection by index
     * @param index Index to select (-1 for no selection)
     */
    void set_current_index(int index) {
        int new_index = std::clamp(index, -1, static_cast<int>(m_items.size()) - 1);
        if (m_current_index != new_index) {
            m_current_index = new_index;
            this->mark_dirty();
            current_index_changed.emit(m_current_index);
            if (m_current_index >= 0) {
                current_text_changed.emit(m_items[static_cast<std::size_t>(m_current_index)]);
            }
        }
    }

    /**
     * @brief Get current selection index
     * @return Selected index, or -1 if none
     */
    [[nodiscard]] int current_index() const noexcept {
        return m_current_index;
    }

    /**
     * @brief Get current selection text
     * @return Selected item text, or empty string if none
     */
    [[nodiscard]] std::string current_text() const {
        if (m_current_index >= 0 && m_current_index < static_cast<int>(m_items.size())) {
            return m_items[static_cast<std::size_t>(m_current_index)];
        }
        return "";
    }

    /**
     * @brief Find and select item by text
     * @param text Text to search for
     * @return true if found and selected
     */
    bool set_current_text(const std::string& text) {
        for (std::size_t i = 0; i < m_items.size(); ++i) {
            if (m_items[i] == text) {
                set_current_index(static_cast<int>(i));
                return true;
            }
        }
        return false;
    }

    // ===== Popup State =====

    /**
     * @brief Check if popup is currently open
     */
    [[nodiscard]] bool is_popup_open() const noexcept {
        return m_popup_open;
    }

    /**
     * @brief Open the popup
     */
    void open_popup() {
        if (!m_popup_open && !m_items.empty()) {
            m_popup_open = true;
            m_hover_index = m_current_index;
            this->mark_dirty();
        }
    }

    /**
     * @brief Close the popup
     */
    void close_popup() {
        if (m_popup_open) {
            m_popup_open = false;
            m_hover_index = -1;
            this->mark_dirty();
        }
    }

    /**
     * @brief Toggle popup open/closed
     */
    void toggle_popup() {
        if (m_popup_open) {
            close_popup();
        } else {
            open_popup();
        }
    }

    // ===== Sizing =====

    /**
     * @brief Set preferred width in characters
     * @param chars Number of visible characters
     */
    void set_visible_chars(int chars) {
        if (m_visible_chars != chars) {
            m_visible_chars = std::max(1, chars);
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get visible character count
     */
    [[nodiscard]] int visible_chars() const noexcept {
        return m_visible_chars;
    }

    /**
     * @brief Set maximum visible items in popup
     * @param max_items Maximum items to show (0 = show all)
     */
    void set_max_visible_items(int max_items) {
        m_max_visible_items = std::max(0, max_items);
    }

    /**
     * @brief Get maximum visible items
     */
    [[nodiscard]] int max_visible_items() const noexcept {
        return m_max_visible_items;
    }

    /**
     * @brief Set custom combo box tiles (overrides theme)
     * @param tiles Combo tile configuration
     */
    void set_tiles(const combo_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's combo tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    // ===== Signals =====

    /// Emitted when selection index changes
    signal<int> current_index_changed;

    /// Emitted when selection text changes
    signal<const std::string&> current_text_changed;

protected:
    /**
     * @brief Handle keyboard and mouse events
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        // Handle mouse events
        if (auto* mouse = std::get_if<mouse_event>(&evt)) {
            if (mouse->act == mouse_event::action::press && phase == event_phase::target) {
                if (m_popup_open) {
                    // Check if click is on popup item
                    int clicked_item = get_item_at_position(
                        static_cast<int>(mouse->x.value),
                        static_cast<int>(mouse->y.value)
                    );
                    if (clicked_item >= 0) {
                        set_current_index(clicked_item);
                        close_popup();
                        return true;
                    }
                }
            } else if (mouse->act == mouse_event::action::move) {
                if (m_popup_open) {
                    // Update popup hover index
                    int hover_item = get_item_at_position(
                        static_cast<int>(mouse->x.value),
                        static_cast<int>(mouse->y.value)
                    );
                    if (hover_item != m_hover_index) {
                        m_hover_index = hover_item;
                        this->mark_dirty();
                    }
                }
                // Update button hover state (check if mouse is over the button)
                // Note: Since there's no explicit "leave" event, we rely on move events
                // that occur outside the button bounds to clear hover state
                bool is_over = is_point_in_button(
                    static_cast<int>(mouse->x.value),
                    static_cast<int>(mouse->y.value)
                );
                if (is_over != m_is_hovered) {
                    m_is_hovered = is_over;
                    this->mark_dirty();
                }
            }
        }

        // Handle keyboard events only in target phase when focused
        if (phase != event_phase::target || !this->has_focus()) {
            return base::handle_event(evt, phase);
        }

        if (auto* kbd = std::get_if<keyboard_event>(&evt)) {
            if (kbd->pressed) {
                return handle_key_press(*kbd);
            }
        }

        return base::handle_event(evt, phase);
    }

    /**
     * @brief Render the combo box using tile renderer
     */
    void do_render(render_context<Backend>& ctx) const override {
        auto* renderer = get_renderer();
        if (!renderer) {
            return;
        }

        // Get physical position and size from context
        const auto& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);
        const auto& size = ctx.available_size();
        const int w = size_utils::get_width(size);
        const int h = size_utils::get_height(size);

        // Get tiles
        const combo_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().combo;
        }

        if (!tiles) {
            return;
        }

        // Draw button background based on state
        const nine_slice* btn_slice = nullptr;
        if (m_popup_open && tiles->button_open.is_valid()) {
            btn_slice = &tiles->button_open;
        } else if (m_is_hovered && tiles->button_hover.is_valid()) {
            btn_slice = &tiles->button_hover;
        } else {
            btn_slice = &tiles->button;
        }

        if (btn_slice && btn_slice->is_valid()) {
            tile_renderer::tile_rect btn_rect{x, y, w, h};
            renderer->draw_nine_slice(*btn_slice, btn_rect);
        }

        // Get font
        const bitmap_font* font = nullptr;
        if (has_theme()) {
            font = &get_theme().font_normal;
        }

        if (font) {
            // Draw selected text
            int margin_h = btn_slice ? btn_slice->margin_h : 2;
            int margin_v = btn_slice ? btn_slice->margin_v : 2;
            int text_x = x + margin_h;
            int text_y = y + margin_v;

            std::string display_text = current_text();
            if (!display_text.empty()) {
                tile_renderer::tile_point text_pos{text_x, text_y};
                renderer->draw_bitmap_text(display_text, text_pos, *font);
            }
        }

        // Draw dropdown arrow
        if (tiles->arrow >= 0) {
            int arrow_size = font ? font->glyph_height : 16;
            int arrow_x = x + w - arrow_size - 2;
            int arrow_y = y + (h - arrow_size) / 2;
            tile_renderer::tile_point arrow_pos{arrow_x, arrow_y};
            renderer->draw_tile(tiles->arrow, arrow_pos);
        }

        // Draw popup if open
        if (m_popup_open) {
            draw_popup(ctx, x, y + h, w, tiles, font);
        }
    }

    /**
     * @brief Get content size based on visible chars
     */
    [[nodiscard]] logical_size get_content_size() const override {
        int char_width = 8;
        int char_height = 16;

        if (has_theme()) {
            const bitmap_font& font = get_theme().font_normal;
            char_width = font.glyph_width;
            char_height = font.glyph_height;
        }

        // Get margins from theme
        int margin_h = 2;
        int margin_v = 2;
        if (has_theme()) {
            const combo_tiles& tiles = get_theme().combo;
            if (tiles.button.is_valid()) {
                margin_h = tiles.button.margin_h;
                margin_v = tiles.button.margin_v;
            }
        }

        // Width = text area + arrow + margins
        int arrow_width = char_height;  // Arrow is square
        int text_width = m_visible_chars * char_width;

        return logical_size{
            logical_unit(text_width + arrow_width + 2 * margin_h + 4),
            logical_unit(char_height + 2 * margin_v)
        };
    }

private:
    std::vector<std::string> m_items;
    int m_current_index = -1;
    int m_hover_index = -1;
    bool m_popup_open = false;
    bool m_is_hovered = false;
    int m_visible_chars = 15;
    int m_max_visible_items = 8;
    combo_tiles m_custom_tiles{};
    bool m_use_custom_tiles = false;

    /**
     * @brief Handle key press
     */
    bool handle_key_press(const keyboard_event& evt) {
        if (m_popup_open) {
            // Popup navigation
            switch (evt.key) {
                case key_code::arrow_up:
                    if (m_hover_index > 0) {
                        m_hover_index--;
                        this->mark_dirty();
                    }
                    return true;

                case key_code::arrow_down:
                    if (m_hover_index < static_cast<int>(m_items.size()) - 1) {
                        m_hover_index++;
                        this->mark_dirty();
                    }
                    return true;

                case key_code::enter:
                case key_code::space:
                    if (m_hover_index >= 0) {
                        set_current_index(m_hover_index);
                    }
                    close_popup();
                    return true;

                case key_code::escape:
                    close_popup();
                    return true;

                case key_code::home:
                    m_hover_index = 0;
                    this->mark_dirty();
                    return true;

                case key_code::end:
                    m_hover_index = static_cast<int>(m_items.size()) - 1;
                    this->mark_dirty();
                    return true;

                default:
                    break;
            }
        } else {
            // Closed combo navigation
            switch (evt.key) {
                case key_code::arrow_up:
                    if (m_current_index > 0) {
                        set_current_index(m_current_index - 1);
                    }
                    return true;

                case key_code::arrow_down:
                    if (m_current_index < static_cast<int>(m_items.size()) - 1) {
                        set_current_index(m_current_index + 1);
                    }
                    return true;

                case key_code::space:
                case key_code::enter:
                    open_popup();
                    return true;

                default:
                    break;
            }
        }

        return false;
    }

    /**
     * @brief Draw the popup dropdown
     */
    void draw_popup(render_context<Backend>& ctx, int popup_x, int popup_y,
                    int popup_w, const combo_tiles* tiles, const bitmap_font* font) const {
        auto* renderer = get_renderer();
        if (!renderer || m_items.empty()) {
            return;
        }

        // Calculate popup height
        int item_height = font ? font->glyph_height : 16;
        int margin_v = tiles->dropdown.is_valid() ? tiles->dropdown.margin_v : 2;
        int visible_items = static_cast<int>(m_items.size());
        if (m_max_visible_items > 0 && visible_items > m_max_visible_items) {
            visible_items = m_max_visible_items;
        }
        int popup_h = visible_items * item_height + 2 * margin_v;

        // Draw popup background
        if (tiles->dropdown.is_valid()) {
            tile_renderer::tile_rect popup_rect{popup_x, popup_y, popup_w, popup_h};
            renderer->draw_nine_slice(tiles->dropdown, popup_rect);
        }

        // Draw items
        int margin_h = tiles->dropdown.is_valid() ? tiles->dropdown.margin_h : 2;
        int item_y = popup_y + margin_v;

        for (int i = 0; i < visible_items && i < static_cast<int>(m_items.size()); ++i) {
            // Draw item background if hovered or selected
            if (i == m_hover_index && tiles->item_hover.is_valid()) {
                tile_renderer::tile_rect item_rect{popup_x + margin_h, item_y,
                                              popup_w - 2 * margin_h, item_height};
                renderer->draw_nine_slice(tiles->item_hover, item_rect);
            } else if (i == m_current_index && tiles->item_selected.is_valid()) {
                tile_renderer::tile_rect item_rect{popup_x + margin_h, item_y,
                                              popup_w - 2 * margin_h, item_height};
                renderer->draw_nine_slice(tiles->item_selected, item_rect);
            }

            // Draw item text
            if (font) {
                tile_renderer::tile_point text_pos{popup_x + margin_h + 2, item_y};
                renderer->draw_bitmap_text(m_items[static_cast<std::size_t>(i)], text_pos, *font);
            }

            item_y += item_height;
        }
    }

    /**
     * @brief Check if point is within the button area (not the popup)
     */
    [[nodiscard]] bool is_point_in_button(int screen_x, int screen_y) const {
        auto abs_bounds = this->get_absolute_logical_bounds();
        int x = static_cast<int>(abs_bounds.x.value);
        int y = static_cast<int>(abs_bounds.y.value);
        int w = static_cast<int>(abs_bounds.width.value);
        int h = static_cast<int>(abs_bounds.height.value);

        return screen_x >= x && screen_x < x + w &&
               screen_y >= y && screen_y < y + h;
    }

    /**
     * @brief Get item index at screen position
     */
    [[nodiscard]] int get_item_at_position(int screen_x, int screen_y) const {
        if (!m_popup_open || m_items.empty()) {
            return -1;
        }

        // Get combo box absolute position
        auto abs_bounds = this->get_absolute_logical_bounds();
        int combo_x = static_cast<int>(abs_bounds.x.value);
        int combo_y = static_cast<int>(abs_bounds.y.value);
        int combo_w = static_cast<int>(abs_bounds.width.value);
        int combo_h = static_cast<int>(abs_bounds.height.value);

        // Popup is below the combo box
        int popup_x = combo_x;
        int popup_y = combo_y + combo_h;

        // Get font for item height
        int item_height = 16;
        int margin_v = 2;
        if (has_theme()) {
            item_height = get_theme().font_normal.glyph_height;
            if (get_theme().combo.dropdown.is_valid()) {
                margin_v = get_theme().combo.dropdown.margin_v;
            }
        }

        // Check if position is within popup
        int visible_items = static_cast<int>(m_items.size());
        if (m_max_visible_items > 0 && visible_items > m_max_visible_items) {
            visible_items = m_max_visible_items;
        }
        int popup_h = visible_items * item_height + 2 * margin_v;

        if (screen_x < popup_x || screen_x >= popup_x + combo_w ||
            screen_y < popup_y || screen_y >= popup_y + popup_h) {
            return -1;
        }

        // Calculate item index
        int relative_y = screen_y - popup_y - margin_v;
        if (relative_y < 0) {
            return -1;
        }

        int index = relative_y / item_height;
        if (index >= 0 && index < visible_items && index < static_cast<int>(m_items.size())) {
            return index;
        }

        return -1;
    }
};

} // namespace onyxui::tile
