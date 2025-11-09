//
// Created by igor on 21/10/2025.
//
#include <onyxui/conio/conio_backend.hh>
#include "conio_themes.hh"
#include <onyxui/theming/theme_registry.hh>  // for theme_registry
#include <termbox2.h>

namespace onyxui::conio {
    void conio_backend::register_themes(theme_registry<conio_backend>& registry) {
        conio_themes::register_themes(registry);
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
}