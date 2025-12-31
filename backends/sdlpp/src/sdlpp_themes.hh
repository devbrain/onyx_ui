/**
 * @file sdlpp_themes.hh
 * @brief Built-in theme definitions for SDL++ backend
 */

#pragma once

#include <onyxui/theming/theme.hh>
#include <onyxui/sdlpp/sdlpp_backend.hh>

namespace onyxui::sdlpp {

/**
 * @brief Create Windows 3.11 style theme (default)
 *
 * Classic Windows 3.11 color scheme with 3D beveled controls.
 */
inline ui_theme<sdlpp_backend> create_windows311_theme()
{
    ui_theme<sdlpp_backend> t;
    t.name = "Windows 3.11";
    t.description = "Classic Windows 3.11 color scheme with 3D beveled controls";

    // Windows 3.11 system colors
    constexpr color button_face{192, 192, 192};      // btnface
    constexpr color button_shadow{128, 128, 128};    // btnshadow
    constexpr color button_highlight{255, 255, 255}; // btnhighlight
    constexpr color window_bg{192, 192, 192};        // window background
    constexpr color window_text{0, 0, 0};            // windowtext
    constexpr color highlight{0, 0, 128};            // highlight (navy)
    constexpr color highlight_text{255, 255, 255};   // highlighttext
    constexpr color white{255, 255, 255};
    constexpr color gray_text{140, 140, 140};        // disabled/grayed text (darker for visibility)

    // Default font (will use system fallback, sized for Windows 3.11 look)
    // MS Sans Serif 8pt was typical; we use 14px as a reasonable approximation
    sdlpp_renderer::font default_font{};
    default_font.size_px = 14.0f;

    sdlpp_renderer::font bold_font{};
    bold_font.size_px = 14.0f;
    bold_font.bold = true;

    // Global palette
    t.window_bg = window_bg;
    t.text_fg = window_text;
    t.border_color = button_shadow;

    // Spacing values (pixel-based for SDL++ backend)
    t.spacing.none = 0;
    t.spacing.tiny = 2;
    t.spacing.small = 4;
    t.spacing.medium = 8;
    t.spacing.large = 16;
    t.spacing.xlarge = 24;

    // Button - 3D raised style
    t.button.normal.background = button_face;
    t.button.normal.foreground = window_text;
    t.button.normal.font = default_font;
    t.button.normal.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.button.hover.background = button_face;
    t.button.hover.foreground = window_text;
    t.button.hover.font = default_font;
    t.button.hover.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.button.pressed.background = button_face;
    t.button.pressed.foreground = window_text;
    t.button.pressed.font = default_font;
    t.button.pressed.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.button.disabled.background = button_face;
    t.button.disabled.foreground = gray_text;  // Lighter gray for disabled
    t.button.disabled.font = default_font;
    t.button.disabled.mnemonic_foreground = gray_text;

    t.button.mnemonic_font = default_font;
    t.button.mnemonic_font.underline = true;

    t.button.box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.button.padding_horizontal = 8;
    t.button.padding_vertical = 4;

    // Label
    t.label.text = window_text;
    t.label.background = window_bg;
    t.label.font = default_font;
    t.label.mnemonic_font = default_font;
    t.label.mnemonic_font.underline = true;

    // Panel
    t.panel.background = window_bg;
    t.panel.border_color = button_shadow;

    // Menu
    t.menu.background = window_bg;
    t.menu.border_color = button_shadow;

    // Menu item
    t.menu_item.normal.background = window_bg;
    t.menu_item.normal.foreground = window_text;
    t.menu_item.highlighted.background = highlight;
    t.menu_item.highlighted.foreground = highlight_text;
    t.menu_item.disabled.background = window_bg;
    t.menu_item.disabled.foreground = button_shadow;

    // Menu bar
    t.menu_bar.item_spacing = 4;             // Space between File, Edit, etc.
    t.menu_bar.item_padding_horizontal = 8;  // Horizontal padding inside each item
    t.menu_bar.item_padding_vertical = 4;    // Vertical padding inside each item

    // Menu bar item
    t.menu_bar_item.normal.background = window_bg;
    t.menu_bar_item.normal.foreground = window_text;
    t.menu_bar_item.normal.font = default_font;
    t.menu_bar_item.normal.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.menu_bar_item.hover.background = highlight;
    t.menu_bar_item.hover.foreground = highlight_text;
    t.menu_bar_item.hover.font = default_font;
    t.menu_bar_item.hover.mnemonic_foreground = highlight_text;

    t.menu_bar_item.open.background = highlight;
    t.menu_bar_item.open.foreground = highlight_text;
    t.menu_bar_item.open.font = default_font;
    t.menu_bar_item.open.mnemonic_foreground = highlight_text;

    t.menu_bar_item.mnemonic_font = default_font;
    t.menu_bar_item.mnemonic_font.underline = true;

    // Line edit - 3D sunken style
    t.line_edit.text = window_text;
    t.line_edit.background = white;
    t.line_edit.border_color = button_shadow;
    t.line_edit.placeholder_text = button_shadow;
    t.line_edit.cursor = window_text;
    t.line_edit.box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::sunken, true};

