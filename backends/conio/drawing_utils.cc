//
// Created by igor on 14/10/2025.
//
#include "dos_chars.h"
#include "drawing_utils.hh"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace onyxui::conio {
    // ========================================================================
    // Text Drawing
    // ========================================================================

    void drawing_context::draw_text(int x, int y, const std::string& text,
                                    color fg, color bg) {
        for (size_t i = 0; i < text.length(); ++i) {
            m_vram->put(x + i, y, static_cast <unsigned char>(text[i]), fg, bg);
        }
    }

    void drawing_context::draw_text_centered(const rect& bounds, const std::string& text,
                                             color fg, color bg) {
        int text_len = static_cast <int>(text.length());
        if (text_len > bounds.w) {
            // Text too long, draw what fits
            draw_text(bounds.x, bounds.y + bounds.h / 2,
                      text.substr(0, bounds.w), fg, bg);
        } else {
            int x = bounds.x + (bounds.w - text_len) / 2;
            int y = bounds.y + bounds.h / 2;
            draw_text(x, y, text, fg, bg);
        }
    }

    void drawing_context::draw_text_right(int x, int y, int width,
                                          const std::string& text,
                                          color fg, color bg) {
        int text_len = static_cast <int>(text.length());
        if (text_len > width) {
            draw_text(x, y, text.substr(text_len - width), fg, bg);
        } else {
            draw_text(x + width - text_len, y, text, fg, bg);
        }
    }

    // ========================================================================
    // Box Drawing
    // ========================================================================

    drawing_context::box_chars drawing_context::get_box_chars(box_style style) const {
        box_chars chars;

        switch (style) {
            case box_style::single:
            default:
                chars.tl = DOS_TL;
                chars.tr = DOS_TR;
                chars.bl = DOS_BL;
                chars.br = DOS_BR;
                chars.h = DOS_H;
                chars.v = DOS_V;
                chars.t = DOS_T;
                chars.b = DOS_B;
                chars.l = DOS_L;
                chars.r = DOS_R;
                chars.x = DOS_X;
                break;

            case box_style::double_line:
                chars.tl = DOS_TL2;
                chars.tr = DOS_TR2;
                chars.bl = DOS_BL2;
                chars.br = DOS_BR2;
                chars.h = DOS_H2;
                chars.v = DOS_V2;
                chars.t = DOS_T2;
                chars.b = DOS_B2;
                chars.l = DOS_L2;
                chars.r = DOS_R2;
                chars.x = DOS_X2;
                break;

            case box_style::rounded:
                chars.tl = BOX_ROUND_TL;
                chars.tr = BOX_ROUND_TR;
                chars.bl = BOX_ROUND_BL;
                chars.br = BOX_ROUND_BR;
                chars.h = BOX_ROUND_H;
                chars.v = BOX_ROUND_V;
                chars.t = DOS_T;
                chars.b = DOS_B;
                chars.l = DOS_L;
                chars.r = DOS_R;
                chars.x = DOS_X;
                break;

            case box_style::heavy:
                chars.tl = BOX_HEAVY_ROUND_TL;
                chars.tr = BOX_HEAVY_ROUND_TR;
                chars.bl = BOX_HEAVY_ROUND_BL;
                chars.br = BOX_HEAVY_ROUND_BR;
                chars.h = BOX_HEAVY_H;
                chars.v = BOX_HEAVY_V;
                chars.t = DOS_T;
                chars.b = DOS_B;
                chars.l = DOS_L;
                chars.r = DOS_R;
                chars.x = DOS_X;
                break;
        }

        return chars;
    }

    void drawing_context::draw_box(const rect& bounds, color fg, color bg, box_style style) {
        if (bounds.w < 2 || bounds.h < 2) return;

        auto chars = get_box_chars(style);

        // Draw corners
        m_vram->put(bounds.x, bounds.y, chars.tl, fg, bg);
        m_vram->put(bounds.x + bounds.w - 1, bounds.y, chars.tr, fg, bg);
        m_vram->put(bounds.x, bounds.y + bounds.h - 1, chars.bl, fg, bg);
        m_vram->put(bounds.x + bounds.w - 1, bounds.y + bounds.h - 1, chars.br, fg, bg);

        // Draw horizontal lines
        for (int x = bounds.x + 1; x < bounds.x + bounds.w - 1; ++x) {
            m_vram->put(x, bounds.y, chars.h, fg, bg);
            m_vram->put(x, bounds.y + bounds.h - 1, chars.h, fg, bg);
        }

        // Draw vertical lines
        for (int y = bounds.y + 1; y < bounds.y + bounds.h - 1; ++y) {
            m_vram->put(bounds.x, y, chars.v, fg, bg);
            m_vram->put(bounds.x + bounds.w - 1, y, chars.v, fg, bg);
        }
    }

    void drawing_context::draw_filled_box(const rect& bounds, color fg, color bg, box_style style) {
        // Draw the frame
        draw_box(bounds, fg, bg, style);

        // Fill the interior
        for (int y = bounds.y + 1; y < bounds.y + bounds.h - 1; ++y) {
            for (int x = bounds.x + 1; x < bounds.x + bounds.w - 1; ++x) {
                m_vram->put(x, y, ' ', fg, bg);
            }
        }
    }

    void drawing_context::draw_shadow(const rect& bounds) {
        // Draw shadow to the right
        for (int y = bounds.y + 1; y < bounds.y + bounds.h + 1; ++y) {
            if (y < m_vram->get_height()) {
                m_vram->put(bounds.x + bounds.w, y, DOS_BLOCK_FULL,
                            color(0, 0, 0), color(0, 0, 0));
            }
        }

        // Draw shadow at the bottom
        for (int x = bounds.x + 1; x < bounds.x + bounds.w + 1; ++x) {
            if (x < m_vram->get_width()) {
                m_vram->put(x, bounds.y + bounds.h, DOS_BLOCK_FULL,
                            color(0, 0, 0), color(0, 0, 0));
            }
        }
    }

    // ========================================================================
    // Lines
    // ========================================================================

    void drawing_context::draw_hline(int x, int y, int width, color fg, color bg, int ch) {
        for (int i = 0; i < width; ++i) {
            m_vram->put(x + i, y, ch, fg, bg);
        }
    }

    void drawing_context::draw_vline(int x, int y, int height, color fg, color bg, int ch) {
        for (int i = 0; i < height; ++i) {
            m_vram->put(x, y + i, ch, fg, bg);
        }
    }

    // ========================================================================
    // Windows
    // ========================================================================

    void drawing_context::draw_window(const rect& bounds, const std::string& title,
                                      color frame_fg, color frame_bg,
                                      color title_fg, color title_bg,
                                      color fill_bg, box_style style) {
        // Draw filled box
        draw_filled_box(bounds, frame_fg, frame_bg, style);

        // Fill background
        for (int y = bounds.y + 1; y < bounds.y + bounds.h - 1; ++y) {
            for (int x = bounds.x + 1; x < bounds.x + bounds.w - 1; ++x) {
                m_vram->put(x, y, ' ', frame_fg, fill_bg);
            }
        }

        // Draw title bar if title provided
        if (!title.empty()) {
            int title_width = std::min(static_cast <int>(title.length()) + 4,
                                       bounds.w - 2);
            int title_x = bounds.x + (bounds.w - title_width) / 2;

            // Draw title background
            for (int x = title_x; x < title_x + title_width; ++x) {
                m_vram->put(x, bounds.y, ' ', title_fg, title_bg);
            }

            // Draw title text
            draw_text(title_x + 2, bounds.y, title, title_fg, title_bg);

            // Draw title bar connectors
            if (title_x > bounds.x + 1) {
                m_vram->put(title_x - 1, bounds.y, DOS_R2, frame_fg, frame_bg);
            }
            if (title_x + title_width < bounds.x + bounds.w - 1) {
                m_vram->put(title_x + title_width, bounds.y, DOS_L2, frame_fg, frame_bg);
            }
        }
    }

    // ========================================================================
    // Progress Bars
    // ========================================================================

    void drawing_context::draw_progress(const rect& bounds, float percent,
                                        color fg, color bg, color bar_fg, color bar_bg) {
        // Clamp percent to 0-100
        percent = std::max(0.0f, std::min(100.0f, percent));

        // Draw background
        draw_filled_box(bounds, fg, bg, box_style::single);

        // Calculate filled width
        int inner_width = bounds.w - 2;
        int filled_width = static_cast <int>(inner_width * percent / 100.0f);

        // Draw filled portion
        for (int x = bounds.x + 1; x <= bounds.x + filled_width; ++x) {
            for (int y = bounds.y + 1; y < bounds.y + bounds.h - 1; ++y) {
                m_vram->put(x, y, DOS_BLOCK_FULL, bar_fg, bar_bg);
            }
        }

        // Draw percentage text in center
        std::string percent_text = std::to_string(static_cast <int>(percent)) + "%";
        int text_x = bounds.x + (bounds.w - percent_text.length()) / 2;
        int text_y = bounds.y + bounds.h / 2;

        // Use contrasting colors for text
        for (size_t i = 0; i < percent_text.length(); ++i) {
            int x = text_x + i;
            bool in_filled = (x <= bounds.x + filled_width);
            color text_fg = in_filled ? bg : bar_fg;
            color text_bg = in_filled ? bar_bg : bg;
            m_vram->put(x, text_y, percent_text[i], text_fg, text_bg);
        }
    }

    // ========================================================================
    // Menus
    // ========================================================================

    void drawing_context::draw_menu_bar(int y, const std::vector <menu_item>& items,
                                        color fg, color bg, color sel_fg, color sel_bg) {
        // Fill menu bar background
        draw_hline(0, y, m_vram->get_width(), fg, bg, ' ');

        int x = 2; // Start with some padding
        for (const auto& item : items) {
            color item_fg = item.selected ? sel_fg : (item.enabled ? fg : color(128, 128, 128));
            color item_bg = item.selected ? sel_bg : bg;

            // Draw item with padding
            m_vram->put(x++, y, ' ', item_fg, item_bg);
            for (char c : item.text) {
                m_vram->put(x++, y, c, item_fg, item_bg);
            }
            m_vram->put(x++, y, ' ', item_fg, item_bg);

            x++; // Space between items
        }
    }

    void drawing_context::draw_dropdown(const rect& bounds, const std::vector <menu_item>& items,
                                        color fg, color bg, color sel_fg, color sel_bg,
                                        color disabled_fg) {
        // Draw dropdown box with shadow
        draw_filled_box(bounds, fg, bg, box_style::single);
        draw_shadow(bounds);

        // Draw items
        int y = bounds.y + 1;
        for (const auto& item : items) {
            if (y >= bounds.y + bounds.h - 1) break;

            if (item.text == "-") {
                // Separator
                auto chars = get_box_chars(box_style::single);
                m_vram->put(bounds.x, y, chars.l, fg, bg);
                draw_hline(bounds.x + 1, y, bounds.w - 2, fg, bg, chars.h);
                m_vram->put(bounds.x + bounds.w - 1, y, chars.r, fg, bg);
            } else {
                // Menu item
                color item_fg = item.selected ? sel_fg : (item.enabled ? fg : disabled_fg);
                color item_bg = item.selected ? sel_bg : bg;

                // Fill line
                for (int x = bounds.x + 1; x < bounds.x + bounds.w - 1; ++x) {
                    m_vram->put(x, y, ' ', item_fg, item_bg);
                }

                // Draw text
                draw_text(bounds.x + 2, y, item.text, item_fg, item_bg);

                // Draw hotkey if present
                if (item.hotkey > 0 && item.hotkey < static_cast <int>(item.text.length())) {
                    m_vram->put(bounds.x + 2 + item.hotkey, y,
                                item.text[item.hotkey], sel_fg, item_bg);
                }
            }
            y++;
        }
    }

    // ========================================================================
    // Scrollbars
    // ========================================================================

    void drawing_context::draw_vscrollbar(int x, int y, int height, int total_items,
                                          int visible_items, int scroll_pos,
                                          color fg, color bg) {
        if (total_items <= visible_items) {
            // No scrollbar needed
            return;
        }

        // Draw scrollbar track
        m_vram->put(x, y, DOS_ARROW_UP, fg, bg);
        for (int i = 1; i < height - 1; ++i) {
            m_vram->put(x, y + i, DOS_BLOCK_1Q, fg, bg);
        }
        m_vram->put(x, y + height - 1, DOS_ARROW_DOWN, fg, bg);

        // Calculate thumb size and position
        int track_height = height - 2;
        int thumb_height = std::max(1, (visible_items * track_height) / total_items);
        int thumb_pos = 1 + ((scroll_pos * (track_height - thumb_height)) /
                             (total_items - visible_items));

        // Draw thumb
        for (int i = 0; i < thumb_height; ++i) {
            if (y + thumb_pos + i < y + height - 1) {
                m_vram->put(x, y + thumb_pos + i, DOS_BLOCK_FULL, fg, bg);
            }
        }
    }

    void drawing_context::draw_hscrollbar(int x, int y, int width, int total_width,
                                          int visible_width, int scroll_pos,
                                          color fg, color bg) {
        if (total_width <= visible_width) {
            return;
        }

        // Draw scrollbar track
        m_vram->put(x, y, DOS_ARROW_LEFT, fg, bg);
        for (int i = 1; i < width - 1; ++i) {
            m_vram->put(x + i, y, DOS_BLOCK_1Q, fg, bg);
        }
        m_vram->put(x + width - 1, y, DOS_ARROW_RIGHT, fg, bg);

        // Calculate thumb size and position
        int track_width = width - 2;
        int thumb_width = std::max(1, (visible_width * track_width) / total_width);
        int thumb_pos = 1 + ((scroll_pos * (track_width - thumb_width)) /
                             (total_width - visible_width));

        // Draw thumb
        for (int i = 0; i < thumb_width; ++i) {
            if (x + thumb_pos + i < x + width - 1) {
                m_vram->put(x + thumb_pos + i, y, DOS_BLOCK_FULL, fg, bg);
            }
        }
    }

    // ========================================================================
    // Status Bar
    // ========================================================================

    void drawing_context::draw_status_bar(int y, const std::string& left_text,
                                          const std::string& right_text,
                                          color fg, color bg) {
        // Fill background
        draw_hline(0, y, m_vram->get_width(), fg, bg, ' ');

        // Draw left text
        draw_text(1, y, left_text, fg, bg);

        // Draw right text
        if (!right_text.empty()) {
            draw_text_right(0, y, m_vram->get_width() - 1, right_text, fg, bg);
        }
    }

    // ========================================================================
    // Buttons
    // ========================================================================

    void drawing_context::draw_button(const rect& bounds, const std::string& text,
                                      bool pressed, bool focused, bool enabled,
                                      color fg, color bg) {
        // Determine colors
        if (!enabled) {
            fg = color(128, 128, 128);
        } else if (pressed) {
            std::swap(fg, bg);
        }

        // Draw button frame
        if (pressed) {
            // Pressed button - single line, no shadow
            draw_filled_box(bounds, fg, bg, box_style::single);
        } else {
            // Normal button - with shadow effect
            draw_filled_box(bounds, fg, bg, box_style::single);
            if (!focused) {
                draw_shadow(rect(bounds.x, bounds.y, bounds.w - 1, bounds.h - 1));
            }
        }

        // Draw button text centered
        draw_text_centered(bounds, text, fg, bg);

        // Draw focus indicator
        if (focused && enabled) {
            m_vram->put(bounds.x + 1, bounds.y + bounds.h / 2, '<', fg, bg);
            m_vram->put(bounds.x + bounds.w - 2, bounds.y + bounds.h / 2, '>', fg, bg);
        }
    }

    // ========================================================================
    // Input Fields
    // ========================================================================

    void drawing_context::draw_input_field(const rect& bounds, const std::string& text,
                                           int cursor_pos, bool focused,
                                           color fg, color bg, color cursor_fg) {
        // Draw field background
        draw_filled_box(bounds, fg, bg, box_style::single);

        // Calculate visible text range
        int field_width = bounds.w - 2;
        int text_len = static_cast <int>(text.length());
        int visible_start = 0;

        if (cursor_pos > field_width - 1) {
            visible_start = cursor_pos - field_width + 1;
        }

        // Draw visible text
        int y = bounds.y + bounds.h / 2;
        for (int i = 0; i < field_width && visible_start + i < text_len; ++i) {
            m_vram->put(bounds.x + 1 + i, y, text[visible_start + i], fg, bg);
        }

        // Draw cursor if focused
        if (focused && cursor_pos >= visible_start &&
            cursor_pos - visible_start < field_width) {
            int cursor_x = bounds.x + 1 + (cursor_pos - visible_start);
            char cursor_char = (cursor_pos < text_len) ? text[cursor_pos] : ' ';
            m_vram->put(cursor_x, y, cursor_char, cursor_fg, bg);
        }
    }

    // ========================================================================
    // Lists
    // ========================================================================

    void drawing_context::draw_list_item(int x, int y, int width, const std::string& text,
                                         bool selected, bool focused,
                                         color fg, color bg, color sel_fg, color sel_bg) {
        color item_fg = selected ? sel_fg : fg;
        color item_bg = selected ? sel_bg : bg;

        // Fill background
        for (int i = 0; i < width; ++i) {
            m_vram->put(x + i, y, ' ', item_fg, item_bg);
        }

        // Draw text (truncate if needed)
        std::string display_text = text;
        if (static_cast <int>(display_text.length()) > width - 2) {
            display_text = display_text.substr(0, width - 5) + "...";
        }
        draw_text(x + 1, y, display_text, item_fg, item_bg);

        // Draw focus indicator
        if (focused && selected) {
            m_vram->put(x, y, '>', sel_fg, sel_bg);
        }
    }

    // ========================================================================
    // Tables/Grids
    // ========================================================================

    void drawing_context::draw_grid(const rect& bounds, int cols, int rows,
                                    color fg, color bg) {
        if (cols <= 0 || rows <= 0) return;

        // Draw outer box
        draw_box(bounds, fg, bg, box_style::single);

        // Calculate cell dimensions
        int cell_width = (bounds.w - 1) / cols;
        int cell_height = (bounds.h - 1) / rows;

        auto chars = get_box_chars(box_style::single);

        // Draw vertical lines
        for (int col = 1; col < cols; ++col) {
            int x = bounds.x + col * cell_width;

            // Top junction
            m_vram->put(x, bounds.y, chars.t, fg, bg);

            // Vertical lines
            for (int y = bounds.y + 1; y < bounds.y + bounds.h - 1; ++y) {
                // Check for horizontal line intersection
                bool on_hline = false;
                for (int row = 1; row < rows; ++row) {
                    if (y == bounds.y + row * cell_height) {
                        m_vram->put(x, y, chars.x, fg, bg);
                        on_hline = true;
                        break;
                    }
                }
                if (!on_hline) {
                    m_vram->put(x, y, chars.v, fg, bg);
                }
            }

            // Bottom junction
            m_vram->put(x, bounds.y + bounds.h - 1, chars.b, fg, bg);
        }

        // Draw horizontal lines
        for (int row = 1; row < rows; ++row) {
            int y = bounds.y + row * cell_height;

            // Left junction
            m_vram->put(bounds.x, y, chars.l, fg, bg);

            // Horizontal lines (skip intersections already drawn)
            for (int x = bounds.x + 1; x < bounds.x + bounds.w - 1; ++x) {
                bool on_vline = false;
                for (int col = 1; col < cols; ++col) {
                    if (x == bounds.x + col * cell_width) {
                        on_vline = true;
                        break;
                    }
                }
                if (!on_vline) {
                    m_vram->put(x, y, chars.h, fg, bg);
                }
            }

            // Right junction
            m_vram->put(bounds.x + bounds.w - 1, y, chars.r, fg, bg);
        }
    }

    void drawing_context::draw_grid_cell(const rect& cell_bounds, const std::string& text,
                                         int alignment, color fg, color bg) {
        // Fill cell background
        for (int y = cell_bounds.y; y < cell_bounds.y + cell_bounds.h; ++y) {
            for (int x = cell_bounds.x; x < cell_bounds.x + cell_bounds.w; ++x) {
                m_vram->put(x, y, ' ', fg, bg);
            }
        }

        // Draw text with alignment
        int text_y = cell_bounds.y + cell_bounds.h / 2;

        switch (alignment) {
            case 0: // Left
                draw_text(cell_bounds.x + 1, text_y, text, fg, bg);
                break;
            case 1: // Center
                draw_text_centered(cell_bounds, text, fg, bg);
                break;
            case 2: // Right
                draw_text_right(cell_bounds.x, text_y, cell_bounds.w - 1, text, fg, bg);
                break;
        }
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================

    std::vector <std::string> drawing_context::wrap_text(const std::string& text, int width) const {
        std::vector <std::string> lines;
        std::istringstream words(text);
        std::string word;
        std::string current_line;

        while (words >> word) {
            if (current_line.empty()) {
                current_line = word;
            } else if (static_cast <int>(current_line.length() + word.length() + 1) <= width) {
                current_line += " " + word;
            } else {
                lines.push_back(current_line);
                current_line = word;
            }
        }

        if (!current_line.empty()) {
            lines.push_back(current_line);
        }

        return lines;
    }

    bool drawing_context::clip_rect(rect& r) const {
        rect screen(0, 0, m_vram->get_width(), m_vram->get_height());

        // Check if completely outside
        if (r.x >= screen.w || r.y >= screen.h ||
            r.x + r.w <= 0 || r.y + r.h <= 0) {
            return false;
        }

        // Clip to screen bounds
        if (r.x < 0) {
            r.w += r.x;
            r.x = 0;
        }
        if (r.y < 0) {
            r.h += r.y;
            r.y = 0;
        }
        if (r.x + r.w > screen.w) {
            r.w = screen.w - r.x;
        }
        if (r.y + r.h > screen.h) {
            r.h = screen.h - r.y;
        }

        return r.w > 0 && r.h > 0;
    }

    bool drawing_context::in_bounds(int x, int y) const {
        return x >= 0 && x < m_vram->get_width() &&
               y >= 0 && y < m_vram->get_height();
    }

    // ========================================================================
    // Color Schemes
    // ========================================================================

    namespace color_schemes {
        const theme norton_utilities = {
            // Window colors
            .window_frame_fg = color(255, 255, 255),
            .window_frame_bg = color(0, 0, 128),
            .window_title_fg = color(0, 0, 0),
            .window_title_bg = color(192, 192, 192),
            .window_bg = color(0, 0, 128),

            // Widget colors
            .button_fg = color(0, 0, 0),
            .button_bg = color(192, 192, 192),
            .button_focused_fg = color(255, 255, 255),
            .button_focused_bg = color(0, 128, 128),
            .button_pressed_fg = color(255, 255, 255),
            .button_pressed_bg = color(0, 64, 64),
            .button_disabled_fg = color(128, 128, 128),
            .button_disabled_bg = color(192, 192, 192),

            // Text colors
            .text_fg = color(255, 255, 255),
            .text_bg = color(0, 0, 128),
            .text_selected_fg = color(255, 255, 255),
            .text_selected_bg = color(0, 128, 128),
            .text_disabled_fg = color(128, 128, 128),

            // Menu colors
            .menu_fg = color(0, 0, 0),
            .menu_bg = color(192, 192, 192),
            .menu_selected_fg = color(255, 255, 255),
            .menu_selected_bg = color(0, 0, 128),
            .menu_hotkey_fg = color(255, 0, 0),

            // Status bar
            .status_fg = color(255, 255, 255),
            .status_bg = color(0, 0, 64),

            // Shadows
            .shadow = color(0, 0, 0)
        };

        const theme turbo_vision = {
            // Window colors
            .window_frame_fg = color(255, 255, 255),
            .window_frame_bg = color(0, 128, 128),
            .window_title_fg = color(255, 255, 0),
            .window_title_bg = color(0, 128, 128),
            .window_bg = color(0, 128, 128),

            // Widget colors
            .button_fg = color(0, 0, 0),
            .button_bg = color(0, 255, 255),
            .button_focused_fg = color(255, 255, 255),
            .button_focused_bg = color(0, 255, 0),
            .button_pressed_fg = color(255, 255, 0),
            .button_pressed_bg = color(0, 128, 0),
            .button_disabled_fg = color(128, 128, 128),
            .button_disabled_bg = color(0, 128, 128),

            // Text colors
            .text_fg = color(255, 255, 255),
            .text_bg = color(0, 128, 128),
            .text_selected_fg = color(255, 255, 0),
            .text_selected_bg = color(128, 0, 128),
            .text_disabled_fg = color(128, 128, 128),

            // Menu colors
            .menu_fg = color(0, 0, 0),
            .menu_bg = color(192, 192, 192),
            .menu_selected_fg = color(255, 255, 255),
            .menu_selected_bg = color(0, 0, 0),
            .menu_hotkey_fg = color(255, 0, 0),

            // Status bar
            .status_fg = color(0, 0, 0),
            .status_bg = color(0, 255, 255),

            // Shadows
            .shadow = color(0, 0, 0)
        };

        const theme midnight_commander = {
            // Window colors
            .window_frame_fg = color(255, 255, 255),
            .window_frame_bg = color(0, 0, 128),
            .window_title_fg = color(255, 255, 0),
            .window_title_bg = color(0, 0, 128),
            .window_bg = color(0, 0, 128),

            // Widget colors
            .button_fg = color(0, 0, 0),
            .button_bg = color(192, 192, 192),
            .button_focused_fg = color(255, 255, 255),
            .button_focused_bg = color(0, 128, 128),
            .button_pressed_fg = color(255, 255, 0),
            .button_pressed_bg = color(128, 0, 0),
            .button_disabled_fg = color(128, 128, 128),
            .button_disabled_bg = color(64, 64, 64),

            // Text colors
            .text_fg = color(192, 192, 192),
            .text_bg = color(0, 0, 128),
            .text_selected_fg = color(255, 255, 0),
            .text_selected_bg = color(0, 128, 128),
            .text_disabled_fg = color(128, 128, 128),

            // Menu colors
            .menu_fg = color(0, 0, 0),
            .menu_bg = color(0, 255, 255),
            .menu_selected_fg = color(255, 255, 255),
            .menu_selected_bg = color(0, 0, 0),
            .menu_hotkey_fg = color(255, 255, 0),

            // Status bar
            .status_fg = color(0, 0, 0),
            .status_bg = color(192, 192, 192),

            // Shadows
            .shadow = color(0, 0, 0)
        };

        const theme dos_edit = {
            // Window colors
            .window_frame_fg = color(255, 255, 255),
            .window_frame_bg = color(0, 0, 128),
            .window_title_fg = color(255, 255, 255),
            .window_title_bg = color(0, 0, 128),
            .window_bg = color(0, 0, 128),

            // Widget colors
            .button_fg = color(0, 0, 0),
            .button_bg = color(192, 192, 192),
            .button_focused_fg = color(0, 0, 0),
            .button_focused_bg = color(255, 255, 255),
            .button_pressed_fg = color(255, 255, 255),
            .button_pressed_bg = color(0, 0, 0),
            .button_disabled_fg = color(128, 128, 128),
            .button_disabled_bg = color(192, 192, 192),

            // Text colors
            .text_fg = color(255, 255, 255),
            .text_bg = color(0, 0, 128),
            .text_selected_fg = color(0, 0, 128),
            .text_selected_bg = color(192, 192, 192),
            .text_disabled_fg = color(128, 128, 128),

            // Menu colors
            .menu_fg = color(0, 0, 0),
            .menu_bg = color(192, 192, 192),
            .menu_selected_fg = color(255, 255, 255),
            .menu_selected_bg = color(0, 0, 0),
            .menu_hotkey_fg = color(255, 0, 0),

            // Status bar
            .status_fg = color(255, 255, 255),
            .status_bg = color(0, 0, 0),

            // Shadows
            .shadow = color(0, 0, 0)
        };

        const theme modern_dark = {
            // Window colors
            .window_frame_fg = color(200, 200, 200),
            .window_frame_bg = color(40, 40, 40),
            .window_title_fg = color(255, 255, 255),
            .window_title_bg = color(60, 60, 60),
            .window_bg = color(30, 30, 30),

            // Widget colors
            .button_fg = color(255, 255, 255),
            .button_bg = color(70, 70, 70),
            .button_focused_fg = color(255, 255, 255),
            .button_focused_bg = color(0, 120, 212),
            .button_pressed_fg = color(255, 255, 255),
            .button_pressed_bg = color(0, 90, 158),
            .button_disabled_fg = color(128, 128, 128),
            .button_disabled_bg = color(50, 50, 50),

            // Text colors
            .text_fg = color(230, 230, 230),
            .text_bg = color(30, 30, 30),
            .text_selected_fg = color(255, 255, 255),
            .text_selected_bg = color(0, 120, 212),
            .text_disabled_fg = color(128, 128, 128),

            // Menu colors
            .menu_fg = color(230, 230, 230),
            .menu_bg = color(50, 50, 50),
            .menu_selected_fg = color(255, 255, 255),
            .menu_selected_bg = color(0, 120, 212),
            .menu_hotkey_fg = color(100, 180, 255),

            // Status bar
            .status_fg = color(200, 200, 200),
            .status_bg = color(20, 20, 20),

            // Shadows
            .shadow = color(0, 0, 0)
        };

        const theme modern_light = {
            // Window colors
            .window_frame_fg = color(100, 100, 100),
            .window_frame_bg = color(240, 240, 240),
            .window_title_fg = color(255, 255, 255),
            .window_title_bg = color(0, 120, 212),
            .window_bg = color(255, 255, 255),

            // Widget colors
            .button_fg = color(0, 0, 0),
            .button_bg = color(225, 225, 225),
            .button_focused_fg = color(255, 255, 255),
            .button_focused_bg = color(0, 120, 212),
            .button_pressed_fg = color(255, 255, 255),
            .button_pressed_bg = color(0, 90, 158),
            .button_disabled_fg = color(160, 160, 160),
            .button_disabled_bg = color(240, 240, 240),

            // Text colors
            .text_fg = color(0, 0, 0),
            .text_bg = color(255, 255, 255),
            .text_selected_fg = color(255, 255, 255),
            .text_selected_bg = color(0, 120, 212),
            .text_disabled_fg = color(160, 160, 160),

            // Menu colors
            .menu_fg = color(0, 0, 0),
            .menu_bg = color(240, 240, 240),
            .menu_selected_fg = color(255, 255, 255),
            .menu_selected_bg = color(0, 120, 212),
            .menu_hotkey_fg = color(0, 90, 158),

            // Status bar
            .status_fg = color(0, 0, 0),
            .status_bg = color(225, 225, 225),

            // Shadows
            .shadow = color(180, 180, 180)
        };
    }

    // ========================================================================
    // Animation Utilities
    // ========================================================================

    void animation::fade_in(drawing_context& ctx, const rect& bounds,
                            float progress, color from, color to) {
        // Linear interpolation of colors
        auto lerp = [](uint8_t a, uint8_t b, float t) -> uint8_t {
            return static_cast <uint8_t>(a + (b - a) * t);
        };

        color current(
            lerp(from.r, to.r, progress),
            lerp(from.g, to.g, progress),
            lerp(from.b, to.b, progress)
        );

        ctx.draw_filled_box(bounds, current, current, box_style::single);
    }

    void animation::slide_in(drawing_context& ctx, const rect& from,
                             const rect& to, float progress) {
        // Linear interpolation of rect
        auto lerp = [](int a, int b, float t) -> int {
            return static_cast <int>(a + (b - a) * t);
        };

        rect current(
            lerp(from.x, to.x, progress),
            lerp(from.y, to.y, progress),
            lerp(from.w, to.w, progress),
            lerp(from.h, to.h, progress)
        );

        ctx.draw_filled_box(current, color(255, 255, 255), color(0, 0, 0), box_style::single);
    }

    void animation::spinner(drawing_context& ctx, int x, int y,
                            int frame, color fg, color bg) {
        const char* frames[] = {"|", "/", "-", "\\"};
        const char* spinner_frame = frames[frame % 4];
        ctx.draw_text(x, y, spinner_frame, fg, bg);
    }

    // ========================================================================
    // ASCII Art Utilities
    // ========================================================================

    namespace ascii_art {
        void draw_logo(drawing_context& ctx, int x, int y,
                       const std::vector <std::string>& lines,
                       color fg, color bg) {
            for (size_t i = 0; i < lines.size(); ++i) {
                ctx.draw_text(x, y + i, lines[i], fg, bg);
            }
        }

        void draw_banner(drawing_context& ctx, const rect& bounds,
                         const std::string& text, color fg, color bg) {
            // Draw decorative banner
            ctx.draw_filled_box(bounds, fg, bg, box_style::rounded);

            // Draw banner text centered with decorations
            std::string decorated = "=[ " + text + " ]=";
            ctx.draw_text_centered(bounds, decorated, fg, bg);
        }
    }
}
