//
// Created by igor on 14/10/2025.
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <cstdint>  // For uint8_t
#include <string>

namespace onyxui {

    /**
     * @enum scrollbar_style
     * @brief Visual style for scrollbar rendering
     *
     * @details
     * Defines the overall appearance of scrollbars including arrow button presence.
     */
    enum class scrollbar_style : std::uint8_t {
        minimal,      ///< No arrow buttons, just track + thumb
        classic,      ///< Arrow buttons at both ends (Windows style)
        compact       ///< Arrow buttons at one end only
    };

    /**
     * @brief Convert scrollbar_style to string
     * @param style The scrollbar_style enum value
     * @return String representation of the style
     */
    inline constexpr std::string_view scrollbar_style_to_string(scrollbar_style style) noexcept {
        switch (style) {
            case scrollbar_style::minimal: return "minimal";
            case scrollbar_style::classic: return "classic";
            case scrollbar_style::compact: return "compact";
        }
        return "minimal";  // Unreachable, but satisfies compiler
    }

    /**
     * @brief Convert string to scrollbar_style
     * @param str The string representation
     * @return The scrollbar_style enum value
     * @throws std::runtime_error if string doesn't match any value
     */
    inline scrollbar_style scrollbar_style_from_string(std::string_view str) {
        if (str == "minimal") return scrollbar_style::minimal;
        if (str == "classic") return scrollbar_style::classic;
        if (str == "compact") return scrollbar_style::compact;
        throw std::runtime_error(std::string("Invalid scrollbar_style: ") + std::string(str));
    }

    template<UIBackend Backend>
    struct ui_theme {
        using color_type = typename Backend::color_type;
        using box_style_type = typename Backend::renderer_type::box_style;
        using line_style_type = typename Backend::renderer_type::line_style;
        using font_type = typename Backend::renderer_type::font;
        using icon_style_type = typename Backend::renderer_type::icon_style;

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
            font_type font;                  ///< Font style (normal, bold, italic, etc.)
            color_type foreground;           ///< Text/foreground color
            color_type background;           ///< Background color
            color_type mnemonic_foreground;  ///< Mnemonic character color (defaults to foreground if not set)
        };

        /**
         * @brief Shadow configuration for popup elements and buttons
         * @details
         * Defines whether shadows are enabled and their offset.
         * Backend determines rendering method (e.g., darkening, shading characters).
         * Used by menus, dialogs, tooltips, and buttons.
         */
        struct shadow_config {
            bool enabled = false;      ///< Enable/disable shadow rendering
            int offset_x = 1;          ///< Horizontal shadow offset (pixels/cells to the right)
            int offset_y = 1;          ///< Vertical shadow offset (pixels/cells down)
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
            shadow_config shadow;             ///< Shadow configuration for button depth effect

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

        struct line_edit_style {
            color_type text;                // Text color
            color_type background;          // Background color
            color_type border_color;        // Border color
            color_type placeholder_text;    // Placeholder text color
            color_type cursor;              // Cursor color
            box_style_type box_style{};     // Border style
            font_type font{};               // Font for text
            int padding_horizontal = 1;     // Horizontal padding (left/right)
            int padding_vertical = 0;       // Vertical padding (top/bottom)

            // Cursor icons (backend-specific)
            icon_style_type cursor_insert_icon{};    // Cursor icon for insert mode (e.g., vertical bar │)
            icon_style_type cursor_overwrite_icon{}; // Cursor icon for overwrite mode (e.g., block █)

            // Cursor animation
            int cursor_blink_interval_ms = 500;  // Cursor blink interval in milliseconds (500ms = 0.5s on/off)
        };

        struct menu_style {
            color_type background;
            color_type border_color;
            box_style_type box_style{};  // Use {} to trigger default member initializers
            shadow_config shadow;        // Shadow configuration for dropdown menus
        };

        struct menu_bar_style {
            int item_spacing = 2;           // Spacing between menu items (File, Edit, etc.)
            int item_padding_horizontal = 0; // Horizontal padding inside each menu item (0 = left-aligned)
            int item_padding_vertical = 0;   // Vertical padding inside each menu item
        };

