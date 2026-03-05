/**
 * @file tile_theme.hh
 * @brief Widget tile configurations and theme structure
 *
 * This file defines per-widget tile configurations that map widget
 * visual elements to tile IDs in the atlas. Each widget type has
 * its own config struct matching its visual structure.
 */

#pragma once

#include <onyxui/tile/tile_types.hh>

namespace onyxui::tile {

// ============================================================================
// Widget Tile Configurations
// ============================================================================

/**
 * @struct button_tiles
 * @brief Tile configuration for button widget states
 *
 * Buttons use nine_slice for scalable backgrounds with different
 * appearances for each interaction state.
 */
struct button_tiles {
    nine_slice normal;      ///< Default state
    nine_slice hover;       ///< Mouse over
    nine_slice pressed;     ///< Mouse down / active
    nine_slice disabled;    ///< Disabled state
    nine_slice focused;     ///< Keyboard focus (optional, uses normal if invalid)
};

/**
 * @struct panel_tiles
 * @brief Tile configuration for panel/container widget
 */
struct panel_tiles {
    nine_slice background;  ///< Panel background
};

/**
 * @struct label_tiles
 * @brief Tile configuration for label widget (text only)
 *
 * Labels typically don't have background tiles, just use bitmap_font.
 * This struct is provided for consistency and optional background.
 */
struct label_tiles {
    nine_slice background;  ///< Optional background (use invalid for none)
};

/**
 * @struct progress_bar_tiles
 * @brief Tile configuration for progress bar
 *
 * Progress bars have a track (background) and fill (foreground).
 * Both use h_slice for horizontal stretching with distinct caps.
 */
struct progress_bar_tiles {
    h_slice track;          ///< Background track (full width)
    h_slice fill;           ///< Foreground fill (partial width)
};

/**
 * @struct scrollbar_tiles
 * @brief Tile configuration for scrollbar (vertical and horizontal)
 */
struct scrollbar_tiles {
    // Vertical scrollbar
    v_slice track_v;        ///< Vertical track background
    v_slice thumb_v;        ///< Vertical thumb/handle
    int arrow_up{-1};       ///< Up arrow tile
    int arrow_down{-1};     ///< Down arrow tile

