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

#include <onyxui/theming/theme.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/theming/theme_builder.hh>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/colors.hh>
#include <onyxui/conio/conio_renderer.hh>
#include <onyxui/layout/layout_strategy.hh>

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

    private:
        /**
         * @brief Create base TUI theme with semantic colors and common structure
         * @return Base theme with semantic color palette
         *
         * @details
         * This base theme uses semantic color names that derived themes can remap.
         * All structure (padding, alignment, visual states) is defined here once.
         * Derived themes only need to provide color mappings.
         *
         * Semantic color names:
         * - window_bg, text_fg, border_color (globals)
         * - button_bg, button_fg (button normal)
         * - button_hover_bg, button_hover_fg (button hover)
         * - button_pressed_bg, button_pressed_fg (button pressed)
         * - button_disabled_bg, button_disabled_fg (button disabled)
         * - menu_item_shortcut_fg (dimmed shortcut text)
         * - menu_bar_open_bg, menu_bar_open_fg (menu bar when open)
         */
        static theme_type create_base_theme() {
            auto builder = theme_builder<Backend>::create("Base", "Common TUI structure")
                .with_palette({
                    // Global semantic colors
                    {"window_bg", 0x000000},
                    {"text_fg", 0xFFFFFF},
                    {"border_color", 0xAAAAAA},
                    // Button semantic colors
                    {"button_bg", 0x000000},
                    {"button_fg", 0xFFFFFF},
                    {"button_hover_bg", 0x333333},
                    {"button_hover_fg", 0xFFFFFF},
                    {"button_pressed_bg", 0xAAAAAA},
                    {"button_pressed_fg", 0x000000},
                    {"button_disabled_bg", 0x000000},
                    {"button_disabled_fg", 0x555555},
                    // Menu item semantic colors
                    {"menu_item_shortcut_fg", 0x555555},
                    // Menu bar semantic colors
                    {"menu_bar_open_bg", 0xFFFFFF},
                    {"menu_bar_open_fg", 0x000000}
                });

            // ====================================================================
            // Button Widget Configuration
            // ====================================================================
            // Configure button appearance and interactive states
            builder.with_button()
                // Set button border style: single-line box with border drawing enabled
                .box_style(conio_renderer::border_style::single_line, true)

                // Set button padding: 2 pixels horizontal (left/right), 0 vertical (top/bottom)
                .padding(2, 0)

                // Configure NORMAL state (button at rest, not interacting)
                .normal()
                    // Text color in normal state (resolved from semantic palette)
                    .foreground("button_fg")
                    // Background color in normal state (resolved from semantic palette)
                    .background("button_bg")
                    // Font style: not bold, not underlined, not reversed
                    .font(false, false, false)
                    // End state builder, return to button_builder context
                    .end<theme_builder<Backend>::button_builder>()

                // Configure HOVER state (mouse over or keyboard focus)
                .hover()
                    // Text color when hovering (typically highlighted)
                    .foreground("button_hover_fg")
                    // Background color when hovering (typically different from normal)
                    .background("button_hover_bg")
                    // Font style: bold to emphasize, not underlined, not reversed
                    .font(true, false, false)
                    // End state builder, return to button_builder context
                    .end<theme_builder<Backend>::button_builder>()

                // Configure PRESSED state (mouse down or key held)
                .pressed()
                    // Text color when pressed (typically inverted or distinct)
                    .foreground("button_pressed_fg")
                    // Background color when pressed (typically high contrast)
                    .background("button_pressed_bg")
                    // Font style: bold to show active press, not underlined, not reversed
                    .font(true, false, false)
                    // End state builder, return to button_builder context
                    .end<theme_builder<Backend>::button_builder>()

                // Configure DISABLED state (button inactive, non-interactive)
                .disabled()
                    // Text color when disabled (typically muted/grayed)
                    .foreground("button_disabled_fg")
                    // Background color when disabled (same as normal in most themes)
                    .background("button_disabled_bg")
                    // Font style: not bold (de-emphasize), not underlined, not reversed
                    .font(false, false, false)
                    // End state builder, return to button_builder context
                    .end<theme_builder<Backend>::button_builder>();

            // ====================================================================
            // Panel Widget Configuration
            // ====================================================================
            // Configure panel container appearance
            builder.with_panel()
                // Set panel border style: single-line box with border drawing enabled
                .box_style(conio_renderer::border_style::single_line, true);

            // ====================================================================
            // Menu Widget Configuration
            // ====================================================================
            // Configure dropdown menu appearance
            builder.with_menu()
                // Set menu border style: single-line box with border drawing enabled
                .box_style(conio_renderer::border_style::single_line, true);

            // ====================================================================
            // Menu Bar Configuration
            // ====================================================================
            // Configure top-level menu bar layout
            builder.with_menu_bar()
                // Horizontal spacing between menu bar items (e.g., "File  Edit  View")
                .item_spacing(2)
                // Padding around each menu bar item: 0 horizontal, 0 vertical
                .item_padding(0, 0);

            // ====================================================================
            // Menu Item Configuration
            // ====================================================================
            // Configure individual menu entries within dropdown menus
            builder.with_menu_item()
                // Padding around menu item text: 8 pixels horizontal (for shortcut alignment), 1 vertical
                .padding(8, 1)

                // Configure NORMAL state (menu item not selected)
                .normal()
                    // Text color in normal state (inherits button fg for consistency)
                    .foreground("button_fg")
                    // Background color in normal state (inherits button bg)
                    .background("button_bg")
                    // Font style: not bold, not underlined, not reversed
                    .font(false, false, false)
                    // End state builder, return to menu_item_builder context
                    .end<theme_builder<Backend>::menu_item_builder>()

                // Configure HIGHLIGHTED state (keyboard/mouse selection)
                .highlighted()
                    // Text color when highlighted (same as button hover)
                    .foreground("button_hover_fg")
                    // Background color when highlighted (same as button hover)
                    .background("button_hover_bg")
                    // Font style: bold to emphasize selection, not underlined, not reversed
                    .font(true, false, false)
                    // End state builder, return to menu_item_builder context
                    .end<theme_builder<Backend>::menu_item_builder>()

                // Configure DISABLED state (menu item inactive, grayed out)
                .disabled()
                    // Text color when disabled (muted/grayed)
                    .foreground("button_disabled_fg")
                    // Background color when disabled (same as normal)
                    .background("button_disabled_bg")
                    // Font style: not bold (de-emphasize), not underlined, not reversed
                    .font(false, false, false)
                    // End state builder, return to menu_item_builder context
                    .end<theme_builder<Backend>::menu_item_builder>()

                // Configure SHORTCUT state (keyboard shortcut text appearance)
                .shortcut()
                    // Text color for shortcuts (typically muted to differentiate from main text)
                    .foreground("menu_item_shortcut_fg")
                    // Background color for shortcuts (same as menu background)
                    .background("button_bg")
                    // Font style: not bold, not underlined, not reversed
                    .font(false, false, false)
                    // End state builder, return to menu_item_builder context
                    .end<theme_builder<Backend>::menu_item_builder>();

            // Build the theme from the builder configuration
            auto theme = builder.build();

            // Set submenu indicator icon (arrow_right for all themes)
            theme.menu_item.submenu_icon = conio_renderer::icon_style::folder;

            // ====================================================================
            // Menu Bar Item Manual Configuration
            // ====================================================================
            // Menu bar items don't have a builder API yet, configure manually

            // NORMAL state: top-level menu title at rest (e.g., "File", "Edit")
            theme.menu_bar_item.normal = {
                // Font: regular (not bold), not underlined, not reversed
                .font = conio_renderer::font{.bold = false, .underline = false, .reverse = false},
                // Text color: same as button normal state
                .foreground = builder.resolve_color("button_fg"),
                // Background color: same as button normal state
                .background = builder.resolve_color("button_bg"),
                // Mnemonic color: same as foreground (will be customized per-theme)
                .mnemonic_foreground = builder.resolve_color("button_fg")
            };

            // HOVER state: mouse hovering over menu bar item or keyboard navigation
            theme.menu_bar_item.hover = {
                // Font: bold to emphasize, not underlined, not reversed
                .font = conio_renderer::font{.bold = true, .underline = false, .reverse = false},
                // Text color: same as button hover state (highlighted)
                .foreground = builder.resolve_color("button_hover_fg"),
                // Background color: same as button hover state (highlighted)
                .background = builder.resolve_color("button_hover_bg"),
                // Mnemonic color: same as foreground (will be customized per-theme)
                .mnemonic_foreground = builder.resolve_color("button_hover_fg")
            };

            // OPEN state: menu bar item with its dropdown menu currently displayed
            theme.menu_bar_item.open = {
                // Font: bold to show active state, not underlined, not reversed
                .font = conio_renderer::font{.bold = true, .underline = false, .reverse = false},
                // Text color: distinct color for open state (often inverted or bright)
                .foreground = builder.resolve_color("menu_bar_open_fg"),
                // Background color: distinct color for open state (often inverted or bright)
                .background = builder.resolve_color("menu_bar_open_bg"),
                // Mnemonic color: same as foreground (will be customized per-theme)
                .mnemonic_foreground = builder.resolve_color("menu_bar_open_fg")
            };

            // Mnemonic font: appearance of mnemonic characters (e.g., underlined 'F' in "File")
            // Font: not bold, underlined to indicate keyboard shortcut, not reversed
            theme.menu_bar_item.mnemonic_font = conio_renderer::font{.bold = false, .underline = true, .reverse = false};

            // Horizontal padding around menu bar item text (4 pixels on each side)
            theme.menu_bar_item.padding_horizontal = 4;

            // Vertical padding around menu bar item text (0 pixels, flush with top/bottom)
            theme.menu_bar_item.padding_vertical = 0;

            // Return the fully configured base theme
            return theme;
        }

    public:

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
            registry.register_theme(create_nu8());
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
         *
         * This theme extends the base and only remaps colors.
         */
        static theme_type create_norton_blue() {
            // Start from base theme, then override with Norton Blue colors
            auto theme = create_base_theme();

            theme.name = "Norton Blue";
            theme.description = "Classic Norton Utilities color scheme";

            // Global colors
            theme.window_bg = color{0, 0, 170};      // Dark blue
            theme.text_fg = color{255, 255, 255};    // White
            theme.border_color = color{255, 255, 0}; // Yellow

            // Button colors (remap semantic palette)
            color button_bg = color{0, 0, 170};      // Dark blue
            color button_fg = color{255, 255, 255};  // White
            color button_hover_bg = color{0, 0, 255};      // Bright blue
            color button_hover_fg = color{255, 255, 0};    // Yellow
            color button_pressed_bg = color{170, 170, 170}; // Light gray
            color button_pressed_fg = color{0, 0, 0};       // Black
            color button_disabled_bg = color{0, 0, 170};    // Dark blue
            color button_disabled_fg = color{85, 85, 85};   // Dark gray
            color menu_item_shortcut_fg = color{170, 170, 170}; // Gray
            color menu_bar_open_bg = color{0, 255, 255};    // Cyan
            color menu_bar_open_fg = color{0, 0, 0};        // Black

            // Apply button colors
            theme.button.normal.background = button_bg;
            theme.button.normal.foreground = button_fg;
            theme.button.hover.background = button_hover_bg;
            theme.button.hover.foreground = button_hover_fg;
            theme.button.pressed.background = button_pressed_bg;
            theme.button.pressed.foreground = button_pressed_fg;
            theme.button.disabled.background = button_disabled_bg;
            theme.button.disabled.foreground = button_disabled_fg;

            // Apply label colors
            theme.label.text = button_fg;
            theme.label.background = button_bg;

            // Apply panel colors
            theme.panel.background = button_bg;
            theme.panel.border_color = theme.border_color;

            // Apply menu colors
            theme.menu.background = button_bg;
            theme.menu.border_color = theme.border_color;

            // Apply menu item colors
            theme.menu_item.normal.background = button_bg;
            theme.menu_item.normal.foreground = button_fg;
            theme.menu_item.highlighted.background = button_hover_bg;
            theme.menu_item.highlighted.foreground = button_hover_fg;
            theme.menu_item.disabled.background = button_disabled_bg;
            theme.menu_item.disabled.foreground = button_disabled_fg;
            theme.menu_item.shortcut.background = button_bg;
            theme.menu_item.shortcut.foreground = menu_item_shortcut_fg;

            // Apply menu bar item colors
            theme.menu_bar_item.normal.background = button_bg;
            theme.menu_bar_item.normal.foreground = button_fg;
            theme.menu_bar_item.hover.background = button_hover_bg;
            theme.menu_bar_item.hover.foreground = button_hover_fg;
            theme.menu_bar_item.open.background = menu_bar_open_bg;
            theme.menu_bar_item.open.foreground = menu_bar_open_fg;

            return theme;
        }

        /**
         * @brief Create "Borland Turbo" theme
         * @return Theme with Turbo Pascal/C++ IDE colors
         */
        static theme_type create_borland_turbo() {
            auto theme = create_base_theme();

            theme.name = "Borland Turbo";
            theme.description = "Turbo Pascal/C++ IDE color scheme";

            // Override box styles to double_line
            theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};
            theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};
            theme.menu.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};

            // Global colors
            theme.window_bg = color{0, 170, 170};    // Cyan
            theme.text_fg = color{0, 0, 0};          // Black
            theme.border_color = color{255, 255, 0}; // Yellow

            // Button colors
            color button_bg = color{170, 170, 170};  // Light gray
            color button_fg = color{0, 0, 0};        // Black
            color button_hover_bg = color{0, 85, 85};      // Dark cyan
            color button_hover_fg = color{255, 255, 255};  // White
            color button_pressed_bg = color{0, 0, 0};      // Black
            color button_pressed_fg = color{255, 255, 255}; // White
            color button_disabled_bg = color{170, 170, 170}; // Light gray
            color button_disabled_fg = color{85, 85, 85};    // Dark gray
            color menu_item_shortcut_fg = color{85, 85, 85}; // Dark gray
            color menu_bar_open_bg = color{0, 0, 0};         // Black
            color menu_bar_open_fg = color{255, 255, 255};   // White

            // Apply colors (button uses light_gray instead of window_bg)
            theme.button.normal.background = button_bg;
            theme.button.normal.foreground = button_fg;
            theme.button.hover.background = button_hover_bg;
            theme.button.hover.foreground = button_hover_fg;
            theme.button.pressed.background = button_pressed_bg;
            theme.button.pressed.foreground = button_pressed_fg;
            theme.button.disabled.background = button_disabled_bg;
            theme.button.disabled.foreground = button_disabled_fg;

            theme.label.text = theme.text_fg;
            theme.label.background = theme.window_bg;

            theme.panel.background = theme.window_bg;
            theme.panel.border_color = theme.border_color;

            theme.menu.background = theme.window_bg;
            theme.menu.border_color = theme.border_color;

            theme.menu_item.normal.background = theme.window_bg;
            theme.menu_item.normal.foreground = theme.text_fg;
            theme.menu_item.highlighted.background = button_hover_bg;
            theme.menu_item.highlighted.foreground = button_hover_fg;
            theme.menu_item.disabled.background = theme.window_bg;
            theme.menu_item.disabled.foreground = button_disabled_fg;
            theme.menu_item.shortcut.background = theme.window_bg;
            theme.menu_item.shortcut.foreground = menu_item_shortcut_fg;

            theme.menu_bar_item.normal.background = theme.window_bg;
            theme.menu_bar_item.normal.foreground = theme.text_fg;
            theme.menu_bar_item.hover.background = button_hover_bg;
            theme.menu_bar_item.hover.foreground = button_hover_fg;
            theme.menu_bar_item.open.background = menu_bar_open_bg;
            theme.menu_bar_item.open.foreground = menu_bar_open_fg;

            return theme;
        }

        /**
         * @brief Create "Midnight Commander" theme
         * @return Theme with MC file manager colors
         */
        static theme_type create_midnight_commander() {
            auto theme = create_base_theme();

            theme.name = "Midnight Commander";
            theme.description = "MC file manager color scheme";

            // Global colors
            theme.window_bg = color{0, 0, 85};       // Dark blue
            theme.text_fg = color{255, 255, 85};     // Light yellow
            theme.border_color = color{170, 170, 170}; // Gray

            // Button colors (MC uses cyan buttons)
            color button_bg = color{0, 170, 170};    // Cyan
            color button_fg = color{0, 0, 0};        // Black
            color button_hover_bg = color{0, 85, 85};      // Dark cyan
            color button_hover_fg = color{255, 255, 255};  // White
            color button_pressed_bg = color{255, 255, 0};  // Yellow
            color button_pressed_fg = color{0, 0, 0};      // Black
            color button_disabled_bg = color{0, 170, 170}; // Cyan
            color button_disabled_fg = color{85, 85, 85};  // Dark gray
            color menu_item_shortcut_fg = color{170, 170, 170}; // Gray
            color menu_bar_open_bg = color{255, 255, 0};   // Yellow
            color menu_bar_open_fg = color{0, 0, 0};       // Black

            // Apply colors
            theme.button.normal.background = button_bg;
            theme.button.normal.foreground = button_fg;
            theme.button.hover.background = button_hover_bg;
            theme.button.hover.foreground = button_hover_fg;
            theme.button.pressed.background = button_pressed_bg;
            theme.button.pressed.foreground = button_pressed_fg;
            theme.button.disabled.background = button_disabled_bg;
            theme.button.disabled.foreground = button_disabled_fg;

            theme.label.text = theme.text_fg;
            theme.label.background = theme.window_bg;

            theme.panel.background = theme.window_bg;
            theme.panel.border_color = theme.border_color;

            theme.menu.background = theme.window_bg;
            theme.menu.border_color = theme.border_color;

            // Menu items use window colors (not button colors)
            theme.menu_item.normal.background = theme.window_bg;
            theme.menu_item.normal.foreground = theme.text_fg;
            theme.menu_item.highlighted.background = color{0, 0, 170};   // Brighter blue
            theme.menu_item.highlighted.foreground = color{255, 255, 0}; // Bright yellow
            theme.menu_item.disabled.background = theme.window_bg;
            theme.menu_item.disabled.foreground = button_disabled_fg;
            theme.menu_item.shortcut.background = theme.window_bg;
            theme.menu_item.shortcut.foreground = menu_item_shortcut_fg;

            theme.menu_bar_item.normal.background = theme.window_bg;
            theme.menu_bar_item.normal.foreground = theme.text_fg;
            theme.menu_bar_item.hover.background = color{0, 0, 170};   // Brighter blue
            theme.menu_bar_item.hover.foreground = color{255, 255, 0}; // Bright yellow
            theme.menu_bar_item.open.background = menu_bar_open_bg;
            theme.menu_bar_item.open.foreground = menu_bar_open_fg;

            return theme;
        }

        /**
         * @brief Create "DOS Edit" theme
         * @return Theme with MS-DOS Edit colors
         */
        static theme_type create_dos_edit() {
            auto theme = create_base_theme();

            theme.name = "DOS Edit";
            theme.description = "MS-DOS Edit text editor colors";



            // Global colors
            theme.window_bg = color{170, 170, 170};  // Light gray
            theme.text_fg = color{0, 0, 0};          // Black
            theme.border_color = color{85, 85, 85};  // Dark gray

            // Button colors
            color button_bg = color{170, 170, 170};  // Light gray
            color button_fg = color{0, 0, 0};        // Black
            color button_hover_bg = color{0, 0, 0};        // Black
            color button_hover_fg = color{255, 255, 255};  // White
            color button_pressed_bg = color{255, 255, 255}; // White
            color button_pressed_fg = color{0, 0, 0};       // Black
            color button_disabled_bg = color{170, 170, 170}; // Light gray
            color button_disabled_fg = color{85, 85, 85};    // Dark gray
            color menu_item_shortcut_fg = color{85, 85, 85}; // Dark gray
            color menu_bar_open_bg = color{255, 255, 255};   // White
            color menu_bar_open_fg = color{0, 0, 0};         // Black

            // Apply colors
            theme.button.normal.background = button_bg;
            theme.button.normal.foreground = button_fg;
            theme.button.hover.background = button_hover_bg;
            theme.button.hover.foreground = button_hover_fg;
            theme.button.pressed.background = button_pressed_bg;
            theme.button.pressed.foreground = button_pressed_fg;
            theme.button.disabled.background = button_disabled_bg;
            theme.button.disabled.foreground = button_disabled_fg;

            theme.label.text = theme.text_fg;
            theme.label.background = theme.window_bg;

            theme.panel.background = theme.window_bg;
            theme.panel.border_color = theme.border_color;

            theme.menu.background = theme.window_bg;
            theme.menu.border_color = theme.border_color;

            theme.menu_item.normal.background = theme.window_bg;
            theme.menu_item.normal.foreground = theme.text_fg;
            theme.menu_item.highlighted.background = button_hover_bg;
            theme.menu_item.highlighted.foreground = button_hover_fg;
            theme.menu_item.disabled.background = theme.window_bg;
            theme.menu_item.disabled.foreground = button_disabled_fg;
            theme.menu_item.shortcut.background = theme.window_bg;
            theme.menu_item.shortcut.foreground = menu_item_shortcut_fg;

            theme.menu_bar_item.normal.background = theme.window_bg;
            theme.menu_bar_item.normal.foreground = theme.text_fg;
            theme.menu_bar_item.hover.background = button_hover_bg;
            theme.menu_bar_item.hover.foreground = button_hover_fg;
            theme.menu_bar_item.open.background = menu_bar_open_bg;
            theme.menu_bar_item.open.foreground = menu_bar_open_fg;

            return theme;
        }

            /**
         * @brief Create "DOS Edit" theme
         * @return Theme with MS-DOS Edit colors
         */
        static theme_type create_nu8() {
            auto theme = create_base_theme();

            theme.name = "NU8";
            theme.description = "Norton Utilities 8";

            // Define color palette
            constexpr color white{255, 255, 255};
            constexpr color black{32, 32, 32};  // Dark grey instead of pure black (0,0,0) - avoids terminal contrast adjustments
            constexpr color light_gray{170, 170, 170};
            constexpr color dark_gray{85, 85, 85};
            constexpr color red{255, 85, 85};  // Bright red for mnemonics (Norton Utilities 8 style)

            // Global colors
            theme.window_bg = color{80, 87, 255};
            theme.text_fg = white;
            theme.border_color = white;

            // Button colors - Norton Utilities 8 style
            // Normal (inactive): gray bg, black fg, red mnemonics
            // Active/Hover: white bg, black fg, red mnemonics
            // Disabled: grey bg, dark grey fg, dark grey mnemonics
            color button_bg = light_gray;
            color button_hover_bg = white;
            color button_pressed_bg = white;
            color button_mnemonic_fg = red;  // Bright red for mnemonics (Norton Utilities 8 style)


            // Apply button styling
            theme.button.box_style.style = conio::conio_renderer::border_style::none;  // No border
            theme.button.padding_horizontal = 2;  // At least 2 spaces on each side
            theme.button.padding_vertical = 0;    // Compact vertical padding

            theme.button.normal.background = button_bg;
            theme.button.normal.foreground = black;
            theme.button.normal.mnemonic_foreground = button_mnemonic_fg;
            theme.button.hover.background = button_hover_bg;
            theme.button.hover.foreground = black;
            theme.button.hover.mnemonic_foreground = button_mnemonic_fg;
            theme.button.pressed.background = button_pressed_bg;
            theme.button.pressed.foreground = black;
            theme.button.pressed.mnemonic_foreground = button_mnemonic_fg;
            theme.button.disabled.background = light_gray;
            theme.button.disabled.foreground = dark_gray;
            theme.button.disabled.mnemonic_foreground = dark_gray;  // Dimmed when disabled

            theme.label.text = theme.text_fg;
            theme.label.background = theme.window_bg;

            theme.panel.background = theme.window_bg;
            theme.panel.border_color = theme.border_color;

            // Menu dropdown - white background with black border (Norton Utilities 8 style)
            theme.menu.background = white;
            theme.menu.border_color = black;  // Black box drawing characters

            // Enable drop shadows for classic DOS look
            theme.menu.shadow.enabled = true;
            theme.menu.shadow.offset_x = 1;
            theme.menu.shadow.offset_y = 1;

            // Separator lines - black on white background
            theme.separator.foreground = black;

            // Menu item styling - white bg, black fg in normal state
            theme.menu_item.normal.background = white;
            theme.menu_item.normal.foreground = black;
            theme.menu_item.normal.mnemonic_foreground = button_mnemonic_fg;  // Red mnemonics (NU8 style)

            // Menu item highlighted - black bg, white fg
            theme.menu_item.highlighted.background = black;
            theme.menu_item.highlighted.foreground = white;
            theme.menu_item.highlighted.mnemonic_foreground = button_mnemonic_fg;  // Red mnemonics (NU8 style)

            // Disabled items
            theme.menu_item.disabled.background = white;
            theme.menu_item.disabled.foreground = dark_gray;
            theme.menu_item.disabled.mnemonic_foreground = dark_gray;  // Dimmed when disabled

            // Keyboard shortcut text (e.g., "Ctrl+S") - appears on right side of menu items
            theme.menu_item.shortcut.background = white;
            theme.menu_item.shortcut.foreground = red;  // Bright red to match mnemonics (NU8 style)
            theme.menu_item.shortcut.mnemonic_foreground = red;  // Not used for shortcuts (no mnemonics in shortcuts)

            // Menu bar - continuous white stripe with black text (Norton Utilities 8 style)
            theme.menu_bar.item_spacing = 1;  // Remove gaps to create continuous stripe
            theme.menu_bar_item.normal.background = white;
            theme.menu_bar_item.normal.foreground = black;
            theme.menu_bar_item.normal.mnemonic_foreground = button_mnemonic_fg;  // Red mnemonics (NU8 style)
            theme.menu_bar_item.hover.background = black;
            theme.menu_bar_item.hover.foreground = white;
            theme.menu_bar_item.hover.mnemonic_foreground = button_mnemonic_fg;  // Red mnemonics (NU8 style)
            theme.menu_bar_item.open.background = black;
            theme.menu_bar_item.open.foreground = white;
            theme.menu_bar_item.open.mnemonic_foreground = button_mnemonic_fg;  // Red mnemonics (NU8 style)

            return theme;
        }
    };

} // namespace onyxui::conio


