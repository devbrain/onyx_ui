//
// Created by igor on 14/10/2025.
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/layout_strategy.hh>  // For horizontal_alignment
#include <cstdint>  // For uint8_t
#include <string>

namespace onyxui {

    template<UIBackend Backend>
    struct ui_theme {
        using color_type = typename Backend::color_type;
        using box_style_type = typename Backend::renderer_type::box_style;
        using line_style_type = typename Backend::renderer_type::line_style;
        using font_type = typename Backend::renderer_type::font;

        // Theme metadata
        std::string name;         ///< Theme name (e.g., "Norton Blue", "Dark Mode")
        std::string description;  ///< Optional description (e.g., "Classic Norton Utilities style")

        /**
         * @brief Complete visual state bundle - reusable across all stateful widgets
         * @details Combines font style + colors for a single visual state
         *
         * @note No helper methods provided - backend types are opaque
         *       Use explicit initialization with designated initializers
         */
        struct visual_state {
            font_type font;           ///< Font style (normal, bold, italic, etc.)
            color_type foreground;    ///< Text/foreground color
            color_type background;    ///< Background color
        };

        /**
         * @brief Button styling - REFACTORED to use visual_state (BREAKING CHANGE)
         * @details Old: fg_normal/bg_normal pattern → New: state bundles
         */
        struct button_style {
            // State bundles (NEW - replaces fg_*/bg_* fields)
            visual_state normal;      ///< Normal state
            visual_state hover;       ///< Mouse hover state
            visual_state pressed;     ///< Mouse pressed state
            visual_state disabled;    ///< Disabled state

            // Button-specific styling
            font_type mnemonic_font{};        ///< Font for mnemonic character (typically underlined)
            box_style_type box_style{};       ///< Box drawing style

            // Layout preferences
            int padding_horizontal = 4;       ///< Horizontal padding (left/right) in renderer units
            int padding_vertical = 4;         ///< Vertical padding (top/bottom) in renderer units
            horizontal_alignment text_align = horizontal_alignment::center;  ///< Text alignment within button
            int corner_radius = 0;            ///< Hint for renderer, if supported
        };

        struct panel_style {
            color_type background;
            color_type border_color;
            box_style_type box_style{};  // Use {} to trigger default member initializers
            bool has_border = true;
        };

        struct label_style {
            color_type text;
            color_type background;
            font_type font{};              // Font for normal text - Use {} to trigger default member initializers
            font_type mnemonic_font{};     // Font for mnemonic character (typically underlined)
        };

        struct menu_style {
            color_type background;
            color_type border_color;
            box_style_type box_style{};  // Use {} to trigger default member initializers
        };

        struct menu_bar_style {
            int item_spacing = 2;           // Spacing between menu items (File, Edit, etc.)
            int item_padding_horizontal = 0; // Horizontal padding inside each menu item (0 = left-aligned)
            int item_padding_vertical = 0;   // Vertical padding inside each menu item
        };

        struct separator_style {
            line_style_type line_style{};   // Line drawing style for separators
        };

        /**
         * @brief Menu item styling with state-based bundles
         */
        struct menu_item_style {
            visual_state normal;       ///< Default state (not focused, not hovered)
            visual_state highlighted;  ///< Keyboard focus OR mouse hover
            visual_state disabled;     ///< Item is disabled

            // Mnemonic-specific styling
            font_type mnemonic_font{};   ///< Font for underlined mnemonic character

            // Shortcut hint styling
            visual_state shortcut;       ///< "Ctrl+S" hint (typically dimmer)

            // Layout
            int padding_horizontal = 8;
            int padding_vertical = 1;

            // NOTE: Submenu indicator rendering is deferred to Phase 4/5
            // Each backend can decide how to render the "▶" indicator
        };

        /**
         * @brief Menu bar item styling
         */
        struct menu_bar_item_style {
            visual_state normal;       ///< Closed, not hovered
            visual_state hover;        ///< Mouse hover (closed)
            visual_state open;         ///< Menu is expanded

            font_type mnemonic_font{};   ///< Underlined mnemonic

            int padding_horizontal = 4;
            int padding_vertical = 0;
        };

        // Widget-specific styles
        button_style button{};            // BREAKING CHANGE - refactored
        label_style label{};              // Unchanged
        panel_style panel{};              // Unchanged
        menu_style menu{};                // Unchanged
        menu_bar_style menu_bar{};        // Unchanged
        separator_style separator{};      // Unchanged

        // NEW: Menu item styles
        menu_item_style menu_item{};
        menu_bar_item_style menu_bar_item{};

        // Global palette
        color_type window_bg;
        color_type text_fg;
        color_type border_color;

    };
}
