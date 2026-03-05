/**
 * @file tile_text_input.hh
 * @brief Single-line text input widget with tile-based rendering
 */

#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/events/ui_event.hh>

namespace onyxui::tile {

/**
 * @class tile_text_input
 * @brief Single-line text input widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_text_input displays a text input field using nine-slice background
 * and bitmap font for text rendering.
 *
 * ## Features
 * - Single-line text entry
 * - Cursor navigation (arrows, Home/End)
 * - Text editing (insert, delete, backspace)
 * - Password mode (displays asterisks)
 * - Placeholder text
 * - Cursor blinking
 *
 * @example
 * @code
 * auto input = std::make_unique<tile_text_input<sdlpp_tile_backend>>();
 * input->set_placeholder("Enter name...");
 * input->text_changed.connect([](const std::string& text) {
 *     std::cout << "Text: " << text << "\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_text_input : public widget<Backend> {
public:
    using base = widget<Backend>;

    /**
     * @brief Construct a text input with initial text
     * @param text Initial text content
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_text_input(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent), m_text(std::move(text))
    {
        this->set_focusable(true);
        m_cursor_pos = static_cast<int>(m_text.length());
    }

    /**
     * @brief Destructor
     */
    ~tile_text_input() override = default;

    // Rule of Five
    tile_text_input(const tile_text_input&) = delete;
    tile_text_input& operator=(const tile_text_input&) = delete;
    tile_text_input(tile_text_input&&) noexcept = default;
    tile_text_input& operator=(tile_text_input&&) noexcept = default;

    // ===== Text Management =====

    /**
     * @brief Set the text content
     * @param text New text to display
     */
    void set_text(const std::string& text) {
        if (m_text != text) {
            m_text = text;
            m_cursor_pos = std::min(m_cursor_pos, static_cast<int>(m_text.length()));
            this->mark_dirty();
            text_changed.emit(m_text);
        }
    }

    /**
     * @brief Get the current text
     */
    [[nodiscard]] const std::string& text() const noexcept {
        return m_text;
    }

    /**
     * @brief Set placeholder text (hint when empty)
     * @param placeholder Placeholder text
     */
    void set_placeholder(const std::string& placeholder) {
        if (m_placeholder != placeholder) {
            m_placeholder = placeholder;
            this->mark_dirty();
        }
    }

    /**
     * @brief Get the placeholder text
     */
    [[nodiscard]] const std::string& placeholder() const noexcept {
        return m_placeholder;
    }

    // ===== Display Modes =====

    /**
     * @brief Set password mode (displays asterisks)
     * @param password True to enable password mode
     */
    void set_password_mode(bool password) {
        if (m_is_password != password) {
            m_is_password = password;
            this->mark_dirty();
        }
    }

    /**
     * @brief Check if password mode is enabled
     */
    [[nodiscard]] bool is_password_mode() const noexcept {
        return m_is_password;
    }

    /**
     * @brief Set read-only mode
     * @param read_only True to make widget read-only
     */
    void set_read_only(bool read_only) {
        m_is_read_only = read_only;
    }