    // ========================================================================
    // Scrollbar
    // ========================================================================
    t.scrollbar.width = 16;              // Standard Win3.11 scrollbar width
    t.scrollbar.min_thumb_size = 16;     // Minimum thumb size in pixels
    t.scrollbar.arrow_size = 16;         // Arrow button size
    t.scrollbar.min_render_size = 32;    // Minimum size to render scrollbar
    t.scrollbar.line_increment = 16;     // Pixels per line scroll

    t.scrollbar.track_normal.background = button_face;
    t.scrollbar.track_normal.foreground = button_shadow;
    t.scrollbar.thumb_normal.background = button_face;
    t.scrollbar.thumb_normal.foreground = window_text;
    t.scrollbar.thumb_normal.box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.scrollbar.thumb_hover = t.scrollbar.thumb_normal;
    t.scrollbar.thumb_pressed = t.scrollbar.thumb_normal;
    t.scrollbar.thumb_disabled.background = button_face;
    t.scrollbar.thumb_disabled.foreground = button_shadow;
    t.scrollbar.arrow_normal.background = button_face;
    t.scrollbar.arrow_normal.foreground = window_text;
    t.scrollbar.arrow_normal.box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.scrollbar.arrow_hover = t.scrollbar.arrow_normal;
    t.scrollbar.arrow_pressed = t.scrollbar.arrow_normal;

    // Scrollbar arrow icons
    t.scrollbar.arrow_up_icon = sdlpp_renderer::icon_style::arrow_up;
    t.scrollbar.arrow_down_icon = sdlpp_renderer::icon_style::arrow_down;
    t.scrollbar.arrow_left_icon = sdlpp_renderer::icon_style::arrow_left;
    t.scrollbar.arrow_right_icon = sdlpp_renderer::icon_style::arrow_right;

    // ========================================================================
    // Line Edit (text input)
    // ========================================================================
    t.line_edit.cursor_insert_icon = sdlpp_renderer::icon_style::cursor_insert;
    t.line_edit.cursor_overwrite_icon = sdlpp_renderer::icon_style::cursor_overwrite;
    t.line_edit.cursor_blink_interval_ms = 530;  // Classic Windows blink rate

    // ========================================================================
    // Checkbox
    // ========================================================================
    t.checkbox.unchecked_icon = sdlpp_renderer::icon_style::checkbox_unchecked;
    t.checkbox.checked_icon = sdlpp_renderer::icon_style::checkbox_checked;
    t.checkbox.indeterminate_icon = sdlpp_renderer::icon_style::checkbox_indeterminate;

    t.checkbox.normal.font = default_font;
    t.checkbox.normal.foreground = window_text;
    t.checkbox.normal.background = window_bg;
    t.checkbox.normal.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.checkbox.hover.font = default_font;
    t.checkbox.hover.foreground = window_text;
    t.checkbox.hover.background = window_bg;
    t.checkbox.hover.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.checkbox.checked.font = default_font;
    t.checkbox.checked.foreground = window_text;
    t.checkbox.checked.background = window_bg;
    t.checkbox.checked.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.checkbox.disabled.font = default_font;
    t.checkbox.disabled.foreground = gray_text;  // Lighter gray
    t.checkbox.disabled.background = window_bg;
    t.checkbox.disabled.mnemonic_foreground = gray_text;

    t.checkbox.mnemonic_font = default_font;
    t.checkbox.mnemonic_font.underline = true;
    t.checkbox.spacing = 4;  // Space between checkbox and label

    // ========================================================================
    // Radio Button
    // ========================================================================
    t.radio_button.unchecked_icon = sdlpp_renderer::icon_style::radio_unchecked;
    t.radio_button.checked_icon = sdlpp_renderer::icon_style::radio_checked;

    t.radio_button.normal.font = default_font;
    t.radio_button.normal.foreground = window_text;
    t.radio_button.normal.background = window_bg;
    t.radio_button.normal.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.radio_button.hover.font = default_font;
    t.radio_button.hover.foreground = window_text;
    t.radio_button.hover.background = window_bg;
    t.radio_button.hover.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.radio_button.checked.font = default_font;
    t.radio_button.checked.foreground = window_text;
    t.radio_button.checked.background = window_bg;
    t.radio_button.checked.mnemonic_foreground = highlight;  // Navy blue for emphasis

    t.radio_button.disabled.font = default_font;
    t.radio_button.disabled.foreground = gray_text;  // Lighter gray
    t.radio_button.disabled.background = window_bg;
    t.radio_button.disabled.mnemonic_foreground = gray_text;

    t.radio_button.mnemonic_font = default_font;
    t.radio_button.mnemonic_font.underline = true;
    t.radio_button.spacing = 4;

