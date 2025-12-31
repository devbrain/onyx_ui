//
// Created by igor on 21/10/2025.
//
#include <onyxui/conio/conio_backend.hh>
#include "conio_themes.hh"
#include <onyxui/theming/theme_registry.hh>  // for theme_registry
#include <termbox2.h>

namespace onyxui::conio {

// ============================================================================
// Static State for Platform Integration
// ============================================================================

namespace {

// Global quit flag for application exit
bool g_quit_requested = false;

// Termbox initialization state
bool g_termbox_initialized = false;

} // anonymous namespace

// ============================================================================
// Theme Registration
// ============================================================================

void conio_backend::register_themes(theme_registry<conio_backend>& registry) {
    conio_themes::register_themes(registry);
}

// ============================================================================
// Platform Integration Methods
// ============================================================================

bool conio_backend::init() {
    if (g_termbox_initialized) {
        return true;  // Already initialized by us
    }

    // Check if termbox is already initialized (e.g., by vram)
    // tb_width() returns -1 if not initialized
    if (tb_width() > 0) {
        g_termbox_initialized = true;
        g_quit_requested = false;
        return true;
    }

    const int result = tb_init();
    if (result != TB_OK) {
        return false;
    }

    // Enable mouse support
    tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

    g_termbox_initialized = true;
    g_quit_requested = false;
    return true;
}

void conio_backend::shutdown() {
    if (g_termbox_initialized) {
        tb_shutdown();
        g_termbox_initialized = false;
    }
}

bool conio_backend::should_quit() noexcept {
    return g_quit_requested;
}

void conio_backend::clear_quit_flag() noexcept {
    g_quit_requested = false;
}

void conio_backend::request_quit() noexcept {
    g_quit_requested = true;
}

// ======================================================================
// Termbox2 Wrapper Implementations
// ======================================================================
// These wrappers are properly exported for use by applications linking
// to the conio backend as a shared library.

int conio_get_width() {
    return tb_width();
}

int conio_get_height() {
    return tb_height();
}

int conio_poll_event(tb_event* event) {
    return tb_poll_event(event);
}

} // namespace onyxui::conio