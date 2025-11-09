//
// Theme Defaults - Smart default generation for minimal themes (Phase 4)
// Enables 3-5 color themes to generate 50+ consistent styling values
//

#pragma once

#include <onyxui/theming/theme.hh>
#include <onyxui/utils/color_utils.hh>

namespace onyxui::theme_defaults {

/**
 * @brief Tracks which theme fields were explicitly specified
 * @details Used to determine which fields need default generation
 *
 * Each boolean indicates whether the corresponding field was present
 * in the theme definition (YAML file, programmatic initialization, etc.)
 */
template<UIBackend Backend>
struct theme_completeness {
    // Global colors
    bool has_window_bg = false;
    bool has_text_fg = false;
    bool has_border_color = false;

    // Button states
    bool has_button_normal = false;
    bool has_button_hover = false;
    bool has_button_pressed = false;
    bool has_button_disabled = false;

    // Label
    bool has_label_text = false;
    bool has_label_background = false;

    // Panel
    bool has_panel_background = false;
    bool has_panel_border_color = false;

    // Menu
    bool has_menu_background = false;
    bool has_menu_border_color = false;

    // Menu item states
    bool has_menu_item_normal = false;
    bool has_menu_item_highlighted = false;
    bool has_menu_item_disabled = false;
    bool has_menu_item_shortcut = false;

    // Menu bar item states
    bool has_menu_bar_item_normal = false;
    bool has_menu_bar_item_hover = false;
    bool has_menu_bar_item_open = false;

    // Separator
    bool has_separator_foreground = false;

