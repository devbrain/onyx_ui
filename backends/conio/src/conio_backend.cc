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
    // Note: Termbox lifecycle is owned by vram (RAII via vram::impl constructor/destructor).
    // This method just tracks whether a vram instance exists and is usable.
    // Do NOT call tb_init() here - that's vram's responsibility.

    if (g_termbox_initialized) {
        return true;  // Already initialized
    }

    // Check if termbox is already initialized (by vram)
    // tb_width() returns -1 if not initialized
    if (tb_width() > 0) {
        g_termbox_initialized = true;
        g_quit_requested = false;
        return true;
    }

    // Not initialized - caller should create a conio_renderer (which owns vram)
    return false;
}

void conio_backend::shutdown() {
    // Note: Termbox shutdown is owned by vram destructor (RAII).
    // This method just clears our tracking state.
    // Do NOT call tb_shutdown() here - that's vram's responsibility.
    g_termbox_initialized = false;
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

// =============================================================================
// Explicit Template Instantiation
// =============================================================================
// Instantiate all widget templates for conio_backend to reduce compile times.
// Users linking against this library get pre-compiled widget code.

#include <onyxui/instantiations/all_widgets.hh>
#include <onyxui/backends/conio/onyxui_conio_export.h>

#define ONYXUI_BACKEND onyxui::conio::conio_backend
#define ONYXUI_EXPORT ONYXUI_CONIO_EXPORT
#include <onyxui/instantiations/instantiate_widgets.inl>