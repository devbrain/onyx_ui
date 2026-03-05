//
// Backend configuration header for widgets demo
// Defines concrete Backend type based on CMake macro
//

#pragma once

// ============================================================================
// Backend Selection
// ============================================================================

#if defined(ONYXUI_USE_SDLPP_BACKEND)

#include <onyxui/sdlpp/sdlpp_backend.hh>
#include <onyxui/instantiations/all_widgets.hh>

#define ONYXUI_BACKEND onyxui::sdlpp::sdlpp_backend
#include <onyxui/instantiations/extern_widgets.inl>

namespace widgets_demo_config {
    using Backend = onyxui::sdlpp::sdlpp_backend;
}

#elif defined(ONYXUI_USE_CONIO_BACKEND)

#include <onyxui/conio/conio_backend.hh>
#include <onyxui/instantiations/all_widgets.hh>

#define ONYXUI_BACKEND onyxui::conio::conio_backend
#include <onyxui/instantiations/extern_widgets.inl>

namespace widgets_demo_config {
    using Backend = onyxui::conio::conio_backend;
}

#else
#error "Define ONYXUI_USE_CONIO_BACKEND or ONYXUI_USE_SDLPP_BACKEND"
#endif

// ============================================================================
// Convenience alias in global scope
// ============================================================================

using Backend = widgets_demo_config::Backend;