    // Scrollbar (simplified - track major components)
    bool has_scrollbar_track = false;
    bool has_scrollbar_thumb_normal = false;
    bool has_scrollbar_thumb_hover = false;
    bool has_scrollbar_arrow = false;
};

/**
 * @brief Check which fields are present in a theme
 * @param theme The theme to analyze
 * @return Completeness tracking structure
 *
 * @details
 * This is a heuristic check - it compares colors against default-constructed
 * values to guess which fields were explicitly set. Not perfect, but sufficient
 * for detecting truly minimal themes.
 *
 * More accurate tracking requires integration with the YAML loader to record
 * which fields were present in the input file.
 */
template<UIBackend Backend>
[[nodiscard]] theme_completeness<Backend> check_completeness(const ui_theme<Backend>& theme) {
    theme_completeness<Backend> result;

    using color_type = typename Backend::color_type;

    // Default-constructed instances for comparison
    const color_type default_color{};

    // Global colors - check if different from default
    result.has_window_bg = (theme.window_bg != default_color);
    result.has_text_fg = (theme.text_fg != default_color);
    result.has_border_color = (theme.border_color != default_color);

    // Button states
    result.has_button_normal = (theme.button.normal.foreground != default_color ||
                                 theme.button.normal.background != default_color);
    result.has_button_hover = (theme.button.hover.foreground != default_color ||
                                theme.button.hover.background != default_color);
    result.has_button_pressed = (theme.button.pressed.foreground != default_color ||
                                  theme.button.pressed.background != default_color);
    result.has_button_disabled = (theme.button.disabled.foreground != default_color ||
                                   theme.button.disabled.background != default_color);

    // Label
    result.has_label_text = (theme.label.text != default_color);
    result.has_label_background = (theme.label.background != default_color);

    // Panel
    result.has_panel_background = (theme.panel.background != default_color);
    result.has_panel_border_color = (theme.panel.border_color != default_color);

    // Menu
    result.has_menu_background = (theme.menu.background != default_color);
    result.has_menu_border_color = (theme.menu.border_color != default_color);

    // Menu item states
    result.has_menu_item_normal = (theme.menu_item.normal.foreground != default_color ||
                                    theme.menu_item.normal.background != default_color);
    result.has_menu_item_highlighted = (theme.menu_item.highlighted.foreground != default_color ||
                                         theme.menu_item.highlighted.background != default_color);
    result.has_menu_item_disabled = (theme.menu_item.disabled.foreground != default_color ||
                                      theme.menu_item.disabled.background != default_color);
    result.has_menu_item_shortcut = (theme.menu_item.shortcut.foreground != default_color ||
                                      theme.menu_item.shortcut.background != default_color);

    // Menu bar item states
    result.has_menu_bar_item_normal = (theme.menu_bar_item.normal.foreground != default_color ||
                                        theme.menu_bar_item.normal.background != default_color);
    result.has_menu_bar_item_hover = (theme.menu_bar_item.hover.foreground != default_color ||
                                       theme.menu_bar_item.hover.background != default_color);
    result.has_menu_bar_item_open = (theme.menu_bar_item.open.foreground != default_color ||
                                      theme.menu_bar_item.open.background != default_color);

    // Separator
    result.has_separator_foreground = (theme.separator.foreground != default_color);

    // Scrollbar (simplified)
    result.has_scrollbar_track = (theme.scrollbar.track_normal.background != default_color);
    result.has_scrollbar_thumb_normal = (theme.scrollbar.thumb_normal.background != default_color);
    result.has_scrollbar_thumb_hover = (theme.scrollbar.thumb_hover.background != default_color);
    result.has_scrollbar_arrow = (theme.scrollbar.arrow_normal.background != default_color);

    return result;
}

// Removed get_palette_color() - not needed for post-loading defaults

/**
 * @brief Generate hover state from normal state
 * @param normal The normal visual state
 * @param accent_color Accent color from palette (for text highlighting)
 * @return Generated hover state
 *
 * @details Default rules:
 * - Foreground: Use accent color (typically bright/yellow)
 * - Background: Slightly lighten normal background (20%)
 * - Font: Bold for emphasis
 */
template<UIBackend Backend>
[[nodiscard]] typename ui_theme<Backend>::visual_state generate_hover_state(
    const typename ui_theme<Backend>::visual_state& normal,
    const typename Backend::color_type& accent_color
) {
    using visual_state = typename ui_theme<Backend>::visual_state;
    using rgb_components = color_utils::rgb_components;

    visual_state hover = normal;

    // Set foreground to accent color
    hover.foreground = accent_color;

    // Lighten background by 20%
    const rgb_components bg{normal.background.r, normal.background.g, normal.background.b, 255};
    auto lightened = color_utils::lighten(bg, 0.2f);
    hover.background = typename Backend::color_type{lightened.r, lightened.g, lightened.b};

    // Bold font
    hover.font.bold = true;

    return hover;
}

/**
 * @brief Generate pressed state from normal state
 * @param normal The normal visual state
 * @return Generated pressed state
 *
 * @details Default rules:
 * - Invert both foreground and background colors
 * - Keep font style the same
 */
template<UIBackend Backend>
[[nodiscard]] typename ui_theme<Backend>::visual_state generate_pressed_state(
    const typename ui_theme<Backend>::visual_state& normal
) {
    using visual_state = typename ui_theme<Backend>::visual_state;
    using rgb_components = color_utils::rgb_components;

    visual_state pressed = normal;

    // Invert foreground
    const rgb_components fg{normal.foreground.r, normal.foreground.g, normal.foreground.b, 255};
    auto inverted_fg = color_utils::invert(fg);
    pressed.foreground = typename Backend::color_type{inverted_fg.r, inverted_fg.g, inverted_fg.b};

    // Invert background
    const rgb_components bg{normal.background.r, normal.background.g, normal.background.b, 255};
    auto inverted_bg = color_utils::invert(bg);
    pressed.background = typename Backend::color_type{inverted_bg.r, inverted_bg.g, inverted_bg.b};

    return pressed;
}

/**
 * @brief Generate disabled state from normal state
 * @param normal The normal visual state
 * @return Generated disabled state
 *
 * @details Default rules:
 * - Darken foreground by 60% (for RGB-only backends)
 * - Keep background the same
 * - Keep font style the same
 *
 * @note For backends with alpha support, consider using dim() instead
 */
template<UIBackend Backend>
[[nodiscard]] typename ui_theme<Backend>::visual_state generate_disabled_state(
    const typename ui_theme<Backend>::visual_state& normal
) {
    using visual_state = typename ui_theme<Backend>::visual_state;
    using rgb_components = color_utils::rgb_components;

    visual_state disabled = normal;

    // Darken foreground by 60% (RGB-only approach)
    const rgb_components fg{normal.foreground.r, normal.foreground.g, normal.foreground.b, 255};
    auto darkened = color_utils::darken(fg, 0.6f);
    disabled.foreground = typename Backend::color_type{darkened.r, darkened.g, darkened.b};

    return disabled;
}

/**
 * @brief Apply smart defaults to a theme
 * @param theme The theme to complete (modified in place)
 *
 * @details
 * Fills in missing theme fields based on minimal input using:
 * 1. Visual state generation (hover, pressed, disabled)
 * 2. Widget default copying (label from button, etc.)
 * 3. Color derivation (lighten, darken, dim)
 *
 * Safe to call multiple times - only fills missing fields.
 */
template<UIBackend Backend>
void apply_defaults(ui_theme<Backend>& theme) {
    auto completeness = check_completeness(theme);

    using color_type = typename Backend::color_type;

    // Use global colors from theme as base
    const color_type primary = theme.window_bg;
    const color_type foreground = theme.text_fg;
    const color_type accent = theme.border_color;

    // === Button defaults ===
    const color_type default_color{};

    if (!completeness.has_button_normal && completeness.has_window_bg && completeness.has_text_fg) {
        theme.button.normal.background = primary;
        theme.button.normal.foreground = foreground;
    }

    // Re-check if button.normal is now set (either was already, or just generated)
    bool const has_button_normal = (theme.button.normal.foreground != default_color ||
                                     theme.button.normal.background != default_color);

    if (!completeness.has_button_hover && has_button_normal) {
        theme.button.hover = generate_hover_state<Backend>(theme.button.normal, accent);
    }

    if (!completeness.has_button_pressed && has_button_normal) {
        theme.button.pressed = generate_pressed_state<Backend>(theme.button.normal);
    }

    if (!completeness.has_button_disabled && has_button_normal) {
        theme.button.disabled = generate_disabled_state<Backend>(theme.button.normal);
    }

    // === Label defaults (copy from button) ===
    if (!completeness.has_label_text && has_button_normal) {
        theme.label.text = theme.button.normal.foreground;
    }

    if (!completeness.has_label_background && has_button_normal) {
        theme.label.background = theme.button.normal.background;
    }

    // === Panel defaults ===
    if (!completeness.has_panel_background && completeness.has_window_bg) {
        theme.panel.background = theme.window_bg;
    }

    if (!completeness.has_panel_border_color && completeness.has_border_color) {
        theme.panel.border_color = theme.border_color;
    }

    // === Menu defaults (copy from panel) ===
    if (!completeness.has_menu_background) {
        theme.menu.background = theme.panel.background;
    }

    if (!completeness.has_menu_border_color) {
        theme.menu.border_color = theme.panel.border_color;
    }

    // === Menu item defaults (similar to button) ===
    if (!completeness.has_menu_item_normal && has_button_normal) {
        theme.menu_item.normal = theme.button.normal;
    }

    // Re-check if menu_item.normal is now set
    bool const has_menu_item_normal = (theme.menu_item.normal.foreground != default_color ||
                                        theme.menu_item.normal.background != default_color);

    if (!completeness.has_menu_item_highlighted && has_menu_item_normal) {
        theme.menu_item.highlighted = generate_hover_state<Backend>(theme.menu_item.normal, accent);
    }

    if (!completeness.has_menu_item_disabled && has_menu_item_normal) {
        theme.menu_item.disabled = generate_disabled_state<Backend>(theme.menu_item.normal);
    }

    if (!completeness.has_menu_item_shortcut && has_menu_item_normal) {
        // Shortcut text should be dimmed
        theme.menu_item.shortcut = generate_disabled_state<Backend>(theme.menu_item.normal);
    }

    // === Menu bar item defaults ===
    if (!completeness.has_menu_bar_item_normal && has_button_normal) {
        theme.menu_bar_item.normal = theme.button.normal;
    }

    // Re-check if menu_bar_item.normal is now set
    bool const has_menu_bar_item_normal = (theme.menu_bar_item.normal.foreground != default_color ||
                                            theme.menu_bar_item.normal.background != default_color);

    if (!completeness.has_menu_bar_item_hover && has_menu_bar_item_normal) {
        theme.menu_bar_item.hover = generate_hover_state<Backend>(theme.menu_bar_item.normal, accent);
    }

    if (!completeness.has_menu_bar_item_open && has_menu_bar_item_normal) {
        // Open state similar to hover but inverted
        theme.menu_bar_item.open = generate_pressed_state<Backend>(theme.menu_bar_item.normal);
    }

    // === Scrollbar defaults ===
    // Track = dimmed background
    if (!completeness.has_scrollbar_track && completeness.has_window_bg) {
        using rgb_components = color_utils::rgb_components;
        const rgb_components bg{theme.window_bg.r, theme.window_bg.g, theme.window_bg.b, 255};
        auto dimmed = color_utils::darken(bg, 0.1f);  // Slightly darker
        theme.scrollbar.track_normal.background = color_type{dimmed.r, dimmed.g, dimmed.b};
        theme.scrollbar.track_normal.foreground = theme.border_color;
    }

    // Thumb normal = border color on track background
    if (!completeness.has_scrollbar_thumb_normal) {
        theme.scrollbar.thumb_normal.background = theme.border_color;
        theme.scrollbar.thumb_normal.foreground = theme.text_fg;
    }

    // Re-check if thumb_normal is now set
    bool const has_scrollbar_thumb_normal = (theme.scrollbar.thumb_normal.background != default_color);

    // Thumb hover = lightened thumb
    if (!completeness.has_scrollbar_thumb_hover && has_scrollbar_thumb_normal) {
        using rgb_components = color_utils::rgb_components;
        const rgb_components bg{theme.scrollbar.thumb_normal.background.r,
                         theme.scrollbar.thumb_normal.background.g,
                         theme.scrollbar.thumb_normal.background.b,
                         255};
        auto lightened = color_utils::lighten(bg, 0.3f);
        theme.scrollbar.thumb_hover.background = color_type{lightened.r, lightened.g, lightened.b};
        theme.scrollbar.thumb_hover.foreground = theme.scrollbar.thumb_normal.foreground;
    }

    // Arrow buttons = same as thumb
    if (!completeness.has_scrollbar_arrow) {
        theme.scrollbar.arrow_normal = theme.scrollbar.thumb_normal;
        theme.scrollbar.arrow_hover = theme.scrollbar.thumb_hover;
        theme.scrollbar.arrow_pressed = theme.scrollbar.thumb_pressed;
    }

    // === Separator defaults ===
    // Default separator color = border color (used for menu separators)
    if (!completeness.has_separator_foreground && completeness.has_border_color) {
        theme.separator.foreground = theme.border_color;
    }
}

} // namespace onyxui::theme_defaults
