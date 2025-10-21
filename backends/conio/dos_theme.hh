//
// Created by igor on 16/10/2025.
//

#pragma once

#include <onyxui/theme.hh>
#include <onyxui/layout_strategy.hh>  // For horizontal_alignment enum values
#include "include/onyxui/conio/conio_backend.hh"

namespace onyxui::conio {
    /**
     * @brief Classic DOS color palette
     *
     * Inspired by Norton Commander, Norton Utilities, and other DOS applications
     * from the late 80s and early 90s.
     */
    namespace dos_colors {
        // Primary DOS colors
        constexpr color DOS_BLUE        {0,   0,   170};  // Classic DOS blue background
        constexpr color DOS_CYAN        {0,   255, 255};  // Bright cyan (active elements)
        constexpr color DOS_YELLOW      {255, 255, 0};    // Bright yellow (highlights)
        constexpr color DOS_WHITE       {255, 255, 255};  // Bright white (text)
        constexpr color DOS_LIGHT_GRAY  {192, 192, 192};  // Light gray (normal text)
        constexpr color DOS_DARK_GRAY   {128, 128, 128};  // Dark gray (disabled)
        constexpr color DOS_BLACK       {0,   0,   0};    // Black (shadows, borders)
        constexpr color DOS_GREEN       {0,   255, 0};    // Bright green (success)
        constexpr color DOS_RED         {255, 0,   0};    // Bright red (errors)
        constexpr color DOS_MAGENTA     {255, 0,   255};  // Bright magenta (special)

        // Extended palette
        constexpr color DOS_DARK_BLUE   {0,   0,   128};  // Dark blue (alternate background)
        constexpr color DOS_DARK_CYAN   {0,   128, 128};  // Dark cyan (alternate highlight)
        constexpr color DOS_BROWN       {170, 85,  0};    // Brown (warnings)
    } // namespace dos_colors

    /**
     * @brief Create a classic DOS blue theme
     *
     * This theme recreates the iconic look of DOS applications from the 1980s-1990s:
     * - Blue background with white/cyan text
     * - Yellow highlights for active elements
     * - Single-line borders
     * - High contrast for readability on CRT monitors
     *
     * Perfect for:
     * - Norton Commander-style file managers
     * - System utilities
     * - Text-mode applications
     * - Retro/nostalgic UIs
     *
     * @return DOS blue theme
     *
     * @example
     * @code
     * auto theme = create_dos_blue_theme();
     * root->apply_theme(theme);
     * @endcode
     */
    inline ui_theme<conio_backend> create_dos_blue_theme() {
        ui_theme<conio_backend> theme;

        // =================================================================
        // Global Palette
        // =================================================================
        theme.window_bg = dos_colors::DOS_BLUE;
        theme.text_fg = dos_colors::DOS_WHITE;
        theme.border_color = dos_colors::DOS_CYAN;

        // =================================================================
        // Button Style
        // =================================================================
        theme.button.fg_normal = dos_colors::DOS_BLACK;
        theme.button.bg_normal = dos_colors::DOS_LIGHT_GRAY;

        theme.button.fg_hover = dos_colors::DOS_YELLOW;
        theme.button.bg_hover = dos_colors::DOS_CYAN;

        theme.button.fg_pressed = dos_colors::DOS_BLACK;
        theme.button.bg_pressed = dos_colors::DOS_YELLOW;

        theme.button.fg_disabled = dos_colors::DOS_DARK_GRAY;
        theme.button.bg_disabled = dos_colors::DOS_BLUE;

        theme.button.box_style = conio_renderer::box_style::single_line;
        theme.button.font = conio_renderer::font{false, false, false};
        theme.button.mnemonic_font = conio_renderer::font{false, true, false};  // Underlined

        theme.button.padding_horizontal = 1;  // Minimal horizontal padding
        theme.button.padding_vertical = 0;    // No vertical padding for compact DOS look
        theme.button.text_align = horizontal_alignment::center;  // Center text in button
        theme.button.corner_radius = 0;  // Sharp corners

        // =================================================================
        // Label Style
        // =================================================================
        theme.label.text = dos_colors::DOS_WHITE;
        theme.label.background = dos_colors::DOS_BLUE;
        theme.label.font = conio_renderer::font{false, false, false};
        theme.label.mnemonic_font = conio_renderer::font{false, true, false};  // Underlined

        // =================================================================
        // Panel Style
        // =================================================================
        theme.panel.background = dos_colors::DOS_BLUE;
        theme.panel.border_color = dos_colors::DOS_CYAN;
        theme.panel.box_style = conio_renderer::box_style::single_line;
        theme.panel.has_border = true;

        return theme;
    }

