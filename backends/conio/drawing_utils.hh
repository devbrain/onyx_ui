//
// Created by igor on 14/10/2025.
//

#pragma once

#include "vram.hh"
#include "rect.hh"
#include "colors.hh"
#include "dos_chars.h"

#include <string>
#include <vector>

namespace onyxui::conio {

    /**
     * @enum box_style
     * @brief Box drawing styles using different line characters
     */
    enum class box_style {
        single = 0,   // Single line box drawing (┌─┐│└─┘)
        double_line,  // Double line box drawing (╔═╗║╚═╝)
        rounded,      // Rounded corners (╭─╮│╰─╯)
        heavy         // Heavy/bold lines
    };

    /**
     * @class drawing_context
     * @brief High-level drawing utilities for the VRAM
     *
     * Provides convenient methods for drawing common UI elements.
     */
    class drawing_context {
    public:
        explicit drawing_context(vram* v) : m_vram(v) {}

        // Text drawing
        void draw_text(int x, int y, const std::string& text, color fg, color bg);
        void draw_text_centered(const rect& bounds, const std::string& text, color fg, color bg);
        void draw_text_right(int x, int y, int width, const std::string& text, color fg, color bg);

        // Box drawing
        void draw_box(const rect& bounds, color fg, color bg, box_style style = box_style::single);
        void draw_filled_box(const rect& bounds, color fg, color bg, box_style style = box_style::single);
        void draw_shadow(const rect& bounds);

        // Lines
        void draw_hline(int x, int y, int width, color fg, color bg, int ch = DOS_H);
        void draw_vline(int x, int y, int height, color fg, color bg, int ch = DOS_V);

        // Windows/Panels
        void draw_window(const rect& bounds, const std::string& title,
                        color frame_fg, color frame_bg,
                        color title_fg, color title_bg,
                        color fill_bg, box_style style = box_style::double_line);

        // Progress bars
        void draw_progress(const rect& bounds, float percent,
                          color fg, color bg, color bar_fg, color bar_bg);

        // Menus
        struct menu_item {
            std::string text;
            bool enabled = true;
            bool selected = false;
            int hotkey = 0;  // Character to highlight
        };

        void draw_menu_bar(int y, const std::vector<menu_item>& items,
                          color fg, color bg, color sel_fg, color sel_bg);

        void draw_dropdown(const rect& bounds, const std::vector<menu_item>& items,
                          color fg, color bg, color sel_fg, color sel_bg,
                          color disabled_fg);

        // Scrollbars
        void draw_vscrollbar(int x, int y, int height, int total_items,
                            int visible_items, int scroll_pos,
                            color fg, color bg);

        void draw_hscrollbar(int x, int y, int width, int total_width,
                            int visible_width, int scroll_pos,
                            color fg, color bg);

        // Status bar
        void draw_status_bar(int y, const std::string& left_text,
                            const std::string& right_text,
                            color fg, color bg);

        // Buttons
        void draw_button(const rect& bounds, const std::string& text,
                        bool pressed, bool focused, bool enabled,
                        color fg, color bg);

        // Input fields
        void draw_input_field(const rect& bounds, const std::string& text,
                             int cursor_pos, bool focused,
                             color fg, color bg, color cursor_fg);

        // Lists
        void draw_list_item(int x, int y, int width, const std::string& text,
                           bool selected, bool focused,
                           color fg, color bg, color sel_fg, color sel_bg);

        // Tables/Grids
        void draw_grid(const rect& bounds, int cols, int rows,
                      color fg, color bg);

        void draw_grid_cell(const rect& cell_bounds, const std::string& text,
                           int alignment, color fg, color bg);

    private:
        vram* m_vram;

        // Helper to get box drawing characters based on style
        struct box_chars {
            int tl, tr, bl, br;  // corners
            int h, v;             // horizontal, vertical
            int t, b, l, r;       // T-junctions
            int x;                // cross
        };

        box_chars get_box_chars(box_style style) const;

        // Text wrapping helper
        std::vector<std::string> wrap_text(const std::string& text, int width) const;

        // Clipping helpers
        bool clip_rect(rect& r) const;
        bool in_bounds(int x, int y) const;
    };

    // Color schemes for consistent UI theming
    namespace color_schemes {
        struct theme {
            // Window colors
            color window_frame_fg, window_frame_bg;
            color window_title_fg, window_title_bg;
            color window_bg;

            // Widget colors
            color button_fg, button_bg;
            color button_focused_fg, button_focused_bg;
            color button_pressed_fg, button_pressed_bg;
            color button_disabled_fg, button_disabled_bg;

            // Text colors
            color text_fg, text_bg;
            color text_selected_fg, text_selected_bg;
            color text_disabled_fg;

            // Menu colors
            color menu_fg, menu_bg;
            color menu_selected_fg, menu_selected_bg;
            color menu_hotkey_fg;

            // Status bar
            color status_fg, status_bg;

            // Shadows
            color shadow;
        };

        // Pre-defined themes
        extern const theme norton_utilities;  // Classic blue Norton style
        extern const theme turbo_vision;      // Borland Turbo Vision style
        extern const theme midnight_commander; // MC style
        extern const theme dos_edit;          // MS-DOS Edit style
        extern const theme modern_dark;       // Modern dark theme
        extern const theme modern_light;      // Modern light theme
    };

    // Animation utilities
    class animation {
    public:
        static void fade_in(drawing_context& ctx, const rect& bounds,
                           float progress, color from, color to);

        static void slide_in(drawing_context& ctx, const rect& from,
                           const rect& to, float progress);

        static void spinner(drawing_context& ctx, int x, int y,
                          int frame, color fg, color bg);
    };

    // ASCII art utilities
    namespace ascii_art {
        void draw_logo(drawing_context& ctx, int x, int y,
                      const std::vector<std::string>& lines,
                      color fg, color bg);

        void draw_banner(drawing_context& ctx, const rect& bounds,
                        const std::string& text, color fg, color bg);
    };
}