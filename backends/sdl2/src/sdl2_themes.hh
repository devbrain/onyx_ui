/**
 * @file sdl2_themes.hh
 * @brief Built-in themes for the SDL2 backend
 * @date 2025-10-22
 *
 * @details
 * Provides classic GUI themes, starting with Windows 3.11.
 */

#pragma once

#include <onyxui/theme.hh>
#include <onyxui/theme_registry.hh>
#include <onyxui/sdl2/sdl2_backend.hh>
#include <onyxui/sdl2/colors.hh>
#include <onyxui/sdl2/sdl2_renderer.hh>
#include <onyxui/layout_strategy.hh>

namespace onyxui::sdl2 {

    /**
     * @class sdl2_themes
     * @brief Provider of built-in SDL2 themes
     *
     * @details
     * Static class providing classic GUI themes for SDL2 backend.
     */
    class sdl2_themes {
    public:
        using Backend = sdl2_backend;
        using theme_type = ui_theme<Backend>;

        /**
         * @brief Register all built-in SDL2 themes
         * @param registry Theme registry to populate
         *
         * @details
         * Registers the following themes (in order, first is default):
         * 1. "Windows 3.11" (default) - Classic Windows 3.x look
         *
         * This method is called automatically by sdl2_backend::register_themes(),
         * which is invoked on first ui_context creation. Manual registration is
         * not required.
         */
        static void register_themes(theme_registry<Backend>& registry) {
            registry.register_theme(create_windows_311());
        }

        /**
         * @brief Create "Windows 3.11" theme
         * @return Theme with classic Windows 3.x colors and styling
         *
         * @details
         * Authentic Windows 3.1 color scheme:
         * - Window background: Gray (192, 192, 192) - The iconic gray
         * - Button highlight: White (255, 255, 255) - Top-left bevel
         * - Button shadow: Dark gray (128, 128, 128) - Bottom-right bevel
         * - Active title: Navy blue (0, 0, 128)
         * - Raised/sunken borders with 2px bevels
         */
        static theme_type create_windows_311() {
            theme_type theme;
            theme.name = "Windows 3.11";
            theme.description = "Classic Windows 3.x user interface";

            // Windows 3.x standard colors
            constexpr color win_gray{192, 192, 192};      // Button face / window background
            constexpr color win_dark_gray{128, 128, 128}; // Button shadow
            constexpr color win_white{255, 255, 255};     // Button highlight
            constexpr color win_black{0, 0, 0};           // Window text
            constexpr color win_navy{0, 0, 128};          // Active title bar
            constexpr color win_light_gray{223, 223, 223}; // Menu bar

            // Global palette
            theme.window_bg = win_gray;
            theme.text_fg = win_black;
            theme.border_color = win_dark_gray;

            // Button style - classic Windows 3.x raised button
            theme.button.fg_normal = win_black;
            theme.button.bg_normal = win_gray;
            theme.button.fg_hover = win_black;
            theme.button.bg_hover = win_gray;          // Same as normal
            theme.button.fg_pressed = win_black;
            theme.button.bg_pressed = win_gray;        // Same as normal
            theme.button.fg_disabled = win_dark_gray;
            theme.button.bg_disabled = win_gray;
            theme.button.box_style = sdl2_renderer::box_style::raised; // 3D raised border
            theme.button.padding_horizontal = 8;       // Wider padding for pixel-based UI
            theme.button.padding_vertical = 4;
            theme.button.text_align = horizontal_alignment::center;

            // Label style
            theme.label.text = win_black;
            theme.label.background = win_gray;

            // Panel style
            theme.panel.background = win_gray;
            theme.panel.border_color = win_dark_gray;
            theme.panel.box_style = sdl2_renderer::box_style::sunken; // Classic Windows panel look

            return theme;
        }
    };

} // namespace onyxui::sdl2
