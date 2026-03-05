/**
 * @file tile_backend.cc
 * @brief Implementation of tile backend global theme management
 */

#include <onyxui/tile/tile_backend.hh>
#include <onyxui/sdlpp/sdlpp_backend.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/theming/theme.hh>
#include <sdlpp/video/texture.hh>
#include <sdlpp/events/events.hh>
#include <stdexcept>

namespace onyxui::tile {

// ============================================================================
// Global Theme Storage
// ============================================================================

namespace {
    // =========================================================================
    // THREAD SAFETY WARNING
    // =========================================================================
    // These global pointers are NOT thread-safe. All tile UI operations must
    // be performed from a single thread (typically the main/render thread).
    //
    // DO NOT:
    //   - Call set_theme/get_theme from multiple threads
    //   - Call set_renderer/get_renderer from multiple threads
    //   - Create/destroy tile widgets from background threads
    //
    // The tile backend is designed for game UIs where single-threaded rendering
    // is the norm. For multi-threaded applications, ensure all UI operations
    // are marshalled to the main thread.
    // =========================================================================

    // Global theme pointer (user-owned, we just store reference)
    // LIFETIME: Must remain valid for entire application duration
    const tile_theme* g_theme = nullptr;

    // Global renderer pointer (set during run_app, cleared on exit)
    // LIFETIME: Valid only within run_app() scope
    tile_renderer* g_renderer = nullptr;

    /**
     * @brief Convert physical pixel coordinates to logical units for tile backend
     * @param physical_x X coordinate in physical pixels
     * @param physical_y Y coordinate in physical pixels
     * @return Pair of logical_unit coordinates
     *
     * @details
     * Tile backend uses its own metrics, which typically have 1:1 mapping
     * since tile-based rendering operates directly in screen pixels.
     * Falls back to 1:1 mapping if no metrics are registered.
     */
    [[nodiscard]] std::pair<logical_unit, logical_unit>
    pixels_to_logical(int physical_x, int physical_y) noexcept {
        auto const* metrics = ui_services<sdlpp_tile_backend>::metrics();
        if (metrics) {
            double const scale_x = metrics->logical_to_physical_x * metrics->dpi_scale;
            double const scale_y = metrics->logical_to_physical_y * metrics->dpi_scale;
            double const logical_x = (scale_x > 0) ? physical_x / scale_x : physical_x;
            double const logical_y = (scale_y > 0) ? physical_y / scale_y : physical_y;
            return {logical_unit(logical_x), logical_unit(logical_y)};
        }
        // Fallback: 1:1 mapping (tile-based default)
        return {logical_unit(static_cast<double>(physical_x)),
                logical_unit(static_cast<double>(physical_y))};
    }