    /**
     * @brief Create a dark DOS theme (Norton Commander style)
     *
     * This theme uses a black background with cyan accents, similar to
     * Norton Commander's classic look:
     * - Black background
     * - Cyan borders and highlights
     * - Yellow for active/selected items
     * - High contrast for professional look
     *
     * @return Dark DOS theme
     */
    inline ui_theme<conio_backend> create_dos_dark_theme() {
        ui_theme<conio_backend> theme;

        // =================================================================
        // Global Palette
        // =================================================================
        theme.window_bg = dos_colors::DOS_BLACK;
        theme.text_fg = dos_colors::DOS_CYAN;
        theme.border_color = dos_colors::DOS_CYAN;

        // =================================================================
        // Button Style
        // =================================================================
        theme.button.fg_normal = dos_colors::DOS_BLACK;
        theme.button.bg_normal = dos_colors::DOS_CYAN;

        theme.button.fg_hover = dos_colors::DOS_BLACK;
        theme.button.bg_hover = dos_colors::DOS_YELLOW;

        theme.button.fg_pressed = dos_colors::DOS_BLACK;
        theme.button.bg_pressed = dos_colors::DOS_GREEN;

        theme.button.fg_disabled = dos_colors::DOS_DARK_GRAY;
        theme.button.bg_disabled = dos_colors::DOS_BLACK;

        theme.button.box_style = conio_renderer::box_style::single_line;
        theme.button.font = conio_renderer::font{false, false, false};
        theme.button.mnemonic_font = conio_renderer::font{false, true, false};

        theme.button.padding_horizontal = 1;  // Minimal horizontal padding
        theme.button.padding_vertical = 0;    // No vertical padding for compact DOS look
        theme.button.text_align = horizontal_alignment::center;  // Center text in button
        theme.button.corner_radius = 0;

        // =================================================================
        // Label Style
        // =================================================================
        theme.label.text = dos_colors::DOS_CYAN;
        theme.label.background = dos_colors::DOS_BLACK;
        theme.label.font = conio_renderer::font{false, false, false};
        theme.label.mnemonic_font = conio_renderer::font{false, true, false};

        // =================================================================
        // Panel Style
        // =================================================================
        theme.panel.background = dos_colors::DOS_BLACK;
        theme.panel.border_color = dos_colors::DOS_CYAN;
        theme.panel.box_style = conio_renderer::box_style::double_line;  // Double lines for NC style
        theme.panel.has_border = true;

        return theme;
    }

