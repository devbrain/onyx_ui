/**
 * @file sdlpp.hh
 * @brief Simple-shell bundle header — sdlpp backend.
 *
 * Standalone-tool consumers include this one header and get both the
 * widget alias pack (`onyxui::sdlpp::*`) and the FLTK-grade app-shell
 * types layered into `onyxui::ui_app::*`. See
 * `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.2 for the full model.
 *
 * Namespace contract:
 *   - `onyxui::sdlpp::*` — widget aliases (ODR-safe, backend in the name).
 *     Populated by `<onyxui/backend/sdlpp.hh>`.
 *   - `onyxui::ui_app::*` — widget aliases re-exported + `app_window`,
 *     `run()`, dialog helpers. Populated by this header. Intended for
 *     source-file-level `using`; avoid in public library headers (same
 *     TU-local-backend caveat as `onyxui::ui::*`).
 *
 * Widget-only consumers (engine embedders) can include
 * `<onyxui/backend/sdlpp.hh>` directly and skip the app shell.
 * Consumers who want the uniform `onyxui::ui::*` spelling should
 * additionally include `<onyxui/ui.hh>` with
 * `ONYXUI_UI_BACKEND=::onyxui::sdlpp` set — see that header.
 *
 * Header order matters:
 *   1. Include `<onyxui/backend/sdlpp.hh>` so widget aliases populate
 *      `onyxui::sdlpp` as concrete types.
 *   2. Re-export them into `onyxui::ui_app` via X-macro using-
 *      declarations so the simple/* headers can reference them
 *      unqualified.
 *   3. Include the `<onyxui/ui_app/*.hh>` headers — they reference
 *      the unqualified names `ui_host`, `ui_element`, `window` in
 *      `onyxui::ui_app`, which step 2 just populated.
 */

#pragma once

// Signal to the ui_app/* headers that the bundle is live (so the
// `onyxui::ui_app` aliases are populated before they're parsed).
// Each ui_app/* header #errors without this define.
#define ONYXUI_UI_APP_BUNDLE_INCLUDED 1

// 1. Widget aliases → onyxui::sdlpp (canonical, ODR-safe).
#include <onyxui/backend/sdlpp.hh>

// 2. Re-export them into onyxui::ui_app so the app-shell headers can
//    reference `ui_host`, `window`, etc. without qualifying. This
//    namespace is TU-local (its meaning depends on which bundle
//    header was included); do not re-export `onyxui::ui_app::*`
//    from library public headers.
namespace onyxui::ui_app {
    using ::onyxui::sdlpp::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::sdlpp::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::ui_app

// 3. App-shell types live in onyxui::ui_app and reference the widgets
//    re-exported above.
#include <onyxui/ui_app/app_window.hh>
#include <onyxui/ui_app/run.hh>
#include <onyxui/ui_app/dialogs.hh>
