//
// Created by igor on 21/10/2025.
//
#include <onyxui/conio/conio_backend.hh>
#include "conio_themes.hh"

namespace onyxui::conio {
    void conio_backend::register_themes(theme_registry<conio_backend>& registry) {
        conio_themes::register_themes(registry);
    }
}