    /**
     * @brief Create a monochrome DOS theme
     *
     * For systems with monochrome monitors (Hercules, etc.):
     * - Black and white only
     * - Uses reverse video for highlights
     * - Bold for emphasis
     *
     * @return Monochrome DOS theme
     */
    inline ui_theme<conio_backend> create_dos_monochrome_theme() {
        ui_theme<conio_backend> theme;

        // =================================================================
        // Global Palette
        // =================================================================
        theme.window_bg = dos_colors::DOS_BLACK;
        theme.text_fg = dos_colors::DOS_WHITE;
        theme.border_color = dos_colors::DOS_WHITE;

        // =================================================================
        // Button Style
        // =================================================================
        theme.button.fg_normal = dos_colors::DOS_WHITE;
        theme.button.bg_normal = dos_colors::DOS_BLACK;

        theme.button.fg_hover = dos_colors::DOS_BLACK;
        theme.button.bg_hover = dos_colors::DOS_WHITE;  // Reverse video

        theme.button.fg_pressed = dos_colors::DOS_BLACK;
        theme.button.bg_pressed = dos_colors::DOS_LIGHT_GRAY;

        theme.button.fg_disabled = dos_colors::DOS_DARK_GRAY;
        theme.button.bg_disabled = dos_colors::DOS_BLACK;

        theme.button.box_style = conio_renderer::box_style::single_line;
        theme.button.font = conio_renderer::font{false, false, false};
        theme.button.mnemonic_font = conio_renderer::font{true, false, false};  // Bold

        theme.button.padding_horizontal = 1;  // Minimal horizontal padding
        theme.button.padding_vertical = 0;    // No vertical padding for compact DOS look
        theme.button.text_align = horizontal_alignment::center;  // Center text in button
        theme.button.corner_radius = 0;

        // =================================================================
        // Label Style
        // =================================================================
        theme.label.text = dos_colors::DOS_WHITE;
        theme.label.background = dos_colors::DOS_BLACK;
        theme.label.font = conio_renderer::font{false, false, false};
        theme.label.mnemonic_font = conio_renderer::font{true, false, false};  // Bold

        // =================================================================
        // Panel Style
        // =================================================================
        theme.panel.background = dos_colors::DOS_BLACK;
        theme.panel.border_color = dos_colors::DOS_WHITE;
        theme.panel.box_style = conio_renderer::box_style::single_line;
        theme.panel.has_border = true;

        return theme;
    }

    /**
     * @brief Create a Norton Utilities theme (amber on brown)
     *
     * Inspired by Norton Utilities' distinctive color scheme:
     * - Brown/dark background
     * - Yellow/amber text
     * - Professional utility look
     *
     * @return Norton Utilities style theme
     */
    inline ui_theme<conio_backend> create_norton_utilities_theme() {
        ui_theme<conio_backend> theme;

        // =================================================================
        // Global Palette
        // =================================================================
        theme.window_bg = dos_colors::DOS_BROWN;
        theme.text_fg = dos_colors::DOS_YELLOW;
        theme.border_color = dos_colors::DOS_YELLOW;

        // =================================================================
        // Button Style
        // =================================================================
        theme.button.fg_normal = dos_colors::DOS_BLACK;
        theme.button.bg_normal = dos_colors::DOS_YELLOW;

        theme.button.fg_hover = dos_colors::DOS_YELLOW;
        theme.button.bg_hover = dos_colors::DOS_BROWN;

        theme.button.fg_pressed = dos_colors::DOS_BROWN;
        theme.button.bg_pressed = dos_colors::DOS_WHITE;

        theme.button.fg_disabled = dos_colors::DOS_DARK_GRAY;
        theme.button.bg_disabled = dos_colors::DOS_BROWN;

        theme.button.box_style = conio_renderer::box_style::single_line;
        theme.button.font = conio_renderer::font{true, false, false};  // Bold
        theme.button.mnemonic_font = conio_renderer::font{true, true, false};  // Bold + underline

        theme.button.padding_horizontal = 1;  // Minimal horizontal padding
        theme.button.padding_vertical = 0;    // No vertical padding for compact DOS look
        theme.button.text_align = horizontal_alignment::center;  // Center text in button
        theme.button.corner_radius = 0;

        // =================================================================
        // Label Style
        // =================================================================
        theme.label.text = dos_colors::DOS_YELLOW;
        theme.label.background = dos_colors::DOS_BROWN;
        theme.label.font = conio_renderer::font{false, false, false};
        theme.label.mnemonic_font = conio_renderer::font{false, true, false};

        // =================================================================
        // Panel Style
        // =================================================================
        theme.panel.background = dos_colors::DOS_BROWN;
        theme.panel.border_color = dos_colors::DOS_YELLOW;
        theme.panel.box_style = conio_renderer::box_style::double_line;
        theme.panel.has_border = true;

        return theme;
    }

} // namespace onyxui::conio
