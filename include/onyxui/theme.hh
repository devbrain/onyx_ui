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
        using font_type = typename Backend::renderer_type::font;

        // Theme metadata
        std::string name;         ///< Theme name (e.g., "Norton Blue", "Dark Mode")
        std::string description;  ///< Optional description (e.g., "Classic Norton Utilities style")

        struct button_style {
            // Visual properties (colors)
            color_type fg_normal;
            color_type bg_normal;
            color_type fg_hover;
            color_type bg_hover;
            color_type fg_pressed;
            color_type bg_pressed;
            color_type fg_disabled;
            color_type bg_disabled;

            // Style references (not rendering resources!)
            box_style_type box_style{};      // "Which box style to use" - Use {} to trigger default member initializers
            font_type font{};                 // "Which font to use for normal text"
            font_type mnemonic_font{};        // "Which font to use for mnemonic character (typically underlined)"

            // Layout preferences
            int padding_horizontal = 4;  // Horizontal padding (left/right) in renderer units
            int padding_vertical = 4;    // Vertical padding (top/bottom) in renderer units
            horizontal_alignment text_align = horizontal_alignment::center;  // Text alignment within button
            int corner_radius = 0;  // Hint for renderer, if supported
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

        // Widget-specific styles
        button_style button{};  // Use {} to trigger default member initializers
        label_style label{};
        panel_style panel{};

        // Global palette
        color_type window_bg;
        color_type text_fg;
        color_type border_color;

    };
}
