/**
 * @file conio_themes.hh
 * @brief Built-in themes for the conio backend
 * @author Assistant
 * @date 2025-10-20
 *
 * @details
 * Provides classic DOS/TUI color schemes inspired by:
 * - Norton Utilities (blue/white)
 * - Borland Turbo Vision (cyan/white)
 * - Midnight Commander (blue/yellow)
 * - DOS Edit (gray/black)
 */

#pragma once

#include <onyxui/theme.hh>
#include <onyxui/theme_registry.hh>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/colors.hh>
#include <onyxui/conio/conio_renderer.hh>
#include <onyxui/layout_strategy.hh>

namespace onyxui::conio {

    /**
     * @class conio_themes
     * @brief Provider of built-in conio themes
     *
     * @details
     * Static class providing classic TUI themes. Themes can be registered
     * in bulk or accessed individually.
     */
    class conio_themes {
    public:
        using Backend = conio_backend;
        using theme_type = ui_theme<Backend>;

        /**
         * @brief Register all built-in conio themes
         * @param registry Theme registry to populate
         *
         * @details
         * Registers the following themes (in order, first is default):
         * 1. "Norton Blue" (default) - Classic Norton Utilities
         * 2. "Borland Turbo" - Turbo Pascal/C++ IDE
         * 3. "Midnight Commander" - MC file manager
         * 4. "DOS Edit" - MS-DOS Edit
         *
         * This method is called automatically by conio_backend::register_themes(),
         * which is invoked on first ui_context creation. Manual registration is
         * not required.
         *
         * @example
         * @code
         * // Automatic registration (happens automatically)
         * scoped_ui_context<conio_backend> ctx;
         * auto* theme = ctx.themes().get_theme("Norton Blue");  // Available!
         *
         * // Manual registration (if needed for custom scenarios)
         * conio_themes::register_themes(my_registry);
         * @endcode
         */
        static void register_themes(theme_registry<Backend>& registry) {
            registry.register_theme(create_norton_blue());
            registry.register_theme(create_borland_turbo());
            registry.register_theme(create_midnight_commander());
            registry.register_theme(create_dos_edit());
        }

        // Deprecated: Use register_themes() instead
        [[deprecated("Use register_themes() instead")]]
        static void register_default_themes(theme_registry<Backend>& registry) {
            register_themes(registry);
        }

        /**
         * @brief Create "Norton Blue" theme
         * @return Theme with classic Norton Utilities colors
         *
         * @details
         * Dark blue background with white text, reminiscent of
         * Norton Utilities, Norton Commander, and similar DOS tools.
         */
        static theme_type create_norton_blue() {
            theme_type theme;
            theme.name = "Norton Blue";
            theme.description = "Classic Norton Utilities color scheme";

            // Global palette
            theme.window_bg = color{0, 0, 170};      // Dark blue
            theme.text_fg = color{255, 255, 255};    // White
            theme.border_color = color{255, 255, 0}; // Yellow

            // Button style
            theme.button.fg_normal = color{255, 255, 255};   // White
            theme.button.bg_normal = color{0, 0, 170};       // Dark blue
            theme.button.fg_hover = color{255, 255, 0};      // Yellow
            theme.button.bg_hover = color{0, 0, 255};        // Bright blue
            theme.button.fg_pressed = color{0, 0, 0};        // Black
            theme.button.bg_pressed = color{170, 170, 170};  // Light gray
            theme.button.fg_disabled = color{85, 85, 85};    // Dark gray
            theme.button.bg_disabled = color{0, 0, 170};     // Dark blue
            theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};
            theme.button.padding_horizontal = 2;
            theme.button.padding_vertical = 0;
            theme.button.text_align = horizontal_alignment::center;

            // Label style
            theme.label.text = color{255, 255, 255};        // White
            theme.label.background = color{0, 0, 170};      // Dark blue

            // Panel style
            theme.panel.background = color{0, 0, 170};      // Dark blue
            theme.panel.border_color = color{255, 255, 0};  // Yellow
            theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

            // Menu style
            theme.menu.background = color{0, 0, 170};       // Dark blue
            theme.menu.border_color = color{255, 255, 0};   // Yellow
            theme.menu.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

            // Menu bar style
            theme.menu_bar.item_spacing = 2;              // 2px between menu items
            theme.menu_bar.item_padding_horizontal = 0;   // No horizontal padding (left-aligned)
            theme.menu_bar.item_padding_vertical = 0;     // No vertical padding

            // Separator style
            theme.separator.line_style = conio_renderer::line_style{conio_renderer::border_style::single_line};

            return theme;
        }

        /**
         * @brief Create "Borland Turbo" theme
         * @return Theme with Turbo Pascal/C++ IDE colors
         */
        static theme_type create_borland_turbo() {
            theme_type theme;
            theme.name = "Borland Turbo";
            theme.description = "Turbo Pascal/C++ IDE color scheme";

            // Global palette (cyan/white)
            theme.window_bg = color{0, 170, 170};    // Cyan
            theme.text_fg = color{0, 0, 0};          // Black
            theme.border_color = color{255, 255, 0}; // Yellow

            // Buttons
            theme.button.fg_normal = color{0, 0, 0};
            theme.button.bg_normal = color{170, 170, 170};  // Light gray
            theme.button.fg_hover = color{255, 255, 255};
            theme.button.bg_hover = color{0, 85, 85};
            theme.button.fg_pressed = color{255, 255, 255};
            theme.button.bg_pressed = color{0, 0, 0};
            theme.button.fg_disabled = color{85, 85, 85};
            theme.button.bg_disabled = color{170, 170, 170};
            theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};
            theme.button.padding_horizontal = 2;
            theme.button.padding_vertical = 0;
            theme.button.text_align = horizontal_alignment::center;