    // Horizontal scrollbar
    h_slice track_h;        ///< Horizontal track background
    h_slice thumb_h;        ///< Horizontal thumb/handle
    int arrow_left{-1};     ///< Left arrow tile
    int arrow_right{-1};    ///< Right arrow tile
};

/**
 * @struct slider_tiles
 * @brief Tile configuration for slider widget
 */
struct slider_tiles {
    h_slice track;          ///< Track background
    int thumb{-1};          ///< Thumb/handle tile (single tile)
    int thumb_hover{-1};    ///< Thumb hover state (optional)
    int thumb_pressed{-1};  ///< Thumb pressed state (optional)
};

/**
 * @struct checkbox_tiles
 * @brief Tile configuration for checkbox widget
 */
struct checkbox_tiles {
    int unchecked{-1};      ///< Unchecked state
    int checked{-1};        ///< Checked state
    int indeterminate{-1};  ///< Indeterminate/mixed state (optional)
    int unchecked_hover{-1};///< Unchecked hover (optional)
    int checked_hover{-1};  ///< Checked hover (optional)
    int disabled{-1};       ///< Disabled state (optional)
};

/**
 * @struct radio_tiles
 * @brief Tile configuration for radio button widget
 */
struct radio_tiles {
    int unchecked{-1};      ///< Unselected state
    int checked{-1};        ///< Selected state
    int unchecked_hover{-1};///< Unselected hover (optional)
    int checked_hover{-1};  ///< Selected hover (optional)
    int disabled{-1};       ///< Disabled state (optional)
};

/**
 * @struct tab_tiles
 * @brief Tile configuration for tab widget
 */
struct tab_tiles {
    h_slice active;         ///< Active/selected tab
    h_slice inactive;       ///< Inactive tab
    h_slice hover;          ///< Hovered tab (optional)
    nine_slice panel;       ///< Tab content panel background
};

/**
 * @struct text_input_tiles
 * @brief Tile configuration for text input / line edit widget
 */
struct text_input_tiles {
    nine_slice normal;      ///< Default state
    nine_slice focused;     ///< Has keyboard focus
    nine_slice disabled;    ///< Disabled state
    int cursor{-1};         ///< Text cursor tile
};

/**
 * @struct combo_tiles
 * @brief Tile configuration for combo box / dropdown widget
 */
struct combo_tiles {
    nine_slice button;      ///< Main button area
    nine_slice button_hover;///< Button hover state (optional)
    nine_slice button_open; ///< Button when dropdown is open (optional)
    int arrow{-1};          ///< Dropdown arrow tile
    nine_slice dropdown;    ///< Dropdown popup background
    nine_slice item_hover;  ///< Hovered item background
    nine_slice item_selected;///< Selected item background (optional)
};

/**
 * @struct list_tiles
 * @brief Tile configuration for list box widget
 */
struct list_tiles {
    nine_slice background;  ///< List background/border
    nine_slice item_hover;  ///< Hovered item background
    nine_slice item_selected;///< Selected item background
    nine_slice item_focused;///< Focused item (keyboard nav)
};

/**
 * @struct window_tiles
 * @brief Tile configuration for window widget
 */
struct window_tiles {
    nine_slice frame;       ///< Window frame/border
    nine_slice titlebar;    ///< Title bar background
    int close_button{-1};   ///< Close button tile
    int close_hover{-1};    ///< Close button hover (optional)
    int minimize_button{-1};///< Minimize button tile (optional)
    int minimize_hover{-1}; ///< Minimize hover (optional)
    int maximize_button{-1};///< Maximize button tile (optional)
    int maximize_hover{-1}; ///< Maximize hover (optional)
    int restore_button{-1}; ///< Restore button tile (optional)
};

/**
 * @struct menu_tiles
 * @brief Tile configuration for menu widgets
 */
struct menu_tiles {
    nine_slice bar;         ///< Menu bar background
    nine_slice popup;       ///< Popup menu background
    nine_slice item_hover;  ///< Hovered menu item
    int separator{-1};      ///< Menu separator tile
    int submenu_arrow{-1};  ///< Submenu indicator arrow
    int checkmark{-1};      ///< Checkmark for checked items
};

/**
 * @struct tooltip_tiles
 * @brief Tile configuration for tooltip widget
 */
struct tooltip_tiles {
    nine_slice background;  ///< Tooltip background
};

/**
 * @struct group_box_tiles
 * @brief Tile configuration for group box widget
 */
struct group_box_tiles {
    nine_slice frame;       ///< Group box border/frame
};

// ============================================================================
// Animation Settings
// ============================================================================

/**
 * @struct animation_settings
 * @brief Configuration for UI animations
 */
struct animation_settings {
    unsigned int modal_open_ms{200};    ///< Modal window open animation duration
    unsigned int modal_close_ms{150};   ///< Modal window close animation duration
    unsigned int tooltip_delay_ms{500}; ///< Delay before showing tooltip
    unsigned int tooltip_fade_ms{100};  ///< Tooltip fade in/out duration
    unsigned int blink_interval_ms{500};///< Cursor/selection blink interval
    bool enable_animations{true};       ///< Global animation toggle
};

// ============================================================================
// Complete Theme
// ============================================================================

/**
 * @struct tile_theme
 * @brief Complete tile theme containing all widget configurations
 *
 * This struct aggregates all tile configurations for a complete UI theme.
 * Game developers define one tile_theme and set it globally at startup.
 *
 * Usage:
 * @code
 * tile_theme my_theme {
 *     .atlas = &my_atlas,
 *     .button = {
 *         .normal = {0,1,2, 8,9,10, 16,17,18, 4, 4},
 *         .hover = {3,4,5, 11,12,13, 19,20,21, 4, 4},
 *         // ...
 *     },
 *     // ... other widget configs
 * };
 * tile::set_theme(my_theme);
 * @endcode
 */
struct tile_theme {
    // Atlas reference
    tile_atlas* atlas{nullptr};

    // Widget tile configurations
    button_tiles button;
    panel_tiles panel;
    label_tiles label;
    progress_bar_tiles progress_bar;
    scrollbar_tiles scrollbar;
    slider_tiles slider;
    checkbox_tiles checkbox;
    radio_tiles radio;
    tab_tiles tab;
    text_input_tiles text_input;
    combo_tiles combo;
    list_tiles list;
    window_tiles window;
    menu_tiles menu;
    tooltip_tiles tooltip;
    group_box_tiles group_box;

    // Fonts (pre-colored, one per style)
    bitmap_font font_normal;
    bitmap_font font_disabled;
    bitmap_font font_highlight;
    bitmap_font font_title;
    bitmap_font font_small;

    // Animation settings
    animation_settings animations;

    /// Check if the theme has a valid atlas
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return atlas != nullptr && atlas->is_valid();
    }
};