        struct separator_style {
            color_type foreground;          // Color for separator line
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

            // Submenu indicator icon (backend-specific)
            // Backends should use appropriate icons (arrow_right, triangle, etc.)
            icon_style_type submenu_icon{};  ///< Icon shown for items with submenus
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

        /**
         * @brief Checkbox styling - Phase 2
         * @details Defines the visual appearance of checkboxes in different states
         */
        struct checkbox_style {
            // Visual states
            visual_state normal;        ///< Normal state (unchecked)
            visual_state hover;         ///< Mouse hover state
            visual_state checked;       ///< Checked state
            visual_state disabled;      ///< Disabled state

            // Checkbox-specific styling
            font_type mnemonic_font{};  ///< Font for mnemonic character (typically underlined)

            // Checkbox box icons (backend-specific)
            // These represent the checkbox box itself ([ ], [X], [-])
            icon_style_type unchecked_icon{};      ///< Icon for unchecked box (e.g., [ ])
            icon_style_type checked_icon{};        ///< Icon for checked box (e.g., [X])
            icon_style_type indeterminate_icon{};  ///< Icon for indeterminate box (e.g., [-])

            // Layout
            int spacing = 1;  ///< Space between box and label text (renderer units)
        };

        /**
         * @brief Radio button styling - Phase 2
         * @details Defines the visual appearance of radio buttons in different states
         */
        struct radio_button_style {
            // Visual states
            visual_state normal;        ///< Normal state (unchecked)
            visual_state hover;         ///< Mouse hover state
            visual_state checked;       ///< Checked state
            visual_state disabled;      ///< Disabled state

            // Radio button-specific styling
            font_type mnemonic_font{};  ///< Font for mnemonic character (typically underlined)

            // Radio button icons (backend-specific)
            // These represent the radio button itself (( ), (*))
            icon_style_type unchecked_icon{};  ///< Icon for unchecked button (e.g., ( ))
            icon_style_type checked_icon{};    ///< Icon for checked button (e.g., (*))

            // Layout
            int spacing = 1;  ///< Space between icon and label text (renderer units)
        };

        /**
         * @brief Progress bar styling
         * @details Visual appearance for progress indicators (determinate and indeterminate modes)
         */
        struct progress_bar_style {
            color_type filled_color;     ///< Color for filled/progress portion
            color_type empty_color;      ///< Color for empty/remaining portion
            color_type text_color;       ///< Color for text overlay
            font_type text_font;         ///< Font for text overlay
            icon_style_type filled_icon{};  ///< Icon for filled portion (default: progress_filled)
            icon_style_type empty_icon{};   ///< Icon for empty portion (default: progress_empty)
        };

        /**
         * @brief Slider styling
         * @details Visual appearance for interactive slider controls
         */
        struct slider_style {
            color_type track_filled_color;   ///< Color for filled portion of track
            color_type track_empty_color;    ///< Color for empty portion of track
            color_type thumb_color;          ///< Color for thumb/handle
            color_type tick_color;           ///< Color for tick marks
            icon_style_type filled_icon{};   ///< Icon for filled track portion (default: slider_filled)
            icon_style_type empty_icon{};    ///< Icon for empty track portion (default: slider_empty)
            icon_style_type thumb_icon{};    ///< Icon for slider thumb (default: slider_thumb)
        };

        /**
         * @brief Tab widget styling
         * @details Visual appearance for tabbed containers
         */
        struct tab_widget_style {
            // Tab bar background
            color_type tab_bar_background;

            // Tab button colors
            color_type tab_normal_background;      ///< Inactive tab background
            color_type tab_normal_text;            ///< Inactive tab text
            color_type tab_active_background;      ///< Active tab background
            color_type tab_active_text;            ///< Active tab text
            color_type tab_hover_background;       ///< Hovered tab background
            color_type tab_hover_text;             ///< Hovered tab text

            // Tab borders
            color_type tab_border;                 ///< Tab border color
            color_type tab_active_border;          ///< Active tab border color

            // Close button
            color_type close_button_normal;        ///< Close button (X) color
            color_type close_button_hover;         ///< Close button hover color
            icon_style_type close_button_icon{};   ///< Close button icon (e.g., [X])