            // Labels
            theme.label.text = color{0, 0, 0};
            theme.label.background = color{0, 170, 170};

            // Panels
            theme.panel.background = color{0, 170, 170};
            theme.panel.border_color = color{0, 0, 0};
            theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};

            // Menu style
            theme.menu.background = color{0, 170, 170};
            theme.menu.border_color = color{0, 0, 0};
            theme.menu.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};

            // Menu bar style
            theme.menu_bar.item_spacing = 2;              // 2px between menu items
            theme.menu_bar.item_padding_horizontal = 0;   // No horizontal padding (left-aligned)
            theme.menu_bar.item_padding_vertical = 0;     // No vertical padding

            // Separator style
            theme.separator.line_style = conio_renderer::line_style{conio_renderer::border_style::single_line};

            return theme;
        }

        /**
         * @brief Create "Midnight Commander" theme
         * @return Theme with MC file manager colors
         */
        static theme_type create_midnight_commander() {
            theme_type theme;
            theme.name = "Midnight Commander";
            theme.description = "MC file manager color scheme";

            // Global palette (dark blue/yellow)
            theme.window_bg = color{0, 0, 85};       // Dark blue
            theme.text_fg = color{255, 255, 85};     // Light yellow
            theme.border_color = color{170, 170, 170}; // Gray

            // Buttons
            theme.button.fg_normal = color{0, 0, 0};
            theme.button.bg_normal = color{0, 170, 170};    // Cyan
            theme.button.fg_hover = color{255, 255, 255};
            theme.button.bg_hover = color{0, 85, 85};
            theme.button.fg_pressed = color{0, 0, 0};
            theme.button.bg_pressed = color{255, 255, 0};
            theme.button.fg_disabled = color{85, 85, 85};
            theme.button.bg_disabled = color{0, 170, 170};
            theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};
            theme.button.padding_horizontal = 2;
            theme.button.padding_vertical = 0;
            theme.button.text_align = horizontal_alignment::center;

            // Labels
            theme.label.text = color{255, 255, 85};
            theme.label.background = color{0, 0, 85};

            // Panels
            theme.panel.background = color{0, 0, 85};
            theme.panel.border_color = color{170, 170, 170};
            theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

            // Menu style
            theme.menu.background = color{0, 0, 85};
            theme.menu.border_color = color{170, 170, 170};
            theme.menu.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

            // Menu bar style
            theme.menu_bar.item_spacing = 2;              // 2px between menu items
            theme.menu_bar.item_padding_horizontal = 0;   // No horizontal padding (left-aligned)
            theme.menu_bar.item_padding_vertical = 0;     // No vertical padding

            // Separator style
            theme.separator.line_style = conio_renderer::line_style{conio_renderer::border_style::single_line};

            return theme;
        }

        /**
         * @brief Create "DOS Edit" theme
         * @return Theme with MS-DOS Edit colors
         */
        static theme_type create_dos_edit() {
            theme_type theme;
            theme.name = "DOS Edit";
            theme.description = "MS-DOS Edit text editor colors";

            // Global palette (light gray/black)
            theme.window_bg = color{170, 170, 170};  // Light gray
            theme.text_fg = color{0, 0, 0};          // Black
            theme.border_color = color{85, 85, 85};  // Dark gray

            // Buttons
            theme.button.fg_normal = color{0, 0, 0};
            theme.button.bg_normal = color{170, 170, 170};
            theme.button.fg_hover = color{255, 255, 255};
            theme.button.bg_hover = color{0, 0, 0};
            theme.button.fg_pressed = color{0, 0, 0};
            theme.button.bg_pressed = color{255, 255, 255};
            theme.button.fg_disabled = color{85, 85, 85};
            theme.button.bg_disabled = color{170, 170, 170};
            theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};
            theme.button.padding_horizontal = 2;
            theme.button.padding_vertical = 0;
            theme.button.text_align = horizontal_alignment::center;

            // Labels
            theme.label.text = color{0, 0, 0};
            theme.label.background = color{170, 170, 170};

            // Panels
            theme.panel.background = color{170, 170, 170};
            theme.panel.border_color = color{0, 0, 0};
            theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

            // Menu style
            theme.menu.background = color{170, 170, 170};
            theme.menu.border_color = color{0, 0, 0};
            theme.menu.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

            // Menu bar style
            theme.menu_bar.item_spacing = 2;              // 2px between menu items
            theme.menu_bar.item_padding_horizontal = 0;   // No horizontal padding (left-aligned)
            theme.menu_bar.item_padding_vertical = 0;     // No vertical padding

            // Separator style
            theme.separator.line_style = conio_renderer::line_style{conio_renderer::border_style::single_line};

            return theme;
        }
    };

} // namespace onyxui::conio