// ============================================================================
// State-to-Tile Mapping Helpers
// ============================================================================

/**
 * @enum widget_visual_state
 * @brief Common visual states for interactive widgets
 *
 * This enum unifies the visual state representation across all tile widgets,
 * enabling a common state-to-tile mapping helper.
 */
enum class widget_visual_state : std::uint8_t {
    normal = 0,     ///< Default state
    hover,          ///< Mouse over
    pressed,        ///< Mouse down / active
    disabled,       ///< Disabled / greyed out
    focused,        ///< Has keyboard focus
};

/**
 * @brief Get the appropriate nine_slice for a button based on visual state
 * @param tiles Button tile configuration
 * @param state Current visual state
 * @return Reference to the appropriate nine_slice for the state
 *
 * Falls back to normal state if the requested state's slice is invalid.
 */
[[nodiscard]] inline const nine_slice& get_button_slice(
    const button_tiles& tiles,
    widget_visual_state state) noexcept
{
    switch (state) {
        case widget_visual_state::hover:
            return tiles.hover.is_valid() ? tiles.hover : tiles.normal;
        case widget_visual_state::pressed:
            return tiles.pressed.is_valid() ? tiles.pressed : tiles.normal;
        case widget_visual_state::disabled:
            return tiles.disabled.is_valid() ? tiles.disabled : tiles.normal;
        case widget_visual_state::focused:
            return tiles.focused.is_valid() ? tiles.focused : tiles.normal;
        default:
            return tiles.normal;
    }
}

/**
 * @brief Get the appropriate tile ID for a checkbox based on state
 * @param tiles Checkbox tile configuration
 * @param checked Whether the checkbox is checked
 * @param hover Whether the mouse is hovering
 * @param disabled Whether the checkbox is disabled
 * @return Tile ID for the current state, or -1 if no appropriate tile
 */
[[nodiscard]] inline int get_checkbox_tile(
    const checkbox_tiles& tiles,
    bool checked,
    bool hover,
    bool disabled) noexcept
{
    if (disabled && tiles.disabled >= 0) {
        return tiles.disabled;
    }

    if (checked) {
        if (hover && tiles.checked_hover >= 0) {
            return tiles.checked_hover;
        }
        return tiles.checked;
    } else {
        if (hover && tiles.unchecked_hover >= 0) {
            return tiles.unchecked_hover;
        }
        return tiles.unchecked;
    }
}

/**
 * @brief Get the appropriate tile ID for a radio button based on state
 * @param tiles Radio tile configuration
 * @param selected Whether the radio is selected
 * @param hover Whether the mouse is hovering
 * @param disabled Whether the radio is disabled
 * @return Tile ID for the current state, or -1 if no appropriate tile
 */
[[nodiscard]] inline int get_radio_tile(
    const radio_tiles& tiles,
    bool selected,
    bool hover,
    bool disabled) noexcept
{
    if (disabled && tiles.disabled >= 0) {
        return tiles.disabled;
    }

    if (selected) {
        if (hover && tiles.checked_hover >= 0) {
            return tiles.checked_hover;
        }
        return tiles.checked;
    } else {
        if (hover && tiles.unchecked_hover >= 0) {
            return tiles.unchecked_hover;
        }
        return tiles.unchecked;
    }
}

/**
 * @brief Get the appropriate nine_slice for text input based on state
 * @param tiles Text input tile configuration
 * @param focused Whether the input has focus
 * @param disabled Whether the input is disabled
 * @return Reference to the appropriate nine_slice for the state
 */
[[nodiscard]] inline const nine_slice& get_text_input_slice(
    const text_input_tiles& tiles,
    bool focused,
    bool disabled) noexcept
{
    if (disabled && tiles.disabled.is_valid()) {
        return tiles.disabled;
    }
    if (focused && tiles.focused.is_valid()) {
        return tiles.focused;
    }
    return tiles.normal;
}

/**
 * @brief Get the appropriate slider thumb tile based on state
 * @param tiles Slider tile configuration
 * @param hover Whether the mouse is hovering over thumb
 * @param pressed Whether the thumb is being dragged
 * @return Tile ID for the current state
 */
[[nodiscard]] inline int get_slider_thumb_tile(
    const slider_tiles& tiles,
    bool hover,
    bool pressed) noexcept
{
    if (pressed && tiles.thumb_pressed >= 0) {
        return tiles.thumb_pressed;
    }
    if (hover && tiles.thumb_hover >= 0) {
        return tiles.thumb_hover;
    }
    return tiles.thumb;
}

} // namespace onyxui::tile