    /**
     * @brief Check if widget is read-only
     */
    [[nodiscard]] bool is_read_only() const noexcept {
        return m_is_read_only;
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

    // ===== Cursor =====

    /**
     * @brief Set cursor position
     * @param position New cursor position (clamped to valid range)
     */
    void set_cursor_position(int position) {
        m_cursor_pos = std::clamp(position, 0, static_cast<int>(m_text.length()));
        reset_cursor_blink();
        this->mark_dirty();
    }

    /**
     * @brief Get current cursor position
     */
    [[nodiscard]] int cursor_position() const noexcept {
        return m_cursor_pos;
    }

    /**
     * @brief Set custom text input tiles (overrides theme)
     * @param tiles Text input tile configuration
     */
    void set_tiles(const text_input_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's text input tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    // ===== Signals =====

    /// Emitted when text is modified
    signal<const std::string&> text_changed;

    /// Emitted on Enter key or focus lost
    signal<> editing_finished;

    /// Emitted on Enter key
    signal<> return_pressed;

protected:
    /**
     * @brief Handle keyboard and mouse events
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        // Request focus on mouse press (capture phase)
        if (auto* mouse = std::get_if<mouse_event>(&evt)) {
            if (mouse->act == mouse_event::action::press && phase == event_phase::capture) {
                auto* input = ui_services<Backend>::input();
                if (input && this->is_focusable()) {
                    input->set_focus(this);
                }
                return false;  // Continue to target phase
            }
        }

        // Handle cursor positioning on click
        if (auto* mouse = std::get_if<mouse_event>(&evt)) {
            if (mouse->act == mouse_event::action::press && phase == event_phase::target) {
                // Calculate cursor position from click
                handle_mouse_click(*mouse);
                return true;
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
     * @brief Render the text input using tile renderer
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
        const text_input_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().text_input;
        }

        if (!tiles) {
            return;
        }

        // Draw background based on state
        const nine_slice* bg_slice = nullptr;
        if (!this->is_enabled()) {
            bg_slice = tiles->disabled.is_valid() ? &tiles->disabled : &tiles->normal;
        } else if (this->has_focus()) {
            bg_slice = tiles->focused.is_valid() ? &tiles->focused : &tiles->normal;
        } else {
            bg_slice = &tiles->normal;
        }

        if (bg_slice && bg_slice->is_valid()) {
            tile_renderer::tile_rect bg_rect{x, y, w, h};
            renderer->draw_nine_slice(*bg_slice, bg_rect);
        }

        // Get font
        const bitmap_font* font = nullptr;
        if (has_theme()) {
            font = &get_theme().font_normal;
        }

        if (!font) {
            return;
        }

        // Calculate text area (inside margins)
        int margin_h = bg_slice ? bg_slice->margin_h : 2;
        int margin_v = bg_slice ? bg_slice->margin_v : 2;
        const int text_x = x + margin_h;
        const int text_y = y + margin_v;
        const int text_area_w = std::max(0, w - 2 * margin_h);

        // Determine display text
        std::string display_text;
        bool is_placeholder = false;

        if (m_text.empty() && !this->has_focus()) {
            display_text = m_placeholder;
            is_placeholder = true;
        } else if (m_is_password) {
            display_text = std::string(m_text.length(), '*');
        } else {
            display_text = m_text;
        }

        // Update scroll offset to keep cursor visible
        update_scroll_offset(font, text_area_w);

        // Calculate visible text
        std::string visible_text;
        if (!display_text.empty() && m_scroll_offset < static_cast<int>(display_text.length())) {
            visible_text = display_text.substr(static_cast<std::size_t>(m_scroll_offset));

            // Trim to fit
            while (!visible_text.empty() && font->text_width(visible_text) > text_area_w) {
                visible_text.pop_back();
            }
        }

        // Draw text
        if (!visible_text.empty()) {
            const bitmap_font* text_font = font;
            if (is_placeholder && has_theme()) {
                // Use disabled font for placeholder if available
                text_font = &get_theme().font_disabled;
            }
            tile_renderer::tile_point text_pos{text_x, text_y};
            renderer->draw_bitmap_text(visible_text, text_pos, *text_font);
        }

        // Draw cursor
        if (this->has_focus() && !is_placeholder) {
            update_cursor_blink();

            if (m_cursor_visible) {
                int cursor_x_offset = 0;
                int cursor_pos_in_visible = m_cursor_pos - m_scroll_offset;

                if (cursor_pos_in_visible > 0 && cursor_pos_in_visible <= static_cast<int>(visible_text.length())) {
                    std::string text_before = visible_text.substr(0, static_cast<std::size_t>(cursor_pos_in_visible));
                    cursor_x_offset = font->text_width(text_before);
                }

                // Draw cursor tile or fallback to simple line
                if (tiles->cursor >= 0) {
                    tile_renderer::tile_point cursor_pos{text_x + cursor_x_offset, text_y};
                    renderer->draw_tile(tiles->cursor, cursor_pos);
                }
            }
        }
    }

    /**
     * @brief Get content size based on visible chars
     */
    [[nodiscard]] logical_size get_content_size() const override {
        int char_width = 8;  // Default
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
            const text_input_tiles& tiles = get_theme().text_input;
            if (tiles.normal.is_valid()) {
                margin_h = tiles.normal.margin_h;
                margin_v = tiles.normal.margin_v;
            }
        }

        return logical_size{
            logical_unit(m_visible_chars * char_width + 2 * margin_h),
            logical_unit(char_height + 2 * margin_v)
        };
    }

private:
    std::string m_text;
    std::string m_placeholder;
    bool m_is_password = false;
    bool m_is_read_only = false;
    int m_visible_chars = 20;
    int m_cursor_pos = 0;
    mutable int m_scroll_offset = 0;
    mutable bool m_cursor_visible = true;
    mutable std::chrono::steady_clock::time_point m_last_blink{};
    text_input_tiles m_custom_tiles{};
    bool m_use_custom_tiles = false;

    static constexpr int CURSOR_BLINK_MS = 500;

    /**
     * @brief Handle mouse click to position cursor
     */
    void handle_mouse_click(const mouse_event& mouse) {
        auto const abs_bounds = this->get_absolute_logical_bounds();
        int const widget_x = static_cast<int>(abs_bounds.x.value);
        int const click_x = static_cast<int>(mouse.x.value) - widget_x;

        // Get margins
        int margin_h = 2;
        if (has_theme() && get_theme().text_input.normal.is_valid()) {
            margin_h = get_theme().text_input.normal.margin_h;
        }

        // Get font
        const bitmap_font* font = nullptr;
        if (has_theme()) {
            font = &get_theme().font_normal;
        }

        if (!font) {
            return;
        }

        // Calculate cursor position from click
        int const relative_x = click_x - margin_h;
        std::string display_text = m_is_password
            ? std::string(m_text.length(), '*')
            : m_text;

        if (relative_x <= 0) {
            m_cursor_pos = m_scroll_offset;
        } else {
            // Find character at click position
            int accumulated_width = 0;
            int pos = m_scroll_offset;

            while (pos < static_cast<int>(display_text.length())) {
                accumulated_width += font->glyph_width;
                if (accumulated_width > relative_x) {
                    break;
                }
                pos++;
            }

            m_cursor_pos = pos;
        }

        reset_cursor_blink();
        this->mark_dirty();
    }

    /**
     * @brief Handle key press
     */
    bool handle_key_press(const keyboard_event& evt) {
        switch (evt.key) {
            case key_code::arrow_left:
                if (m_cursor_pos > 0) {
                    m_cursor_pos--;
                    reset_cursor_blink();
                    this->mark_dirty();
                }
                return true;

            case key_code::arrow_right:
                if (m_cursor_pos < static_cast<int>(m_text.length())) {
                    m_cursor_pos++;
                    reset_cursor_blink();
                    this->mark_dirty();
                }
                return true;

            case key_code::home:
                m_cursor_pos = 0;
                reset_cursor_blink();
                this->mark_dirty();
                return true;

            case key_code::end:
                m_cursor_pos = static_cast<int>(m_text.length());
                reset_cursor_blink();
                this->mark_dirty();
                return true;

            case key_code::backspace:
                if (!m_is_read_only && m_cursor_pos > 0) {
                    m_text.erase(static_cast<std::size_t>(m_cursor_pos - 1), 1);
                    m_cursor_pos--;
                    reset_cursor_blink();
                    this->mark_dirty();
                    text_changed.emit(m_text);
                }
                return true;

            case key_code::delete_key:
                if (!m_is_read_only && m_cursor_pos < static_cast<int>(m_text.length())) {
                    m_text.erase(static_cast<std::size_t>(m_cursor_pos), 1);
                    reset_cursor_blink();
                    this->mark_dirty();
                    text_changed.emit(m_text);
                }
                return true;

            case key_code::enter:
                return_pressed.emit();
                editing_finished.emit();
                return true;

            default:
                // Handle printable characters
                if (is_printable(evt.key) && !m_is_read_only) {
                    char ch = static_cast<char>(static_cast<int>(evt.key));
                    m_text.insert(static_cast<std::size_t>(m_cursor_pos), 1, ch);
                    m_cursor_pos++;
                    reset_cursor_blink();
                    this->mark_dirty();
                    text_changed.emit(m_text);
                    return true;
                }
                break;
        }

        return false;
    }

    /**
     * @brief Check if key is printable
     */
    [[nodiscard]] static bool is_printable(key_code key) noexcept {
        int code = static_cast<int>(key);
        return code >= 32 && code <= 126;
    }

    /**
     * @brief Update scroll offset to keep cursor visible
     */
    void update_scroll_offset(const bitmap_font* font, int text_area_w) const {
        if (!font || text_area_w <= 0) {
            m_scroll_offset = 0;
            return;
        }

        // If cursor is before scroll offset, scroll left
        if (m_cursor_pos < m_scroll_offset) {
            m_scroll_offset = m_cursor_pos;
            return;
        }

        // Calculate visible chars
        int visible_chars = text_area_w / font->glyph_width;
        if (visible_chars < 1) visible_chars = 1;

        // If cursor is beyond visible area, scroll right
        if (m_cursor_pos >= m_scroll_offset + visible_chars) {
            m_scroll_offset = m_cursor_pos - visible_chars + 1;
        }

        // Clamp scroll offset
        m_scroll_offset = std::max(0, m_scroll_offset);
    }

    /**
     * @brief Reset cursor blink state
     */
    void reset_cursor_blink() {
        m_cursor_visible = true;
        m_last_blink = std::chrono::steady_clock::now();
    }

    /**
     * @brief Update cursor blink state
     */
    void update_cursor_blink() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_blink);

        if (elapsed.count() >= CURSOR_BLINK_MS) {
            m_cursor_visible = !m_cursor_visible;
            m_last_blink = now;
        }
    }
};

} // namespace onyxui::tile
