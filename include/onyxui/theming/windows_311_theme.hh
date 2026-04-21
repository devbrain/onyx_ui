/**
 * @file windows_311_theme.hh
 * @brief Backend-agnostic factory for the classic Windows 3.11 theme.
 *
 * Constructs a fully-populated `ui_theme<Backend>` from a `palette` of
 * colors + pre-built fonts. Both sdlpp_backend and warlords_backend
 * (and any future SDL-family backend) can share this single definition
 * instead of duplicating ~400 LOC of field assignment.
 *
 * Requirements on the backend:
 *   - `Backend::color_type` constructible from {r, g, b}.
 *   - `Backend::renderer_type::font` default-constructible and assignable.
 *   - `Backend::renderer_type::box_style` constructible from
 *     `{border_style_type::<value>, bool is_solid}`.
 *   - `Backend::renderer_type::border_style_type` enum with
 *     `raised`, `sunken`, `flat` values.
 *   - `Backend::renderer_type::icon_style` enum with the names used
 *     below (arrow_up, close_x, checkbox_checked, etc.).
 *
 * These hold for every SDL-family backend we ship.
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/theming/theme.hh>

namespace onyxui::themes {

/**
 * @brief Color + font palette the Windows 3.11 theme factory consumes.
 *
 * Defaults match the authentic B00merang Windows-3.11 palette; backends
 * override fields they want to tweak (e.g. warlords plugs in BIOS-font
 * handles for default_font / bold_font).
 */
template<UIBackend Backend>
struct windows_311_palette {
    using color_t = typename Backend::color_type;
    using font_t = typename Backend::renderer_type::font;

    color_t button_face      {192, 192, 192};  // btnface #C0C0C0
    color_t button_shadow    {125, 125, 125};  // btnshadow #7D7D7D
    color_t button_highlight {255, 255, 255};
    color_t window_bg        {192, 192, 192};
    color_t window_text      {0,   0,   0  };
    color_t highlight        {0,   0,   170};  // Win3.11 blue #0000AA
    color_t highlight_text   {255, 255, 255};
    color_t white            {255, 255, 255};
    color_t gray_text        {96,  96,  96 };
    color_t disabled_bg      {182, 182, 182};
    color_t menu_bg          {255, 255, 255};  // Menu bg was white in Win3.11
    color_t close_hover      {255, 0,   0  };  // Tab close button hover tint

    font_t default_font{};  // Backend supplies identity (path or font_set)
    font_t bold_font{};
};

/**
 * @brief Construct a fully-populated Windows 3.11 theme for any backend.
 *
 * @param p  Palette. The default-constructed palette is sdlpp's baseline.
 *           Backends that load a different font substrate override the
 *           two font fields before calling.
 * @return   ui_theme<Backend> ready to register with a theme_registry.
 */
