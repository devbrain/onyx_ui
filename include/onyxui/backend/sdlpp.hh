/**
 * @file sdlpp.hh
 * @brief Per-backend alias header for the sdlpp backend.
 *
 * Populates the `onyxui::sdlpp` namespace with backend-fixed widget
 * aliases (`onyxui::sdlpp::button`, `onyxui::sdlpp::vbox`, …). Because
 * the namespace name encodes the backend, references to these types
 * in public library headers are **ODR-safe**: no other backend header
 * can ever populate `onyxui::sdlpp::button` with a different type.
 *
 * Usage (library authors — safe in public headers):
 *
 * @code
 * #include <onyxui/backend/sdlpp.hh>
 *
 * void do_something(onyxui::sdlpp::button* b);  // ABI-stable
 * @endcode
 *
 * Consumer code that wants a backend-agnostic `onyxui::ui::*`
 * spelling should include `<onyxui/ui.hh>` after setting
 * `ONYXUI_BACKEND_SDLPP` at the CMake target level — see that
 * header for the rationale.
 *
 * Consumer code that also wants the FLTK-grade simple shell
 * (`app_window`, `run()`, dialog helpers) should include
 * `<onyxui/for/sdlpp.hh>` — that bundle pulls this header in
 * transitively and layers the app-shell types on top as
 * `onyxui::ui_app::*`.
 *
 * See `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.1 for the full
 * backend-pick model.
 */

#pragma once

// Backend definition: `sdlpp_backend` lives in `onyxui::sdlpp` and is
// the canonical type. Widget aliases below share that namespace.
#include <onyxui/sdlpp/sdlpp_backend.hh>

// Pull in the canonical widget/type include set once. This keeps the
// alias header and the explicit-instantiation umbrella in sync.
#include <onyxui/detail/type_registry.inc>

namespace onyxui::sdlpp {

    /// Canonical alias for the backend introduced by this header.
    /// Prefer `onyxui::sdlpp::backend` to spelling `sdlpp_backend`
    /// directly — keeps consumer code swappable with conio.
    using backend = sdlpp_backend;

    // Expand the canonical type list into backend-fixed aliases.
    // See `include/onyxui/detail/public_types.inc` for the master
    // list and `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.1 for the
    // design.
    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::sdlpp
