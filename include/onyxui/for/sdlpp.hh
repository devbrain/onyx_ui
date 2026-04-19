/**
 * @file sdlpp.hh
 * @brief Simple-shell bundle header — sdlpp backend.
 *
 * Consumers writing standalone tools include exactly this one header
 * plus `using namespace onyxui::simple;` and get the FLTK-grade API
 * — no backend template parameters visible. See
 * `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.2 for the full model.
 *
 * Header order matters, per §5.2:
 *
 *   1. Include the backend-alias header so `onyxui::sdlpp::ui_host`
 *      etc. exist as concrete types.
 *   2. Promote those aliases into `onyxui::simple` via X-macro
 *      using-declarations.
 *   3. Only now include the `<onyxui/simple/*.hh>` headers — they
 *      reference the unqualified names `ui_host` and `ui_element`
 *      in `onyxui::simple`, which the step above just populated.
 */

#pragma once

// 1. Backend-fixed aliases under onyxui::sdlpp::.
#include <onyxui/backend/sdlpp.hh>

// 2. Re-export them into onyxui::simple BEFORE the simple/* headers
//    are parsed. Using-declarations (not a using-directive) so each
//    name is a first-class introduction in the target namespace.
namespace onyxui::simple {
    using ::onyxui::sdlpp::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::sdlpp::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::simple

// 3. Now the simple/* headers can be parsed — ui_host / ui_element
//    are visible as concrete backend-fixed types in onyxui::simple.
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/run.hh>
#include <onyxui/simple/dialogs.hh>
