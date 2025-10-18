//
// Created by igor on 14/10/2025.
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <cstdint>  // For uint8_t

namespace onyxui {
    // Forward declaration to avoid circular dependency
    enum class horizontal_alignment : std::uint8_t;

    template<UIBackend Backend>
    struct ui_theme {
        using color_type = typename Backend::color_type;
        using box_style_type = typename Backend::renderer_type::box_style;
        using font_type = typename Backend::renderer_type::font;

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
            box_style_type box_style;      // "Which box style to use"
            font_type font;                 // "Which font to use for normal text"
            font_type mnemonic_font;        // "Which font to use for mnemonic character (typically underlined)"

            // Layout preferences
            int padding_horizontal = 4;  // Horizontal padding (left/right) in renderer units
            int padding_vertical = 4;    // Vertical padding (top/bottom) in renderer units
            horizontal_alignment text_align;  // Text alignment within button (no default - set in theme creation)
            int corner_radius = 0;  // Hint for renderer, if supported
        };

        struct panel_style {
            color_type background;
            color_type border_color;
            box_style_type box_style;
            bool has_border = true;
        };

        struct label_style {
            color_type text;
            color_type background;
            font_type font;              // Font for normal text
            font_type mnemonic_font;     // Font for mnemonic character (typically underlined)
        };

        // Widget-specific styles
        button_style button;
        label_style label;
        panel_style panel;

        // Global palette
        color_type window_bg;
        color_type text_fg;
        color_type border_color;

    };
}
