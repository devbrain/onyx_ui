/**
 * @file ui.hh
 * @brief Unified `onyxui::ui` widget namespace — program-level backend pick.
 *
 * `onyxui::ui::button` etc. become aliases for **one** backend's
 * widget types. Which backend is the consumer's choice, expressed
 * via a single preprocessor macro:
 *
 *   ONYXUI_UI_BACKEND — fully-qualified namespace containing the
 *                       backend's widget aliases (`backend`,
 *                       `button`, `vbox`, …). Any namespace
 *                       populated by the standard X-macro over
 *                       `<onyxui/detail/public_types.inc>` works.
 *
 * ## Typical use — first-party backend
 *
 * Project-level `backend_config.hh`:
 * @code
 * #include <onyxui/backend/sdlpp.hh>          // populates onyxui::sdlpp::*
 * #define ONYXUI_UI_BACKEND ::onyxui::sdlpp
 * #include <onyxui/ui.hh>                     // populates onyxui::ui::*
 * @endcode
 *
 * Or set `ONYXUI_UI_BACKEND` via CMake for the whole target:
 * @code
 * target_compile_definitions(mytarget PRIVATE
 *     ONYXUI_UI_BACKEND=::onyxui::sdlpp)
 * @endcode
 *
 * ## Third-party backends
 *
 * A consumer-built backend works the same way. Supply a namespace
 * that carries `backend` plus the standard widget aliases (the
 * X-macro expansion handles both), then point this header at it:
 * @code
 * #include <my_engine/backend.hh>         // populates ::my::engine
 * #define ONYXUI_UI_BACKEND ::my::engine
 * #include <onyxui/ui.hh>
 * @endcode
 *
 * No edits to onyx_ui itself are required.
 *
 * ## Contract
 *
 *   - Each target defines `ONYXUI_UI_BACKEND` exactly once, to the
 *     same value, in every translation unit of that target.
 *   - The backend's header (which populates the target namespace)
 *     must be included **before** this header.
 *   - Library authors who need to expose widget types from public
 *     headers should use the backend's canonical namespace
 *     (`onyxui::sdlpp::button`, etc.) or the template form
 *     (`onyxui::button<B>`), not `onyxui::ui::button`. The latter
 *     is TU-local by construction — its meaning depends on which
 *     `ONYXUI_UI_BACKEND` was active when the TU was compiled.
 *
 * Defining `ONYXUI_UI_BACKEND` to two different values across TUs in
 * the same binary is an ODR violation; keep it build-wide.
 */

#pragma once

#ifndef ONYXUI_UI_BACKEND
#  error "<onyxui/ui.hh>: ONYXUI_UI_BACKEND is not defined. Set it " \
         "to the fully-qualified namespace that carries your " \
         "backend's widget aliases (e.g. ::onyxui::sdlpp) — either " \
         "via target_compile_definitions() or by #defining it in a " \
         "project-level config header right before this include."
#endif

namespace onyxui::ui {

    /// Canonical alias for the program's chosen backend type.
    using backend = ONYXUI_UI_BACKEND::backend;

    // Mirror the backend's widget aliases into onyxui::ui. The
    // backend must already have been included; this header relies on
    // the consumer having pulled in the appropriate
    // <onyxui/backend/*.hh> (or third-party equivalent) first so the
    // source names resolve.
    #define ONYXUI_TYPE(name) using name = ONYXUI_UI_BACKEND::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::ui