    // ========================================================================
    // Progress Bar
    // ========================================================================
    t.progress_bar.filled_color = highlight;           // Navy blue filled
    t.progress_bar.empty_color = white;                // White empty
    t.progress_bar.text_color = window_text;
    t.progress_bar.text_font = default_font;
    t.progress_bar.filled_icon = sdlpp_renderer::icon_style::progress_filled;
    t.progress_bar.empty_icon = sdlpp_renderer::icon_style::progress_empty;

    // ========================================================================
    // Slider
    // ========================================================================
    t.slider.track_filled_color = highlight;
    t.slider.track_empty_color = button_shadow;
    t.slider.thumb_color = button_face;
    t.slider.tick_color = button_shadow;
    t.slider.filled_icon = sdlpp_renderer::icon_style::slider_filled;
    t.slider.empty_icon = sdlpp_renderer::icon_style::slider_empty;
    t.slider.thumb_icon = sdlpp_renderer::icon_style::slider_thumb;

    // ========================================================================
    // Menu Item
    // ========================================================================
    // Normal state
    t.menu_item.normal.font = default_font;
    t.menu_item.normal.mnemonic_foreground = highlight;  // Navy blue for emphasis

    // Highlighted state (hover/focus)
    t.menu_item.highlighted.font = default_font;
    t.menu_item.highlighted.mnemonic_foreground = highlight_text;  // White on blue

    // Disabled state
    t.menu_item.disabled.font = default_font;
    t.menu_item.disabled.mnemonic_foreground = gray_text;  // Lighter gray

    // Shortcut hints (Ctrl+S, etc.) - slightly dimmer than main text
    t.menu_item.shortcut.font = default_font;
    t.menu_item.shortcut.foreground = button_shadow;           // Gray for normal
    t.menu_item.shortcut.background = window_bg;
    t.menu_item.shortcut.mnemonic_foreground = button_shadow;

    // Mnemonic font (underlined character)
    t.menu_item.mnemonic_font = default_font;
    t.menu_item.mnemonic_font.underline = true;

    // Layout and icons
    t.menu_item.submenu_icon = sdlpp_renderer::icon_style::folder;
    t.menu_item.padding_horizontal = 4;
    t.menu_item.padding_vertical = 2;

    // ========================================================================
    // Separator
    // ========================================================================
    t.separator.foreground = button_shadow;

    // ========================================================================
    // Tab Widget
    // ========================================================================
    t.tab_widget.tab_bar_background = button_face;
    t.tab_widget.tab_normal_background = button_face;
    t.tab_widget.tab_normal_text = window_text;
    t.tab_widget.tab_active_background = white;
    t.tab_widget.tab_active_text = window_text;
    t.tab_widget.tab_hover_background = button_face;
    t.tab_widget.tab_hover_text = window_text;
    t.tab_widget.tab_border = button_shadow;
    t.tab_widget.tab_active_border = window_text;
    t.tab_widget.close_button_normal = button_shadow;
    t.tab_widget.close_button_hover = color{255, 0, 0};  // Red on hover
    t.tab_widget.close_button_icon = sdlpp_renderer::icon_style::close_x;
    t.tab_widget.tab_font = default_font;
    t.tab_widget.tab_active_font = bold_font;
    t.tab_widget.scroll_arrow_normal = button_shadow;
    t.tab_widget.scroll_arrow_hover = window_text;
    t.tab_widget.scroll_arrow_pressed = highlight;
    t.tab_widget.scroll_arrow_disabled = color{160, 160, 160};
    t.tab_widget.scroll_left_icon = sdlpp_renderer::icon_style::arrow_left;
    t.tab_widget.scroll_right_icon = sdlpp_renderer::icon_style::arrow_right;
    t.tab_widget.tab_padding_horizontal = 8;
    t.tab_widget.tab_padding_vertical = 4;
    t.tab_widget.tab_spacing = 2;
    t.tab_widget.close_button_spacing = 4;
    t.tab_widget.min_tab_width = 60;
    t.tab_widget.scroll_arrow_width = 16;

    // ========================================================================
    // Window (MDI style)
    // ========================================================================
    t.window.title_focused.background = highlight;
    t.window.title_focused.foreground = highlight_text;
    t.window.title_focused.mnemonic_foreground = highlight_text;
    t.window.title_focused.font = bold_font;

    t.window.title_unfocused.background = button_face;
    t.window.title_unfocused.foreground = button_shadow;
    t.window.title_unfocused.mnemonic_foreground = button_shadow;
    t.window.title_unfocused.font = default_font;

    t.window.border_focused = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.window.border_unfocused = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.window.border_color_focused = button_shadow;
    t.window.border_color_unfocused = button_shadow;
    t.window.content_background = window_bg;

    t.window.shadow.enabled = false;  // Win3.11 didn't have window shadows

    return t;
}

} // namespace onyxui::sdlpp