            // Fonts
            font_type tab_font;                    ///< Tab label font
            font_type tab_active_font;             ///< Active tab label font (may be bold)

            // Scroll arrows (for tab overflow - Phase 2)
            color_type scroll_arrow_normal;        ///< Scroll arrow normal color
            color_type scroll_arrow_hover;         ///< Scroll arrow hover color
            color_type scroll_arrow_pressed;       ///< Scroll arrow pressed color
            color_type scroll_arrow_disabled;      ///< Scroll arrow disabled color
            icon_style_type scroll_left_icon{};    ///< Left scroll arrow icon
            icon_style_type scroll_right_icon{};   ///< Right scroll arrow icon

            // Spacing
            int tab_padding_horizontal = 4;       ///< Horizontal padding inside tab
            int tab_padding_vertical = 1;         ///< Vertical padding inside tab
            int tab_spacing = 1;                  ///< Space between tabs
            int close_button_spacing = 2;         ///< Space between label and close button
            int min_tab_width = 10;               ///< Minimum tab width in characters
            int scroll_arrow_width = 3;           ///< Width of scroll arrow buttons
        };

        /**
         * @brief Scrollbar theme - PLACEHOLDER for Phase 3
         * @details Will be fully implemented in Phase 3 of scrolling system
         */
        struct scrollbar_theme {
            /**
             * @brief Component-specific styling (track, thumb, arrows)
             */
            struct component_style {
                color_type background;
                color_type foreground;
                box_style_type box_style;
            };

            // Component states
            component_style track_normal;
            component_style thumb_normal;
            component_style thumb_hover;
            component_style thumb_pressed;
            component_style thumb_disabled;
            component_style arrow_normal;
            component_style arrow_hover;
            component_style arrow_pressed;

            // Geometry
            int width = 16;                     ///< Width for vertical (swapped for horizontal)
            int min_thumb_size = 20;            ///< Minimum thumb size in pixels
            int line_increment = 20;            ///< Scroll amount per arrow click (pixels)
            int arrow_size = 1;                 ///< Size of arrow buttons (1 for text UI, can be larger for graphical UI)
            int min_render_size = 8;            ///< Minimum size to render without visual corruption (borders + content)

            // Visual style
            scrollbar_style style = scrollbar_style::classic;

            // Arrow glyphs (backend-specific icon styles)
            // Note: icon_style_type will be initialized to backend's default (typically icon_style::none)
            // Themes should set these to appropriate arrow icons (arrow_up/down/left/right)
            icon_style_type arrow_up_icon{};         ///< Icon for up arrow (vertical scrollbar decrement)
            icon_style_type arrow_down_icon{};       ///< Icon for down arrow (vertical scrollbar increment)
            icon_style_type arrow_left_icon{};       ///< Icon for left arrow (horizontal scrollbar decrement)
            icon_style_type arrow_right_icon{};      ///< Icon for right arrow (horizontal scrollbar increment)

            // Deprecated: Use specific direction icons above
            [[deprecated("Use arrow_up_icon/arrow_down_icon instead")]]
            icon_style_type arrow_decrement_icon{};  ///< Icon for decrement arrow (up for vertical, left for horizontal)
            [[deprecated("Use arrow_left_icon/arrow_right_icon instead")]]
            icon_style_type arrow_increment_icon{};  ///< Icon for increment arrow (down for vertical, right for horizontal)

            // Animation settings (Phase 2 - auto_hide_inactive policy)
            int fade_duration_ms = 150;         ///< Fade in/out duration
            int inactive_delay_ms = 500;        ///< Delay before hiding after scroll ends
        };

        /**
         * @brief Window styling - Phase 8
         * @details Defines the visual appearance of windows, including title bar,
         *          borders, and focus indicators.
         */
        struct window_style {
            // Title bar styling
            visual_state title_focused;      ///< Title bar when window has focus
            visual_state title_unfocused;    ///< Title bar when window doesn't have focus