    /**
     * @brief Map SDL keycode to OnyxUI key_code
     */
    [[nodiscard]] key_code map_sdl_keycode(::sdlpp::keycode kc) noexcept {
        namespace KC = ::sdlpp::keycodes;

        // Letters
        if (kc == KC::a) return key_code::a;
        if (kc == KC::b) return key_code::b;
        if (kc == KC::c) return key_code::c;
        if (kc == KC::d) return key_code::d;
        if (kc == KC::e) return key_code::e;
        if (kc == KC::f) return key_code::f;
        if (kc == KC::g) return key_code::g;
        if (kc == KC::h) return key_code::h;
        if (kc == KC::i) return key_code::i;
        if (kc == KC::j) return key_code::j;
        if (kc == KC::k) return key_code::k;
        if (kc == KC::l) return key_code::l;
        if (kc == KC::m) return key_code::m;
        if (kc == KC::n) return key_code::n;
        if (kc == KC::o) return key_code::o;
        if (kc == KC::p) return key_code::p;
        if (kc == KC::q) return key_code::q;
        if (kc == KC::r) return key_code::r;
        if (kc == KC::s) return key_code::s;
        if (kc == KC::t) return key_code::t;
        if (kc == KC::u) return key_code::u;
        if (kc == KC::v) return key_code::v;
        if (kc == KC::w) return key_code::w;
        if (kc == KC::x) return key_code::x;
        if (kc == KC::y) return key_code::y;
        if (kc == KC::z) return key_code::z;

        // Numbers
        if (kc == KC::num_0) return static_cast<key_code>('0');
        if (kc == KC::num_1) return static_cast<key_code>('1');
        if (kc == KC::num_2) return static_cast<key_code>('2');
        if (kc == KC::num_3) return static_cast<key_code>('3');
        if (kc == KC::num_4) return static_cast<key_code>('4');
        if (kc == KC::num_5) return static_cast<key_code>('5');
        if (kc == KC::num_6) return static_cast<key_code>('6');
        if (kc == KC::num_7) return static_cast<key_code>('7');
        if (kc == KC::num_8) return static_cast<key_code>('8');
        if (kc == KC::num_9) return static_cast<key_code>('9');

        // Function keys
        if (kc == KC::f1) return key_code::f1;
        if (kc == KC::f2) return key_code::f2;
        if (kc == KC::f3) return key_code::f3;
        if (kc == KC::f4) return key_code::f4;
        if (kc == KC::f5) return key_code::f5;
        if (kc == KC::f6) return key_code::f6;
        if (kc == KC::f7) return key_code::f7;
        if (kc == KC::f8) return key_code::f8;
        if (kc == KC::f9) return key_code::f9;
        if (kc == KC::f10) return key_code::f10;
        if (kc == KC::f11) return key_code::f11;
        if (kc == KC::f12) return key_code::f12;

        // Navigation
        if (kc == KC::up) return key_code::arrow_up;
        if (kc == KC::down) return key_code::arrow_down;
        if (kc == KC::left) return key_code::arrow_left;
        if (kc == KC::right) return key_code::arrow_right;
        if (kc == KC::home) return key_code::home;
        if (kc == KC::end) return key_code::end;
        if (kc == KC::pageup) return key_code::page_up;
        if (kc == KC::pagedown) return key_code::page_down;

        // Editing
        if (kc == KC::backspace) return key_code::backspace;
        if (kc == KC::delete_key) return key_code::delete_key;
        if (kc == KC::insert) return key_code::insert;
        if (kc == KC::return_key) return key_code::enter;
        if (kc == KC::tab) return key_code::tab;
        if (kc == KC::escape) return key_code::escape;
        if (kc == KC::space) return key_code::space;

        return key_code::none;
    }
}

void set_theme(const tile_theme& theme) {
    g_theme = &theme;
}

const tile_theme& get_theme() {
    if (!g_theme) {
        throw std::runtime_error("No tile theme set. Call tile::set_theme() first.");
    }
    return *g_theme;
}

bool has_theme() noexcept {
    return g_theme != nullptr;
}

void set_renderer(tile_renderer* renderer) {
    g_renderer = renderer;
}

tile_renderer* get_renderer() noexcept {
    return g_renderer;
}

std::optional<ui_event> sdlpp_tile_backend::create_event(
    const onyxui::sdlpp::sdl_event& native) noexcept
{
    // Handle keyboard events
    if (const auto* kbd = native.as<::sdlpp::keyboard_event>()) {
        keyboard_event evt{};
        evt.key = map_sdl_keycode(kbd->key);
        evt.pressed = kbd->down;

        // Build modifiers
        key_modifier mods = key_modifier::none;
        if (::sdlpp::has_keymod(kbd->mod, ::sdlpp::keymod::ctrl)) {
            mods = mods | key_modifier::ctrl;
        }
        if (::sdlpp::has_keymod(kbd->mod, ::sdlpp::keymod::shift)) {
            mods = mods | key_modifier::shift;
        }
        if (::sdlpp::has_keymod(kbd->mod, ::sdlpp::keymod::alt)) {
            mods = mods | key_modifier::alt;
        }
        evt.modifiers = mods;

        return ui_event{evt};
    }

    // Handle mouse button events
    if (const auto* btn = native.as<::sdlpp::mouse_button_event>()) {
        mouse_event evt{};
        // Convert physical pixels to logical units using tile backend metrics
        auto [logical_x, logical_y] = pixels_to_logical(btn->x, btn->y);
        evt.x = logical_x;
        evt.y = logical_y;

        auto mb = btn->get_button();
        if (mb == ::sdlpp::mouse_button::left) {
            evt.btn = mouse_event::button::left;
        } else if (mb == ::sdlpp::mouse_button::right) {
            evt.btn = mouse_event::button::right;
        } else if (mb == ::sdlpp::mouse_button::middle) {
            evt.btn = mouse_event::button::middle;
        } else {
            evt.btn = mouse_event::button::none;
        }

        evt.act = btn->down
            ? mouse_event::action::press
            : mouse_event::action::release;

        return ui_event{evt};
    }

    // Handle mouse motion events
    if (const auto* motion = native.as<::sdlpp::mouse_motion_event>()) {
        mouse_event evt{};
        // Convert physical pixels to logical units using tile backend metrics
        auto [logical_x, logical_y] = pixels_to_logical(motion->x, motion->y);
        evt.x = logical_x;
        evt.y = logical_y;
        evt.btn = mouse_event::button::none;
        evt.act = mouse_event::action::move;
        return ui_event{evt};
    }

    // Handle mouse wheel events
    if (const auto* wheel = native.as<::sdlpp::mouse_wheel_event>()) {
        mouse_event evt{};
        // Convert physical pixels to logical units using tile backend metrics
        auto [logical_x, logical_y] = pixels_to_logical(wheel->mouse_x, wheel->mouse_y);
        evt.x = logical_x;
        evt.y = logical_y;
        evt.btn = mouse_event::button::none;

        if (wheel->y > 0) {
            evt.act = mouse_event::action::wheel_up;
        } else if (wheel->y < 0) {
            evt.act = mouse_event::action::wheel_down;
        } else {
            return std::nullopt;
        }
        return ui_event{evt};
    }

    // Handle window events
    if (const auto* win = native.as<::sdlpp::window_event>()) {
        if (win->type == ::sdlpp::event_type::window_resized) {
            resize_event evt{};
            evt.width = win->data1;
            evt.height = win->data2;
            return ui_event{evt};
        }
    }

    return std::nullopt;
}

void sdlpp_tile_backend::register_themes(
    theme_registry<sdlpp_tile_backend>& registry)
{
    // Create a minimal default theme for tile backend
    // This allows standard widgets to work alongside tile widgets
    ui_theme<sdlpp_tile_backend> default_theme;
    default_theme.name = "Tile Default";
    default_theme.description = "Minimal theme for tile-based rendering";

    // Set default colors using backend color type
    // White on black for readability
    using color = sdlpp_tile_backend::color_type;
    color const white{255, 255, 255, 255};
    color const black{0, 0, 0, 255};
    color const gray{128, 128, 128, 255};
    color const yellow{255, 255, 0, 255};
    color const blue{0, 0, 255, 255};
    color const light_gray{192, 192, 192, 255};
    color const dark_blue{0, 0, 128, 255};

    // Default font - empty path uses system fallback font
    // Size matches typical game UI font (smaller than Windows 3.11 default)
    tile_renderer::font default_font{};
    default_font.size_px = 12.0f;

    tile_renderer::font mnemonic_font{};
    mnemonic_font.size_px = 12.0f;
    mnemonic_font.underline = true;

    // Panel style
    default_theme.panel.background = black;
    default_theme.panel.border_color = gray;

    // Button style (using visual_state bundles)
    // button_style has: normal, hover, pressed, disabled
    default_theme.button.normal.foreground = white;
    default_theme.button.normal.background = gray;
    default_theme.button.normal.mnemonic_foreground = yellow;
    default_theme.button.hover.foreground = white;
    default_theme.button.hover.background = blue;
    default_theme.button.hover.mnemonic_foreground = yellow;
    default_theme.button.pressed.foreground = black;
    default_theme.button.pressed.background = white;
    default_theme.button.pressed.mnemonic_foreground = yellow;
    default_theme.button.disabled.foreground = gray;
    default_theme.button.disabled.background = black;
    default_theme.button.disabled.mnemonic_foreground = gray;

    // Label style (uses 'text' not 'foreground')
    default_theme.label.text = white;
    default_theme.label.background = black;

    // Menu bar style - values in PIXELS (tile backend uses 1:1 scaling)
    default_theme.menu_bar.item_spacing = 0;
    default_theme.menu_bar.item_padding_horizontal = 8;   // 8 pixels
    default_theme.menu_bar.item_padding_vertical = 4;     // 4 pixels

    // Menu bar item style (visual states)
    // Normal state: light background with dark text (like Windows classic menu)
    default_theme.menu_bar_item.normal.foreground = black;
    default_theme.menu_bar_item.normal.background = light_gray;
    default_theme.menu_bar_item.normal.mnemonic_foreground = dark_blue;  // Distinct color for mnemonic

    // Hover state: highlighted
    default_theme.menu_bar_item.hover.foreground = white;
    default_theme.menu_bar_item.hover.background = dark_blue;
    default_theme.menu_bar_item.hover.mnemonic_foreground = yellow;

    // Open state: when menu is expanded
    default_theme.menu_bar_item.open.foreground = white;
    default_theme.menu_bar_item.open.background = dark_blue;
    default_theme.menu_bar_item.open.mnemonic_foreground = yellow;

    // Menu item style (dropdown menu items)
    default_theme.menu_item.normal.foreground = black;
    default_theme.menu_item.normal.background = light_gray;
    default_theme.menu_item.normal.mnemonic_foreground = dark_blue;  // Distinct color for mnemonic

    default_theme.menu_item.highlighted.foreground = white;
    default_theme.menu_item.highlighted.background = dark_blue;
    default_theme.menu_item.highlighted.mnemonic_foreground = yellow;

    default_theme.menu_item.disabled.foreground = gray;
    default_theme.menu_item.disabled.background = light_gray;
    default_theme.menu_item.disabled.mnemonic_foreground = gray;

    default_theme.menu_item.shortcut.foreground = gray;
    default_theme.menu_item.shortcut.background = light_gray;
    default_theme.menu_item.shortcut.mnemonic_foreground = gray;

    default_theme.menu_item.padding_horizontal = 8;   // 8 pixels
    default_theme.menu_item.padding_vertical = 4;    // 4 pixels

    // Menu dropdown style
    default_theme.menu.background = light_gray;
    default_theme.menu.border_color = black;

    // Global palette
    default_theme.window_bg = black;
    default_theme.text_fg = white;
    default_theme.border_color = gray;

    // Register as default (first theme) - use std::move to trigger auto-activation
    registry.register_theme(std::move(default_theme));
}

} // namespace onyxui::tile
