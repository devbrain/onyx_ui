/**
 * @file line_edit.hh
 * @brief Single-line text input widget
 * @author Claude Code
 * @date 2025-11-18
 *
 * @details
 * A line_edit is a single-line text input widget for forms and dialogs.
 *
 * ## Key Features
 *
 * - Single-line text entry with horizontal scrolling
 * - Cursor navigation via semantic actions (arrows, Home/End, word jumps)
 * - Text selection via semantic actions (Shift+arrows, Ctrl+A)
 * - Insert vs overwrite modes (Insert key toggles)
 * - Clipboard support via semantic actions (Ctrl+C, Ctrl+V, Ctrl+X)
 * - Undo/redo via semantic actions
 * - Password mode (displays asterisks)
 * - Placeholder text (hint when empty)
 * - Read-only mode
 * - Input validation
 * - Cursor blinking (500ms interval)
 *
 * ## Behavior
 *
 * - **Enter key**: Emits `editing_finished` and `return_pressed` signals (submits form)
 * - **Escape key**: Emits `text_cancel` semantic action
 * - **Focus**: Automatically requests focus on click
 * - **Scrolling**: Content scrolls horizontally when cursor moves beyond visible area
 *
 * ## Difference from text_edit
 *
 * - line_edit: Single line, Enter submits (for forms/dialogs)
 * - text_edit: Multi-line, Enter inserts newline (for notes/comments)
 *
 * ## Example Usage
 *
 * @code
 * // Basic usage
 * auto name_edit = std::make_unique<line_edit<Backend>>("John Doe");
 * name_edit->set_placeholder("Enter your name");
 * name_edit->text_changed.connect([](const std::string& text) {
 *     std::cout << "Text: " << text << "\n";
 * });
 * name_edit->editing_finished.connect([]() {
 *     std::cout << "Submit!\n";
 * });
 *
 * // Password field
 * auto password_edit = std::make_unique<line_edit<Backend>>();
 * password_edit->set_password_mode(true);
 * password_edit->set_placeholder("Password");
 *
 * // Validation
 * auto age_edit = std::make_unique<line_edit<Backend>>();
 * age_edit->set_validator([](const std::string& text) {
 *     return std::all_of(text.begin(), text.end(), ::isdigit);
 * });
 * @endcode
 */

#pragma once

#include "onyxui/core/signal.hh"
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <stack>
#include <chrono>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/ui_constants.hh>

namespace onyxui {

    /**
     * @enum cursor_style
     * @brief Visual style of text cursor
     *
     * @details
     * Different cursor shapes for different edit modes:
     * - line: Thin vertical line (insert mode)
     * - block: Block covering character (overwrite mode)
     * - underline: Underline under character (alternate overwrite)
     */
    enum class cursor_style : std::uint8_t {
        line,       ///< Thin vertical line |  (insert mode)
        block,      ///< Block covering character █ (overwrite mode)
        underline   ///< Underline under character _ (alternate overwrite)
    };

    /**
     * @class line_edit
     * @brief Single-line text input widget
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Interactive text input widget with cursor, selection, and clipboard support.
     * All keyboard operations use semantic actions for customizability.
     *
     * ## States
     *
     * - Normal: Default appearance
     * - Focused: Has keyboard focus (cursor visible and blinking)
     * - Disabled: Not interactive
     *
     * ## Signals
     *
     * - text_changed: Emitted when text is modified
     * - editing_finished: Emitted on Enter key or focus lost (submit form)
     * - return_pressed: Emitted on Enter key specifically
     * - All base widget signals
     */
    template<UIBackend Backend>
    class line_edit : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using size_type = typename Backend::size_type;
        using point_type = typename Backend::point_type;
        using rect_type = typename Backend::rect_type;
        using color_type = typename Backend::color_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a line_edit with initial text
         * @param text Initial text content
         * @param parent Parent element (nullptr for none)
         */
        explicit line_edit(std::string text = "", ui_element<Backend>* parent = nullptr)
            : base(parent), m_text(std::move(text)) {
            this->set_focusable(true);  // line_edit is focusable
            m_cursor_pos = static_cast<int>(m_text.length());  // Cursor at end
        }

        /**
         * @brief Destructor
         */
        ~line_edit() override = default;

        // Rule of Five
        line_edit(const line_edit&) = delete;
        line_edit& operator=(const line_edit&) = delete;
        line_edit(line_edit&&) noexcept = default;
        line_edit& operator=(line_edit&&) noexcept = default;

        // =====================================================================
        // Text Management
        // =====================================================================

