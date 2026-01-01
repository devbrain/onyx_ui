#include <onyxui/sdlpp/sdlpp_backend.hh>
#include "sdlpp_themes.hh"

#include <sdlpp/events/events.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui::sdlpp {

// ============================================================================
// Static State for Platform Integration
// ============================================================================

namespace {

// Global quit flag for application exit
bool g_quit_requested = false;

// Pointer to external renderer (game engine mode)
::sdlpp::renderer* g_external_renderer = nullptr;

// Map SDL keycode to OnyxUI key_code
[[nodiscard]] key_code map_sdl_keycode(::sdlpp::keycode kc) noexcept
{
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

    // Numbers (return as ASCII char values)
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

/**
 * @brief Convert physical pixel coordinates to logical units
 * @param physical_x X coordinate in physical pixels
 * @param physical_y Y coordinate in physical pixels
 * @return Pair of logical_unit coordinates
 *
 * @details
 * SDL provides mouse coordinates in physical screen pixels.
 * Hit testing uses logical coordinates. This function converts
 * using the backend metrics (1 logical unit = 8 pixels for SDL).
 */
[[nodiscard]] std::pair<logical_unit, logical_unit> pixels_to_logical(int physical_x, int physical_y) {
    auto const* metrics = ui_services<sdlpp_backend>::metrics();
    if (metrics) {
        double const scale_x = metrics->logical_to_physical_x * metrics->dpi_scale;
        double const scale_y = metrics->logical_to_physical_y * metrics->dpi_scale;
        double const logical_x = (scale_x > 0) ? physical_x / scale_x : physical_x;
        double const logical_y = (scale_y > 0) ? physical_y / scale_y : physical_y;
        return {logical_unit(logical_x), logical_unit(logical_y)};
    }
    // Fallback: 1:1 mapping
    return {logical_unit(static_cast<double>(physical_x)),
            logical_unit(static_cast<double>(physical_y))};
}

} // anonymous namespace

std::optional<ui_event> sdlpp_backend::create_event(
    const sdl_event& native) noexcept
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
        // Convert physical pixels to logical units for hit testing
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
        // Convert physical pixels to logical units for hit testing
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
        // Convert physical pixels to logical units for hit testing
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

void sdlpp_backend::register_themes(
    theme_registry<sdlpp_backend>& registry)
{
    registry.register_theme(create_windows311_theme());
}

// ============================================================================
// Platform Integration Methods
// ============================================================================

bool sdlpp_backend::init([[maybe_unused]] ::sdlpp::renderer& renderer)
{
    g_external_renderer = &renderer;
    g_quit_requested = false;
    return true;
}

void sdlpp_backend::shutdown()
{
    // Clear font cache before SDL shuts down
    sdlpp_renderer::shutdown();
    g_external_renderer = nullptr;
}

std::optional<ui_event> sdlpp_backend::process_event(const ::sdlpp::event& event)
{
    // Check for quit event
    if (event.is<::sdlpp::quit_event>()) {
        g_quit_requested = true;
        return std::nullopt;
    }

    // sdl_event is an alias for ::sdlpp::event, so just pass directly
    return create_event(event);
}

bool sdlpp_backend::should_quit() noexcept
{
    return g_quit_requested;
}

void sdlpp_backend::clear_quit_flag() noexcept
{
    g_quit_requested = false;
}

} // namespace onyxui::sdlpp