            // Border styling
            box_style_type border_focused{};    ///< Border when window has focus
            box_style_type border_unfocused{};  ///< Border when window doesn't have focus
            color_type border_color_focused;    ///< Border color when focused
            color_type border_color_unfocused;  ///< Border color when unfocused

            // Content area
            color_type content_background;   ///< Background color for content area

            // Window shadow (for modal/floating windows)
            shadow_config shadow;

            // Layout
            int title_bar_height = 1;        ///< Height of title bar (renderer units)
            int border_width = 1;            ///< Width of window border
            horizontal_alignment title_alignment = horizontal_alignment::left; ///< Title text alignment (left/center/right)

            // Title bar button icons (backend-specific)
            // Note: icon_style_type will be initialized to backend's default
            // Themes should set these to appropriate window management icons
            icon_style_type menu_icon{};     ///< Icon for menu/system menu button (≡)
            icon_style_type minimize_icon{}; ///< Icon for minimize button (▁)
            icon_style_type maximize_icon{}; ///< Icon for maximize button (□)
            icon_style_type restore_icon{};  ///< Icon for restore button (▢)
            icon_style_type close_icon{};    ///< Icon for close button (×)
        };

        /**
         * @brief Backend-specific spacing resolution
         * @details
         * Maps semantic spacing enum values to backend-specific integers.
         * - TUI (conio): Values are character cells
         * - GUI (SDL2): Values are pixels
         *
         * This allows the same application code to produce visually consistent
         * layouts across different backends.
         *
         * ## Recommended Values by Backend
         *
         * ### TUI (conio) - Character Cells
         * ```yaml
         * spacing_values:
         *   none: 0
         *   tiny: 0      # No spacing in TUI (too small)
         *   small: 1     # Single character
         *   medium: 1    # Single character (default)
         *   large: 2     # Double character
         *   xlarge: 3    # Triple character
         * ```
         *
         * ### GUI (SDL2) - Pixels
         * ```yaml
         * spacing_values:
         *   none: 0
         *   tiny: 2      # 2px - minimal separation
         *   small: 4     # 4px - tight spacing
         *   medium: 8    # 8px - standard (default)
         *   large: 16    # 16px - loose spacing
         *   xlarge: 24   # 24px - section boundaries
         * ```
         */
        struct spacing_values {
            int none = 0;
            int tiny = 0;
            int small = 1;
            int medium = 1;
            int large = 2;
            int xlarge = 3;

            /**
             * @brief Resolve spacing enum to backend-specific integer
             * @param s The semantic spacing value
             * @return Backend-specific integer (pixels or character cells)
             */
            [[nodiscard]] constexpr int resolve(spacing s) const noexcept {
                switch (s) {
                    case spacing::none:   return none;
                    case spacing::tiny:   return tiny;
                    case spacing::small:  return small;
                    case spacing::medium: return medium;
                    case spacing::large:  return large;
                    case spacing::xlarge: return xlarge;
                }
                return medium;  // Unreachable, but satisfies compiler
            }
        };

        // Widget-specific styles
        button_style button{};            // BREAKING CHANGE - refactored
        label_style label{};              // Unchanged
        line_edit_style line_edit{};      // NEW: Line edit input widget (Phase 1)
        checkbox_style checkbox{};        // NEW: Checkbox input widget (Phase 2)
        radio_button_style radio_button{};// NEW: Radio button input widget (Phase 2)
        progress_bar_style progress_bar{};// NEW: Progress bar widget
        slider_style slider{};            // NEW: Slider input widget
        tab_widget_style tab_widget{};    // NEW: Tab widget container
        panel_style panel{};              // Unchanged
        menu_style menu{};                // Unchanged
        menu_bar_style menu_bar{};        // Unchanged
        separator_style separator{};      // Unchanged

        // NEW: Menu item styles
        menu_item_style menu_item{};
        menu_bar_item_style menu_bar_item{};

        // Scrollbar style (Phase 3)
        scrollbar_theme scrollbar{};

        // Window style (Phase 8)
        window_style window{};

        // Spacing resolution (backend-specific)
        spacing_values spacing{};

        // Global palette
        color_type window_bg;
        color_type text_fg;
        color_type border_color;

    };
}