        /**
         * @brief Set the text content
         * @param text New text to display
         */
        void set_text(const std::string& text) {
            if (m_text != text) {
                m_text = text;
                m_cursor_pos = std::min(m_cursor_pos, static_cast<int>(m_text.length()));
                clear_selection();
                clear_undo_stack();
                this->invalidate_measure();
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
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Get the placeholder text
         */
        [[nodiscard]] const std::string& placeholder() const noexcept {
            return m_placeholder;
        }

        // =====================================================================
        // Display Modes
        // =====================================================================

        /**
         * @brief Set password mode (displays asterisks)
         * @param password True to enable password mode
         */
        void set_password_mode(bool password) {
            if (m_is_password != password) {
                m_is_password = password;
                this->invalidate_arrange();
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
            if (m_is_read_only != read_only) {
                m_is_read_only = read_only;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Check if widget is read-only
         */
        [[nodiscard]] bool is_read_only() const noexcept {
            return m_is_read_only;
        }

        /**
         * @brief Set overwrite mode (insert vs overwrite)
         * @param overwrite True for overwrite mode, false for insert mode
         */
        void set_overwrite_mode(bool overwrite) {
            m_overwrite_mode = overwrite;
            this->invalidate_arrange();  // Cursor shape changes
        }

        /**
         * @brief Check if overwrite mode is enabled
         */
        [[nodiscard]] bool is_overwrite_mode() const noexcept {
            return m_overwrite_mode;
        }

        /**
         * @brief Set whether to draw border around line_edit
         * @param has_border True to draw border, false for borderless
         */
        void set_has_border(bool has_border) {
            if (m_has_border != has_border) {
                m_has_border = has_border;
                this->invalidate_arrange();  // Border changes size
            }
        }

        /**
         * @brief Check if border is enabled
         */
        [[nodiscard]] bool has_border() const noexcept {
            return m_has_border;
        }

        // =====================================================================
        // Semantic Sizing (Backend-Agnostic)
        // =====================================================================

        /**
         * @brief Set preferred width based on number of visible characters
         * @param chars Number of characters to display horizontally
         *
         * @details
         * Sets the preferred width constraint based on the number of characters
         * that should be visible at once, measured in logical units. This is a
         * backend-agnostic way to size line_edit widgets that works correctly
         * across different backends.
         *
         * The widget will use this as a preferred size hint during layout, but may
         * be stretched or shrunk based on parent layout constraints.
         *
         * @note The actual physical width depends on the backend's font metrics.
         *
         * @example
         * @code
         * // Create a line_edit that shows ~30 characters at a time
         * auto username = std::make_unique<line_edit<Backend>>();
         * username->set_visible_chars(30);
         *
         * // Create a narrow line_edit for ZIP code (5 digits)
         * auto zipcode = std::make_unique<line_edit<Backend>>();
         * zipcode->set_visible_chars(10);
         * @endcode
         */
        void set_visible_chars(int chars) {
            if (chars < 1) chars = 1;

            size_constraint width_constraint;
            width_constraint.policy = size_policy::content;
            width_constraint.preferred_size = logical_unit(static_cast<double>(chars));
            this->set_width_constraint(width_constraint);
        }

        // =====================================================================
        // Cursor and Selection
        // =====================================================================

        /**
         * @brief Set cursor position
         * @param position New cursor position (clamped to valid range)
         */
        void set_cursor_position(int position) {
            m_cursor_pos = std::clamp(position, 0, static_cast<int>(m_text.length()));
            clear_selection();
            this->invalidate_arrange();
        }

        /**
         * @brief Get current cursor position
         */
        [[nodiscard]] int cursor_position() const noexcept {
            return m_cursor_pos;
        }

        /**
         * @brief Select all text
         */
        void select_all() {
            m_selection_start = 0;
            m_selection_end = static_cast<int>(m_text.length());
            m_cursor_pos = m_selection_end;
            this->invalidate_arrange();
        }

        /**
         * @brief Clear selection
         */
        void clear_selection() {
            m_selection_start = 0;
            m_selection_end = 0;
        }

        /**
         * @brief Check if there is a selection
         */
        [[nodiscard]] bool has_selection() const noexcept {
            return m_selection_start != m_selection_end;
        }

        /**
         * @brief Get selected text
         */
        [[nodiscard]] std::string selected_text() const {
            if (!has_selection()) {
                return "";
            }
            const int start = std::min(m_selection_start, m_selection_end);
            const int end = std::max(m_selection_start, m_selection_end);
            return m_text.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start));
        }

        // =====================================================================
        // Clipboard Operations
        // =====================================================================

        /**
         * @brief Copy selected text to clipboard
         */
        void copy() const {
            if (has_selection()) {
                // TODO: Implement clipboard integration
                // For now, store in static variable (platform-specific clipboard needed)
            }
        }

        /**
         * @brief Paste text from clipboard
         */
        void paste() {
            if (m_is_read_only) return;
            // TODO: Implement clipboard integration
            // For now, paste from static variable
        }

        /**
         * @brief Cut selected text to clipboard
         */
        void cut() {
            if (m_is_read_only) return;
            if (has_selection()) {
                copy();
                // Delete selection
                const int start = std::min(m_selection_start, m_selection_end);
                const int end = std::max(m_selection_start, m_selection_end);
                m_text.erase(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start));
                m_cursor_pos = start;
                clear_selection();
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        // =====================================================================
        // Undo/Redo
        // =====================================================================

        /**
         * @brief Undo last change
         */
        void undo() {
            if (m_is_read_only) return;
            if (!m_undo_stack.empty()) {
                m_redo_stack.push(m_text);
                m_text = m_undo_stack.top();
                m_undo_stack.pop();
                m_cursor_pos = std::min(m_cursor_pos, static_cast<int>(m_text.length()));
                clear_selection();
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        /**
         * @brief Redo last undo
         */
        void redo() {
            if (m_is_read_only) return;
            if (!m_redo_stack.empty()) {
                m_undo_stack.push(m_text);
                m_text = m_redo_stack.top();
                m_redo_stack.pop();
                m_cursor_pos = std::min(m_cursor_pos, static_cast<int>(m_text.length()));
                clear_selection();
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        // =====================================================================
        // Validation
        // =====================================================================

        /**
         * @brief Set input validator
         * @param validator Function that returns true if text is valid
         */
        void set_validator(std::function<bool(const std::string&)> validator) {
            m_validator = std::move(validator);
        }

        /**
         * @brief Check if current text is valid
         */
        [[nodiscard]] bool is_valid() const {
            if (!m_validator) return true;
            return m_validator(m_text);
        }

        // =====================================================================
        // Signals
        // =====================================================================

        signal<const std::string&> text_changed;    ///< Text modified
        signal<> editing_finished;                  ///< Enter or focus lost (submit)
        signal<> return_pressed;                    ///< Enter key pressed

        // =====================================================================
        // Event Handling
        // =====================================================================

        /**
         * @brief Handle events (keyboard input and mouse clicks)
         * @param event The event to handle
         * @param phase The event routing phase
         * @return true if event was handled
         */
        bool handle_event(const ui_event& event, event_phase phase) override {
            // Request focus on mouse press (capture phase)
            if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
                if (mouse_evt->act == mouse_event::action::press) {
                    if (phase == event_phase::capture) {
                        auto* input = ui_services<Backend>::input();
                        if (input && this->is_focusable()) {
                            input->set_focus(this);
                        }
                        return false;  // Continue to target phase
                    }
                }
            }

            // Handle keyboard events only in target phase
            if (phase != event_phase::target) {
                return base::handle_event(event, phase);
            }

            // Only process keyboard events when focused
            if (!this->has_focus()) {
                return base::handle_event(event, phase);
            }

            if (auto* key_evt = std::get_if<keyboard_event>(&event)) {
                return handle_key_event(*key_evt);
            }

            return base::handle_event(event, phase);
        }

        /**
         * @brief Handle semantic actions for text editing
         * @param action The semantic action to handle
         * @return true if action was handled
         *
         * @details
         * Handles text editing semantic actions from hotkey schemes:
         * - Cursor movement: left, right, home, end, word jumps
         * - Text selection: Shift+movement keys, Ctrl+A
         * - Text deletion: Delete, Backspace, word deletion
         * - Clipboard: Copy, Cut, Paste
         */
        bool handle_semantic_action(hotkey_action action) override {
            // Only handle actions when focused
            if (!this->has_focus()) {
                return base::handle_semantic_action(action);
            }

            switch (action) {
                // Cursor Movement (no selection)
                case hotkey_action::cursor_move_left:
                    handle_cursor_move_left(false);
                    return true;

                case hotkey_action::cursor_move_right:
                    handle_cursor_move_right(false);
                    return true;

                case hotkey_action::cursor_move_home:
                    handle_cursor_move_home(false);
                    return true;

                case hotkey_action::cursor_move_end:
                    handle_cursor_move_end(false);
                    return true;

                case hotkey_action::cursor_move_word_left:
                    handle_cursor_move_word_left(false);
                    return true;

                case hotkey_action::cursor_move_word_right:
                    handle_cursor_move_word_right(false);
                    return true;

                // Cursor Movement (with selection)
                case hotkey_action::cursor_select_left:
                    handle_cursor_move_left(true);
                    return true;

                case hotkey_action::cursor_select_right:
                    handle_cursor_move_right(true);
                    return true;

                case hotkey_action::cursor_select_home:
                    handle_cursor_move_home(true);
                    return true;

                case hotkey_action::cursor_select_end:
                    handle_cursor_move_end(true);
                    return true;

                case hotkey_action::cursor_select_word_left:
                    handle_cursor_move_word_left(true);
                    return true;

                case hotkey_action::cursor_select_word_right:
                    handle_cursor_move_word_right(true);
                    return true;

                case hotkey_action::cursor_select_all:
                    select_all();
                    return true;

                // Text Deletion
                case hotkey_action::text_delete_char:
                    handle_delete();
                    return true;

                case hotkey_action::text_backspace:
                    handle_backspace();
                    return true;

                case hotkey_action::text_delete_word:
                    handle_delete_word();
                    return true;

                case hotkey_action::text_backspace_word:
                    handle_backspace_word();
                    return true;

                // Clipboard (TODO: implement clipboard integration)
                case hotkey_action::text_copy:
                    // TODO: Copy selected text to clipboard
                    return true;

                case hotkey_action::text_cut:
                    // TODO: Cut selected text to clipboard
                    return true;

                case hotkey_action::text_paste:
                    // TODO: Paste text from clipboard
                    return true;

                // Mode Toggle
                case hotkey_action::text_toggle_overwrite:
                    set_overwrite_mode(!m_overwrite_mode);
                    return true;

                default:
                    return base::handle_semantic_action(action);
            }
        }

    protected:
        /**
         * @brief Render line_edit using render context
         *
         * @details
         * Uses visitor pattern via render_context. Handles both rendering
         * and measurement in one method.
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto* theme = ctx.theme();

            // Get padding and border from resolved style
            const int padding_horizontal = ctx.style().padding_horizontal.value
                .value_or(ui_constants::DEFAULT_BUTTON_PADDING_HORIZONTAL);
            const int padding_vertical = ctx.style().padding_vertical.value
                .value_or(ui_constants::DEFAULT_BUTTON_PADDING_VERTICAL);
            const int border = m_has_border ? renderer_type::get_border_thickness(ctx.style().box_style) : 0;

            // Measure text height (width will scroll)
            typename renderer_type::font const default_font{};
            const auto sample_text_size = renderer_type::measure_text("Ay", default_font);
            const int text_height = size_utils::get_height(sample_text_size);

            // Calculate natural size (minimum width for comfortable editing)
            const int min_width = 100;  // Reasonable minimum for text input
            const int natural_width = min_width + (padding_horizontal * 2) + (border * 2);
            const int natural_height = text_height + (padding_vertical * 2) + (border * 2);

            // Get final dimensions
            const auto [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);

            // Get position from context
            const auto& pos = ctx.position();
            const int x = point_utils::get_x(pos);
            const int y = point_utils::get_y(pos);

            // Create rectangle
            rect_type edit_rect;
            rect_utils::set_bounds(edit_rect, x, y, final_width, final_height);

            // Draw background and border
            if (m_has_border) {
                ctx.draw_rect(edit_rect, ctx.style().box_style);
            } else {
                ctx.draw_rect(edit_rect, typename renderer_type::box_style{});
            }

            if (!theme) return;

            // Calculate text area (inside padding and border)
            const int text_x = x + border + padding_horizontal;
            const int text_y = y + border + padding_vertical;

            // Determine what text to display and color
            std::string display_text;
            bool is_placeholder = false;
            if (m_text.empty() && !this->has_focus()) {
                // Show placeholder when empty and not focused
                display_text = m_placeholder;
                is_placeholder = true;
            } else if (m_is_password) {
                // Show asterisks in password mode
                display_text = std::string(m_text.length(), '*');
            } else {
                display_text = m_text;
            }

            // Use line_edit theme colors: placeholder_text for placeholder, text for normal text
            const auto text_color = is_placeholder ? theme->line_edit.placeholder_text : theme->line_edit.text;

            // Update cursor blink state based on theme interval
            update_cursor_blink(theme);

            // ===== Horizontal Scrolling Logic =====
            // Calculate available text width (content area minus padding and border)
            const int available_text_width = final_width - (border * 2) - (padding_horizontal * 2);

            // Adjust scroll offset to keep cursor visible
            if (!is_placeholder && !display_text.empty()) {
                // If cursor is before scroll offset, scroll left
                if (m_cursor_pos < m_scroll_offset) {
                    m_scroll_offset = m_cursor_pos;
                } else {
                    // Measure visible text up to cursor
                    const int chars_from_scroll_to_cursor = m_cursor_pos - m_scroll_offset;
                    if (chars_from_scroll_to_cursor > 0 && m_scroll_offset < static_cast<int>(display_text.length())) {
                        const int visible_end = std::min(m_scroll_offset + chars_from_scroll_to_cursor,
                                                        static_cast<int>(display_text.length()));
                        const std::string visible_to_cursor = display_text.substr(
                            static_cast<std::size_t>(m_scroll_offset),
                            static_cast<std::size_t>(visible_end - m_scroll_offset)
                        );
                        const auto cursor_offset_size = renderer_type::measure_text(visible_to_cursor, default_font);
                        const int cursor_pixel_offset = size_utils::get_width(cursor_offset_size);

                        // If cursor is beyond visible area, scroll right
                        if (cursor_pixel_offset >= available_text_width - 1) {  // -1 for cursor width
                            // Scroll right until cursor is visible
                            m_scroll_offset++;
                        }
                    }
                }

                // Clamp scroll offset to valid range
                m_scroll_offset = std::max(0, std::min(m_scroll_offset, static_cast<int>(display_text.length())));
            } else {
                m_scroll_offset = 0;  // Reset scroll for placeholder or empty text
            }

            // Extract visible portion of text
            std::string visible_text;
            if (!display_text.empty() && m_scroll_offset < static_cast<int>(display_text.length())) {
                // Start from scroll offset
                visible_text = display_text.substr(static_cast<std::size_t>(m_scroll_offset));

                // Trim to fit available width
                while (!visible_text.empty()) {
                    const auto text_size = renderer_type::measure_text(visible_text, default_font);
                    const int text_width = size_utils::get_width(text_size);
                    if (text_width <= available_text_width) {
                        break;  // Fits!
                    }
                    // Remove last character and try again
                    visible_text.pop_back();
                }
            }

            // Draw text with cursor emulation
            const bool show_cursor = this->has_focus() && m_cursor_visible;
            const int cursor_pos_in_text = m_cursor_pos - m_scroll_offset;  // Adjust for scroll offset

            if (!visible_text.empty() || show_cursor) {
                // Calculate cursor offset
                int cursor_x_offset = 0;
                if (show_cursor && cursor_pos_in_text > 0 && cursor_pos_in_text <= static_cast<int>(visible_text.length())) {
                    const std::string text_before_cursor = visible_text.substr(0, static_cast<std::size_t>(cursor_pos_in_text));
                    const auto cursor_offset_size = renderer_type::measure_text(text_before_cursor, default_font);
                    cursor_x_offset = size_utils::get_width(cursor_offset_size);
                }

                // Draw text before cursor
                if (cursor_pos_in_text > 0 && !visible_text.empty() && cursor_pos_in_text <= static_cast<int>(visible_text.length())) {
                    const std::string before_text = visible_text.substr(0, static_cast<std::size_t>(cursor_pos_in_text));
                    const point_type before_pos{text_x, text_y};
                    ctx.draw_text(before_text, before_pos, default_font, text_color);
                }

                // Draw cursor character (emulated)
                if (show_cursor) {
                    const int cursor_x = text_x + cursor_x_offset;
                    const point_type cursor_pos{cursor_x, text_y};

                    if (m_overwrite_mode && cursor_pos_in_text >= 0 && cursor_pos_in_text < static_cast<int>(visible_text.length())) {
                        // Block cursor: draw character at cursor position with cursor background
                        const std::string cursor_char = visible_text.substr(static_cast<std::size_t>(cursor_pos_in_text), 1);

                        // Create font with reverse attribute for cursor highlighting (if supported)
                        typename renderer_type::font cursor_font = default_font;
                        if constexpr (requires { cursor_font.reverse; }) {
                            cursor_font.reverse = true;
                        }

                        ctx.draw_text(cursor_char, cursor_pos, cursor_font, text_color);
                    } else {
                        // Insert mode or end of text: draw cursor icon from theme
                        const auto& cursor_icon = m_overwrite_mode ?
                            theme->line_edit.cursor_overwrite_icon :
                            theme->line_edit.cursor_insert_icon;
                        ctx.draw_icon(cursor_icon, cursor_pos);
                    }
                }

                // Draw text after cursor
                if (cursor_pos_in_text >= 0 && cursor_pos_in_text < static_cast<int>(visible_text.length())) {
                    const int after_start = show_cursor ? cursor_pos_in_text + 1 : cursor_pos_in_text;
                    if (after_start >= 0 && after_start < static_cast<int>(visible_text.length())) {
                        const std::string after_text = visible_text.substr(static_cast<std::size_t>(after_start));
                        const std::string up_to_cursor = visible_text.substr(0, static_cast<std::size_t>(after_start));
                        const auto offset_size = renderer_type::measure_text(up_to_cursor, default_font);
                        const int after_x = text_x + size_utils::get_width(offset_size);
                        const point_type after_pos{after_x, text_y};
                        ctx.draw_text(after_text, after_pos, default_font, text_color);
                    }
                }
            }
        }

        /**
         * @brief Get theme style for line_edit
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            return resolved_style<Backend>{
                .background_color = theme.line_edit.background,
                .foreground_color = theme.line_edit.border_color,  // Foreground is used for border color in draw_rect()
                .mnemonic_foreground = theme.line_edit.text,
                .border_color = theme.line_edit.border_color,
                .box_style = theme.line_edit.box_style,  // Use line_edit box style for border
                .font = theme.line_edit.font,
                .opacity = 1.0f,
                .icon_style = std::optional<typename renderer_type::icon_style>{},
                .padding_horizontal = std::make_optional(theme.line_edit.padding_horizontal),
                .padding_vertical = std::make_optional(theme.line_edit.padding_vertical),
                .mnemonic_font = std::optional<typename renderer_type::font>{},
                .submenu_icon = std::optional<typename renderer_type::icon_style>{}
            };
        }

        /**
         * @brief Prevent color inheritance from parent
         * @return false - line_edit should use its own theme colors
         *
         * @details
         * Similar to buttons, line_edit widgets should maintain consistent
         * border and background colors regardless of parent container colors.
         * This prevents CSS inheritance from overriding our border color.
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Use theme colors, not parent colors
        }

    private:
        // =====================================================================
        // Text Editing Helper Methods
        // =====================================================================

        /**
         * @brief Handle keyboard events (semantic actions)
         */
        bool handle_key_event(const keyboard_event& evt) {
            if (!evt.pressed) {
                return false;  // Only handle key press events
            }

            // Get hotkey_manager for semantic action translation
            auto* hotkeys = ui_services<Backend>::hotkeys();

            // Try semantic actions first (scheme-dependent)
            if (hotkeys) {
                // Cursor movement
                if (hotkeys->matches_action(evt, hotkey_action::cursor_move_left)) {
                    handle_cursor_move_left(false);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_move_right)) {
                    handle_cursor_move_right(false);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_move_word_left)) {
                    handle_cursor_move_word_left(false);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_move_word_right)) {
                    handle_cursor_move_word_right(false);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_move_home)) {
                    handle_cursor_move_home(false);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_move_end)) {
                    handle_cursor_move_end(false);
                    return true;
                }

                // Selection (cursor movement with Shift)
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_left)) {
                    handle_cursor_move_left(true);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_right)) {
                    handle_cursor_move_right(true);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_word_left)) {
                    handle_cursor_move_word_left(true);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_word_right)) {
                    handle_cursor_move_word_right(true);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_home)) {
                    handle_cursor_move_home(true);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_end)) {
                    handle_cursor_move_end(true);
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::cursor_select_all)) {
                    select_all();
                    return true;
                }

                // Deletion
                if (hotkeys->matches_action(evt, hotkey_action::text_delete_char)) {
                    handle_delete();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_backspace)) {
                    handle_backspace();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_delete_word)) {
                    handle_delete_word();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_backspace_word)) {
                    handle_backspace_word();
                    return true;
                }

                // Clipboard
                if (hotkeys->matches_action(evt, hotkey_action::text_copy)) {
                    copy();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_cut)) {
                    cut();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_paste)) {
                    paste();
                    return true;
                }

                // Undo/redo
                if (hotkeys->matches_action(evt, hotkey_action::text_undo)) {
                    undo();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_redo)) {
                    redo();
                    return true;
                }

                // Mode toggle
                if (hotkeys->matches_action(evt, hotkey_action::text_toggle_overwrite)) {
                    set_overwrite_mode(!m_overwrite_mode);
                    return true;
                }

                // Accept/cancel
                if (hotkeys->matches_action(evt, hotkey_action::text_accept)) {
                    return_pressed.emit();
                    editing_finished.emit();
                    return true;
                }
                if (hotkeys->matches_action(evt, hotkey_action::text_cancel)) {
                    // TODO: Restore original text on Escape?
                    return true;
                }
            }

            // Fallback: Handle basic keys even if hotkey scheme isn't set up
            // This ensures basic functionality works in all contexts
            switch (evt.key) {
                case key_code::backspace:
                    handle_backspace();
                    return true;
                case key_code::delete_key:
                    handle_delete();
                    return true;
                case key_code::home:
                    handle_cursor_move_home(false);
                    return true;
                case key_code::end:
                    handle_cursor_move_end(false);
                    return true;
                case key_code::arrow_left:
                    handle_cursor_move_left(false);
                    return true;
                case key_code::arrow_right:
                    handle_cursor_move_right(false);
                    return true;
                case key_code::enter:
                    return_pressed.emit();
                    editing_finished.emit();
                    return true;
                default:
                    break;
            }

            // Handle printable characters (insert at cursor)
            if (is_printable(evt.key)) {
                if (!m_is_read_only) {
                    char ch = static_cast<char>(static_cast<int>(evt.key));
                    insert_char_at_cursor(ch);
                    return true;
                }
            }

            return false;
        }


        /**
         * @brief Clear undo stack
         */
        void clear_undo_stack() {
            m_undo_stack = {};
            m_redo_stack = {};
        }

        /**
         * @brief Check if a key_code represents a printable character
         */
        [[nodiscard]] static bool is_printable(key_code key) noexcept {
            int code = static_cast<int>(key);
            // Printable ASCII range: 32 (space) to 126 (~)
            return code >= 32 && code <= 126;
        }

        /**
         * @brief Insert character at cursor position
         */
        void insert_char_at_cursor(char ch) {
            if (m_is_read_only) return;

            // Delete selection if exists
            if (has_selection()) {
                delete_selection();
            }

            // Add to undo stack before modification
            m_undo_stack.push(m_text);
            m_redo_stack = {};  // Clear redo stack on new edit

            // Insert or overwrite
            if (m_overwrite_mode && m_cursor_pos < static_cast<int>(m_text.length())) {
                m_text[static_cast<std::size_t>(m_cursor_pos)] = ch;
            } else {
                m_text.insert(static_cast<std::size_t>(m_cursor_pos), 1, ch);
            }

            m_cursor_pos++;
            reset_cursor_blink();
            this->invalidate_measure();
            text_changed.emit(m_text);
        }

        /**
         * @brief Delete selection and return true if selection existed
         */
        bool delete_selection() {
            if (!has_selection()) return false;

            const int start = std::min(m_selection_start, m_selection_end);
            const int end = std::max(m_selection_start, m_selection_end);

            m_text.erase(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start));
            m_cursor_pos = start;
            clear_selection();
            this->invalidate_measure();
            text_changed.emit(m_text);
            return true;
        }

        /**
         * @brief Handle cursor movement to the left
         * @param extend_selection True to extend selection, false to move cursor
         */
        void handle_cursor_move_left(bool extend_selection) {
            if (!extend_selection && has_selection()) {
                // Move cursor to start of selection and clear
                m_cursor_pos = std::min(m_selection_start, m_selection_end);
                clear_selection();
            } else {
                if (m_cursor_pos > 0) {
                    if (extend_selection) {
                        if (!has_selection()) {
                            m_selection_start = m_cursor_pos;
                        }
                        m_cursor_pos--;
                        m_selection_end = m_cursor_pos;
                    } else {
                        m_cursor_pos--;
                    }
                }
            }
            reset_cursor_blink();
            this->mark_dirty();
        }

        /**
         * @brief Handle cursor movement to the right
         */
        void handle_cursor_move_right(bool extend_selection) {
            if (!extend_selection && has_selection()) {
                // Move cursor to end of selection and clear
                m_cursor_pos = std::max(m_selection_start, m_selection_end);
                clear_selection();
            } else {
                if (m_cursor_pos < static_cast<int>(m_text.length())) {
                    if (extend_selection) {
                        if (!has_selection()) {
                            m_selection_start = m_cursor_pos;
                        }
                        m_cursor_pos++;
                        m_selection_end = m_cursor_pos;
                    } else {
                        m_cursor_pos++;
                    }
                }
            }
            reset_cursor_blink();
            this->mark_dirty();
        }

        /**
         * @brief Find start of word to the left of cursor
         */
        [[nodiscard]] int find_word_start_left() const noexcept {
            int pos = m_cursor_pos;
            if (pos == 0) return 0;

            // Skip whitespace
            while (pos > 0 && std::isspace(static_cast<unsigned char>(m_text[static_cast<std::size_t>(pos - 1)]))) {
                pos--;
            }

            // Skip word characters
            while (pos > 0 && !std::isspace(static_cast<unsigned char>(m_text[static_cast<std::size_t>(pos - 1)]))) {
                pos--;
            }

            return pos;
        }

        /**
         * @brief Find end of word to the right of cursor
         */
        [[nodiscard]] int find_word_end_right() const noexcept {
            int pos = m_cursor_pos;
            const int length = static_cast<int>(m_text.length());
            if (pos >= length) return length;

            // Skip whitespace
            while (pos < length && std::isspace(static_cast<unsigned char>(m_text[static_cast<std::size_t>(pos)]))) {
                pos++;
            }

            // Skip word characters
            while (pos < length && !std::isspace(static_cast<unsigned char>(m_text[static_cast<std::size_t>(pos)]))) {
                pos++;
            }

            return pos;
        }

        /**
         * @brief Handle word-wise cursor movement to the left
         */
        void handle_cursor_move_word_left(bool extend_selection) {
            const int new_pos = find_word_start_left();

            if (extend_selection) {
                if (!has_selection()) {
                    m_selection_start = m_cursor_pos;
                }
                m_cursor_pos = new_pos;
                m_selection_end = m_cursor_pos;
            } else {
                m_cursor_pos = new_pos;
                clear_selection();
            }
            reset_cursor_blink();
            this->mark_dirty();
        }

        /**
         * @brief Handle word-wise cursor movement to the right
         */
        void handle_cursor_move_word_right(bool extend_selection) {
            const int new_pos = find_word_end_right();

            if (extend_selection) {
                if (!has_selection()) {
                    m_selection_start = m_cursor_pos;
                }
                m_cursor_pos = new_pos;
                m_selection_end = m_cursor_pos;
            } else {
                m_cursor_pos = new_pos;
                clear_selection();
            }
            reset_cursor_blink();
            this->mark_dirty();
        }

        /**
         * @brief Handle cursor movement to home (start of line)
         */
        void handle_cursor_move_home(bool extend_selection) {
            if (extend_selection) {
                if (!has_selection()) {
                    m_selection_start = m_cursor_pos;
                }
                m_cursor_pos = 0;
                m_selection_end = m_cursor_pos;
            } else {
                m_cursor_pos = 0;
                clear_selection();
            }
            reset_cursor_blink();
            this->mark_dirty();
        }

        /**
         * @brief Handle cursor movement to end (end of line)
         */
        void handle_cursor_move_end(bool extend_selection) {
            if (extend_selection) {
                if (!has_selection()) {
                    m_selection_start = m_cursor_pos;
                }
                m_cursor_pos = static_cast<int>(m_text.length());
                m_selection_end = m_cursor_pos;
            } else {
                m_cursor_pos = static_cast<int>(m_text.length());
                clear_selection();
            }
            reset_cursor_blink();
            this->mark_dirty();
        }

        /**
         * @brief Handle Delete key (delete character after cursor)
         */
        void handle_delete() {
            if (m_is_read_only) return;

            // If selection exists, delete it
            if (has_selection()) {
                m_undo_stack.push(m_text);
                m_redo_stack = {};
                delete_selection();
                return;
            }

            // Delete character at cursor
            if (m_cursor_pos < static_cast<int>(m_text.length())) {
                m_undo_stack.push(m_text);
                m_redo_stack = {};
                m_text.erase(static_cast<std::size_t>(m_cursor_pos), 1);
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        /**
         * @brief Handle Backspace key (delete character before cursor)
         */
        void handle_backspace() {
            if (m_is_read_only) return;

            // If selection exists, delete it
            if (has_selection()) {
                m_undo_stack.push(m_text);
                m_redo_stack = {};
                delete_selection();
                return;
            }

            // Delete character before cursor
            if (m_cursor_pos > 0) {
                m_undo_stack.push(m_text);
                m_redo_stack = {};
                m_text.erase(static_cast<std::size_t>(m_cursor_pos - 1), 1);
                m_cursor_pos--;
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        /**
         * @brief Handle Ctrl+Delete (delete word after cursor)
         */
        void handle_delete_word() {
            if (m_is_read_only) return;

            const int word_end = find_word_end_right();
            if (word_end > m_cursor_pos) {
                m_undo_stack.push(m_text);
                m_redo_stack = {};
                m_text.erase(static_cast<std::size_t>(m_cursor_pos),
                            static_cast<std::size_t>(word_end - m_cursor_pos));
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        /**
         * @brief Handle Ctrl+Backspace (delete word before cursor)
         */
        void handle_backspace_word() {
            if (m_is_read_only) return;

            const int word_start = find_word_start_left();
            if (word_start < m_cursor_pos) {
                m_undo_stack.push(m_text);
                m_redo_stack = {};
                m_text.erase(static_cast<std::size_t>(word_start),
                            static_cast<std::size_t>(m_cursor_pos - word_start));
                m_cursor_pos = word_start;
                this->invalidate_measure();
                text_changed.emit(m_text);
            }
        }

        // =====================================================================
        // Member Variables
        // =====================================================================

        std::string m_text;                          ///< Current text content
        std::string m_placeholder;                   ///< Placeholder text (hint)
        bool m_is_password = false;                  ///< Password mode (show asterisks)
        bool m_is_read_only = false;                 ///< Read-only mode
        bool m_overwrite_mode = false;               ///< Insert (false) vs Overwrite (true)
        bool m_has_border = false;                   ///< Whether to draw border

        // Cursor and selection
        int m_cursor_pos = 0;                        ///< Cursor position (index)
        int m_selection_start = 0;                   ///< Selection start index
        int m_selection_end = 0;                     ///< Selection end index
        mutable bool m_cursor_visible = true;        ///< Cursor blink state
        mutable std::chrono::steady_clock::time_point m_last_blink_time = std::chrono::steady_clock::now();  ///< Last cursor blink toggle time

        // Undo/redo
        std::stack<std::string> m_undo_stack;        ///< Undo history
        std::stack<std::string> m_redo_stack;        ///< Redo history

        // Horizontal scrolling
        mutable int m_scroll_offset = 0;             ///< Horizontal scroll position (character index of leftmost visible character)

        // Validation
        std::function<bool(const std::string&)> m_validator;  ///< Input validator

        /**
         * @brief Reset cursor blink state (make cursor visible)
         *
         * @details
         * Called whenever user types or moves cursor to reset the blink animation.
         * Resets the blink timer so cursor remains visible for full interval.
         */
        void reset_cursor_blink() {
            m_cursor_visible = true;
            m_last_blink_time = std::chrono::steady_clock::now();
            this->mark_dirty();
        }

        /**
         * @brief Update cursor blink state based on elapsed time
         *
         * @details
         * Checks if enough time has elapsed since last blink toggle.
         * If blink interval has passed, toggles cursor visibility.
         * This method is called from do_render() on each frame.
         * @param theme The theme to use for blink interval
         */
        void update_cursor_blink(const ui_theme<Backend>* theme) const {
            if (!this->has_focus()) {
                return;  // Don't blink if not focused
            }

            if (!theme) {
                return;
            }

            const int blink_interval_ms = theme->line_edit.cursor_blink_interval_ms;
            if (blink_interval_ms <= 0) {
                m_cursor_visible = true;  // No blinking if interval is 0 or negative
                return;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_blink_time);

            if (elapsed.count() >= blink_interval_ms) {
                m_cursor_visible = !m_cursor_visible;
                m_last_blink_time = now;
            }
        }
    };

} // namespace onyxui