template<UIBackend Backend>
[[nodiscard]] ui_theme<Backend> make_windows_311(
    windows_311_palette<Backend> p = {})
{
    using renderer_t = typename Backend::renderer_type;
    using box_style = typename renderer_t::box_style;
    using border = typename renderer_t::border_style_type;
    using icon = typename renderer_t::icon_style;

    ui_theme<Backend> t;
    t.name = "Windows 3.11";
    t.description = "Classic Windows 3.11 color scheme with 3D beveled controls";

    // Global palette
    t.window_bg = p.window_bg;
    t.text_fg = p.window_text;
    t.border_color = p.button_shadow;

    // Spacing values in pixels (same on sdlpp; backends may pre-adjust).
    t.spacing.none   = 0;
    t.spacing.tiny   = 2;
    t.spacing.small  = 4;
    t.spacing.medium = 8;
    t.spacing.large  = 16;
    t.spacing.xlarge = 24;

    // ------------------------------------------------------------------
    // Button — 3D raised
    // ------------------------------------------------------------------
    t.button.normal.background           = p.button_face;
    t.button.normal.foreground           = p.window_text;
    t.button.normal.font                 = p.default_font;
    t.button.normal.mnemonic_foreground  = p.highlight;

    t.button.hover = t.button.normal;
    t.button.pressed = t.button.normal;

    t.button.disabled.background          = p.button_face;
    t.button.disabled.foreground          = p.gray_text;
    t.button.disabled.font                = p.default_font;
    t.button.disabled.mnemonic_foreground = p.gray_text;

    t.button.mnemonic_font = p.default_font;
    t.button.mnemonic_font.underline = true;

    t.button.box_style = box_style{border::raised, true};
    t.button.padding_horizontal = 8;
    t.button.padding_vertical = 4;

    // ------------------------------------------------------------------
    // Label
    // ------------------------------------------------------------------
    t.label.text = p.window_text;
    t.label.background = p.window_bg;
    t.label.font = p.default_font;
    t.label.mnemonic_font = p.default_font;
    t.label.mnemonic_font.underline = true;

    // ------------------------------------------------------------------
    // Panel
    // ------------------------------------------------------------------
    t.panel.background = p.window_bg;
    t.panel.border_color = p.button_shadow;

    // ------------------------------------------------------------------
    // Menu — white background
    // ------------------------------------------------------------------
    t.menu.background = p.menu_bg;
    t.menu.border_color = p.window_text;

    // Menu bar sizing
    t.menu_bar.item_spacing = 0;
    t.menu_bar.item_padding_horizontal = 8;
    t.menu_bar.item_padding_vertical = 4;

    // Menu bar item
    t.menu_bar_item.normal.background          = p.window_bg;
    t.menu_bar_item.normal.foreground          = p.window_text;
    t.menu_bar_item.normal.font                = p.default_font;
    t.menu_bar_item.normal.mnemonic_foreground = p.highlight;

    t.menu_bar_item.hover.background           = p.highlight;
    t.menu_bar_item.hover.foreground           = p.highlight_text;
    t.menu_bar_item.hover.font                 = p.default_font;
    t.menu_bar_item.hover.mnemonic_foreground  = p.highlight_text;

    t.menu_bar_item.open = t.menu_bar_item.hover;

    t.menu_bar_item.mnemonic_font = p.default_font;
    t.menu_bar_item.mnemonic_font.underline = true;

    // Menu item — states
    t.menu_item.normal.background          = p.menu_bg;
    t.menu_item.normal.foreground          = p.window_text;
    t.menu_item.normal.font                = p.default_font;
    t.menu_item.normal.mnemonic_foreground = p.highlight;

    t.menu_item.highlighted.background          = p.highlight;
    t.menu_item.highlighted.foreground          = p.highlight_text;
    t.menu_item.highlighted.font                = p.default_font;
    t.menu_item.highlighted.mnemonic_foreground = p.highlight_text;

    t.menu_item.disabled.background          = p.menu_bg;
    t.menu_item.disabled.foreground          = p.gray_text;
    t.menu_item.disabled.font                = p.default_font;
    t.menu_item.disabled.mnemonic_foreground = p.gray_text;

    t.menu_item.shortcut.font                = p.default_font;
    t.menu_item.shortcut.foreground          = p.gray_text;
    t.menu_item.shortcut.background          = p.menu_bg;
    t.menu_item.shortcut.mnemonic_foreground = p.gray_text;

    t.menu_item.mnemonic_font = p.default_font;
    t.menu_item.mnemonic_font.underline = true;

    t.menu_item.submenu_icon = icon::folder;
    t.menu_item.padding_horizontal = 12;
    t.menu_item.padding_vertical = 4;

    // ------------------------------------------------------------------
    // Line edit — 3D sunken, black border, white bg
    // ------------------------------------------------------------------
    t.line_edit.text = p.window_text;
    t.line_edit.background = p.white;
    t.line_edit.border_color = p.window_text;
    t.line_edit.placeholder_text = p.gray_text;
    t.line_edit.cursor = p.window_text;
    t.line_edit.box_style = box_style{border::sunken, true};
    t.line_edit.cursor_insert_icon = icon::cursor_insert;
    t.line_edit.cursor_overwrite_icon = icon::cursor_overwrite;
    t.line_edit.cursor_blink_interval_ms = 530;

    // ------------------------------------------------------------------
    // Scrollbar
    // ------------------------------------------------------------------
    t.scrollbar.width           = 16;
    t.scrollbar.min_thumb_size  = 16;
    t.scrollbar.arrow_size      = 16;
    t.scrollbar.min_render_size = 32;
    t.scrollbar.line_increment  = 16;

    // Track: sunken + solid so the channel is visible against window_bg.
    t.scrollbar.track_normal.background = p.button_face;
    t.scrollbar.track_normal.foreground = p.button_shadow;
    t.scrollbar.track_normal.box_style  = box_style{border::sunken, true};

    t.scrollbar.thumb_normal.background = p.button_face;
    t.scrollbar.thumb_normal.foreground = p.window_text;
    t.scrollbar.thumb_normal.box_style  = box_style{border::raised, true};
    t.scrollbar.thumb_hover    = t.scrollbar.thumb_normal;
    t.scrollbar.thumb_pressed  = t.scrollbar.thumb_normal;
    t.scrollbar.thumb_disabled.background = p.button_face;
    t.scrollbar.thumb_disabled.foreground = p.button_shadow;

    t.scrollbar.arrow_normal.background = p.button_face;
    t.scrollbar.arrow_normal.foreground = p.window_text;
    t.scrollbar.arrow_normal.box_style  = box_style{border::raised, true};
    t.scrollbar.arrow_hover   = t.scrollbar.arrow_normal;
    t.scrollbar.arrow_pressed = t.scrollbar.arrow_normal;

    t.scrollbar.arrow_up_icon    = icon::arrow_up;
    t.scrollbar.arrow_down_icon  = icon::arrow_down;
    t.scrollbar.arrow_left_icon  = icon::arrow_left;
    t.scrollbar.arrow_right_icon = icon::arrow_right;

    // ------------------------------------------------------------------
    // Checkbox
    // ------------------------------------------------------------------
    t.checkbox.unchecked_icon     = icon::checkbox_unchecked;
    t.checkbox.checked_icon       = icon::checkbox_checked;
    t.checkbox.indeterminate_icon = icon::checkbox_indeterminate;

    t.checkbox.normal.font                = p.default_font;
    t.checkbox.normal.foreground          = p.window_text;
    t.checkbox.normal.background          = p.window_bg;
    t.checkbox.normal.mnemonic_foreground = p.highlight;

    t.checkbox.hover   = t.checkbox.normal;
    t.checkbox.checked = t.checkbox.normal;

    t.checkbox.disabled.font                = p.default_font;
    t.checkbox.disabled.foreground          = p.gray_text;
    t.checkbox.disabled.background          = p.window_bg;
    t.checkbox.disabled.mnemonic_foreground = p.gray_text;

    t.checkbox.mnemonic_font = p.default_font;
    t.checkbox.mnemonic_font.underline = true;
    t.checkbox.spacing = 4;

    // ------------------------------------------------------------------
    // Radio button
    // ------------------------------------------------------------------
    t.radio_button.unchecked_icon = icon::radio_unchecked;
    t.radio_button.checked_icon   = icon::radio_checked;

    t.radio_button.normal.font                = p.default_font;
    t.radio_button.normal.foreground          = p.window_text;
    t.radio_button.normal.background          = p.window_bg;
    t.radio_button.normal.mnemonic_foreground = p.highlight;

    t.radio_button.hover   = t.radio_button.normal;
    t.radio_button.checked = t.radio_button.normal;

    t.radio_button.disabled.font                = p.default_font;
    t.radio_button.disabled.foreground          = p.gray_text;
    t.radio_button.disabled.background          = p.window_bg;
    t.radio_button.disabled.mnemonic_foreground = p.gray_text;

    t.radio_button.mnemonic_font = p.default_font;
    t.radio_button.mnemonic_font.underline = true;
    t.radio_button.spacing = 4;

    // ------------------------------------------------------------------
    // Progress bar
    // ------------------------------------------------------------------
    t.progress_bar.filled_color   = p.highlight;
    t.progress_bar.empty_color    = p.white;
    t.progress_bar.text_color     = p.window_text;
    t.progress_bar.text_font      = p.default_font;
    t.progress_bar.filled_icon    = icon::progress_filled;
    t.progress_bar.empty_icon     = icon::progress_empty;
    t.progress_bar.bar_thickness  = 16.0f;

    // ------------------------------------------------------------------
    // Slider
    // ------------------------------------------------------------------
    t.slider.track_filled_color = p.highlight;
    t.slider.track_empty_color  = p.button_shadow;
    t.slider.thumb_color        = p.button_face;
    t.slider.tick_color         = p.button_shadow;
    t.slider.filled_icon        = icon::slider_filled;
    t.slider.empty_icon         = icon::slider_empty;
    t.slider.thumb_icon         = icon::slider_thumb;
    t.slider.track_thickness    = 12.0f;

    // ------------------------------------------------------------------
    // Separator
    // ------------------------------------------------------------------
    t.separator.foreground = p.gray_text;

    // ------------------------------------------------------------------
    // Tab widget
    // ------------------------------------------------------------------
    t.tab_widget.tab_bar_background       = p.button_face;
    t.tab_widget.tab_normal_background    = p.button_face;
    t.tab_widget.tab_normal_text          = p.window_text;
    t.tab_widget.tab_active_background    = p.button_face;
    t.tab_widget.tab_active_text          = p.window_text;
    t.tab_widget.tab_hover_background     = p.button_face;
    t.tab_widget.tab_hover_text           = p.window_text;
    t.tab_widget.tab_border               = p.window_text;
    t.tab_widget.tab_active_border        = p.window_text;
    t.tab_widget.close_button_normal      = p.button_shadow;
    t.tab_widget.close_button_hover       = p.close_hover;
    t.tab_widget.close_button_icon        = icon::close_x;
    t.tab_widget.tab_font                 = p.default_font;
    t.tab_widget.tab_active_font          = p.bold_font;
    t.tab_widget.scroll_arrow_normal      = p.window_text;
    t.tab_widget.scroll_arrow_hover       = p.window_text;
    t.tab_widget.scroll_arrow_pressed     = p.highlight;
    t.tab_widget.scroll_arrow_disabled    = p.gray_text;
    t.tab_widget.scroll_left_icon         = icon::arrow_left;
    t.tab_widget.scroll_right_icon        = icon::arrow_right;
    t.tab_widget.tab_padding_horizontal   = 16;
    t.tab_widget.tab_padding_vertical     = 8;
    t.tab_widget.tab_spacing              = 4;
    t.tab_widget.close_button_spacing     = 8;
    t.tab_widget.min_tab_width            = 80;
    t.tab_widget.scroll_arrow_width       = 16;

    // ------------------------------------------------------------------
    // List / table
    // ------------------------------------------------------------------
    t.list.item_background                 = p.white;
    t.list.item_background_alt             = p.white;
    t.list.selection_background            = p.highlight;
    t.list.selection_background_inactive   = p.button_shadow;
    t.list.item_foreground                 = p.window_text;
    t.list.selection_foreground            = p.highlight_text;
    t.list.selection_foreground_inactive   = p.window_text;
    t.list.focus_border_color              = p.window_text;
    t.list.focus_box_style                 = box_style{border::flat, false};
    t.list.font                            = p.default_font;
    t.list.padding_horizontal              = 4;
    t.list.padding_vertical                = 2;
    t.list.min_item_height                 = 20;

    // ------------------------------------------------------------------
    // Window (MDI)
    // ------------------------------------------------------------------
    t.window.title_focused.background          = p.highlight;
    t.window.title_focused.foreground          = p.highlight_text;
    t.window.title_focused.mnemonic_foreground = p.highlight_text;
    t.window.title_focused.font                = p.bold_font;

    t.window.title_unfocused.background          = p.white;
    t.window.title_unfocused.foreground          = p.window_text;
    t.window.title_unfocused.mnemonic_foreground = p.window_text;
    t.window.title_unfocused.font                = p.default_font;

    t.window.border_focused         = box_style{border::raised, true};
    t.window.border_unfocused       = box_style{border::raised, true};
    t.window.border_color_focused   = p.button_shadow;
    t.window.border_color_unfocused = p.button_shadow;
    t.window.content_background     = p.window_bg;

    t.window.title_bar_height = 20.0f;
    t.window.border_width     = 4.0f;
    t.window.title_alignment  = horizontal_alignment::center;
    t.window.shadow.enabled   = false;

    return t;
}

} // namespace onyxui::themes
