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

    // Spacing values in LOGICAL UNITS (1 logical unit = 8 pixels for SDL++)
    // These are used by layout system which works in logical coordinates
    t.spacing.none = 0;
    t.spacing.tiny = 0.25;    // 2 pixels
    t.spacing.small = 0.5;    // 4 pixels
    t.spacing.medium = 1;     // 8 pixels
    t.spacing.large = 2;      // 16 pixels
    t.spacing.xlarge = 3;     // 24 pixels

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
    t.button.padding_horizontal = 1;   // 8 pixels in logical units
    t.button.padding_vertical = 0.5;   // 4 pixels in logical units

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

    // Menu bar - values in LOGICAL UNITS (1 logical unit = 8 pixels for SDL++)
    t.menu_bar.item_spacing = 2;             // 16 pixels - Space between File, Edit, etc.
    t.menu_bar.item_padding_horizontal = 1.5;// 12 pixels - Horizontal padding inside each item
    t.menu_bar.item_padding_vertical = 0.5;  // 4 pixels - Vertical padding inside each item

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
    // Scrollbar - values in LOGICAL UNITS (1 logical unit = 8 pixels for SDL++)
    // ========================================================================
    t.scrollbar.width = 2;               // 16 pixels - Standard Win3.11 scrollbar width
    t.scrollbar.min_thumb_size = 2;      // 16 pixels - Minimum thumb size
    t.scrollbar.arrow_size = 2;          // 16 pixels - Arrow button size
    t.scrollbar.min_render_size = 4;     // 32 pixels - Minimum size to render scrollbar
    t.scrollbar.line_increment = 2;      // 16 pixels - Scroll per line

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
    t.checkbox.spacing = 0.5;  // 4 pixels - Space between checkbox and label

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
    t.radio_button.spacing = 0.5;  // 4 pixels

    // ========================================================================
    // Progress Bar - values in LOGICAL UNITS (1 logical unit = 8 pixels for SDL++)
    // ========================================================================
    t.progress_bar.filled_color = highlight;           // Navy blue filled
    t.progress_bar.empty_color = white;                // White empty
    t.progress_bar.text_color = window_text;
    t.progress_bar.text_font = default_font;
    t.progress_bar.filled_icon = sdlpp_renderer::icon_style::progress_filled;
    t.progress_bar.empty_icon = sdlpp_renderer::icon_style::progress_empty;
    t.progress_bar.bar_thickness = 2.0;                // 16 pixels - appropriate for GUI

    // ========================================================================
    // Slider - values in LOGICAL UNITS (1 logical unit = 8 pixels for SDL++)
    // ========================================================================
    t.slider.track_filled_color = highlight;
    t.slider.track_empty_color = button_shadow;
    t.slider.thumb_color = button_face;
    t.slider.tick_color = button_shadow;
    t.slider.filled_icon = sdlpp_renderer::icon_style::slider_filled;
    t.slider.empty_icon = sdlpp_renderer::icon_style::slider_empty;
    t.slider.thumb_icon = sdlpp_renderer::icon_style::slider_thumb;
    t.slider.track_thickness = 1.5;                    // 12 pixels - appropriate for GUI

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

    // Layout and icons - values in LOGICAL UNITS
    t.menu_item.submenu_icon = sdlpp_renderer::icon_style::folder;
    t.menu_item.padding_horizontal = 1.5;  // 12 pixels - more comfortable spacing
    t.menu_item.padding_vertical = 0.5;    // 4 pixels - more comfortable spacing

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
    // Tab widget values in LOGICAL UNITS (1 logical unit = 8 pixels for SDL++)
    t.tab_widget.tab_padding_horizontal = 2;     // 16 pixels - more spacious tabs
    t.tab_widget.tab_padding_vertical = 1;       // 8 pixels - taller tabs
    t.tab_widget.tab_spacing = 0.5;              // 4 pixels - space between tabs
    t.tab_widget.close_button_spacing = 1;       // 8 pixels
    t.tab_widget.min_tab_width = 10;             // 80 pixels - wider minimum
    t.tab_widget.scroll_arrow_width = 2;         // 16 pixels

    // ========================================================================
    // List/Table Items (MVC Phase 2) - values in LOGICAL UNITS
    // ========================================================================
    // Item backgrounds
    t.list.item_background = window_bg;                  // Normal item background
    t.list.item_background_alt = color{245, 245, 245};   // Alternating row (slightly darker)
    t.list.selection_background = highlight;             // Navy blue selection
    t.list.selection_background_inactive = button_shadow; // Gray when unfocused

    // Item foregrounds
    t.list.item_foreground = window_text;                // Black text
    t.list.selection_foreground = highlight_text;        // White on selection
    t.list.selection_foreground_inactive = window_text;  // Black when unfocused

    // Focus styling
    t.list.focus_border_color = window_text;             // Black focus border
    t.list.focus_box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::flat, false}; // Not filled - just border

    // Font
    t.list.font = default_font;

    // Layout - values in PIXELS (theme struct uses int)
    t.list.padding_horizontal = 4;     // 4 pixels
    t.list.padding_vertical = 2;       // 2 pixels
    t.list.min_item_height = 20;       // 20 pixels - comfortable item height

    // ========================================================================
    // Window (MDI style) - values in LOGICAL UNITS (1 logical unit = 8 pixels)
    // ========================================================================
    t.window.title_focused.background = highlight;
    t.window.title_focused.foreground = highlight_text;
    t.window.title_focused.mnemonic_foreground = highlight_text;
    t.window.title_focused.font = bold_font;

    // Unfocused: darker gray background with black text for visibility
    t.window.title_unfocused.background = button_shadow;  // Darker gray (128,128,128)
    t.window.title_unfocused.foreground = window_text;    // Black text for contrast
    t.window.title_unfocused.mnemonic_foreground = window_text;
    t.window.title_unfocused.font = default_font;

    t.window.border_focused = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.window.border_unfocused = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.window.border_color_focused = button_shadow;
    t.window.border_color_unfocused = button_shadow;
    t.window.content_background = window_bg;

    // Title bar layout
    t.window.title_bar_height = 3;    // 24 pixels - comfortable title bar height
    t.window.border_width = 1;        // 8 pixels - standard border

    t.window.shadow.enabled = false;  // Win3.11 didn't have window shadows

    return t;
}

} // namespace onyxui::sdlpp
