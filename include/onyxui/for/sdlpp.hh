/**
 * @file sdlpp.hh
 * @brief Simple-shell bundle header — sdlpp backend.
 *
 * Consumers writing standalone tools include exactly this one header
 * and get both the widget alias pack (`onyxui::ui`) and the FLTK-grade
 * app-shell types (`onyxui::ui_app`). See
 * `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.2 for the full model.
 *
 * Namespace contract:
 *   - `onyxui::ui::*`     — widget aliases (backend-fixed). Populated
 *                            by `<onyxui/backend/sdlpp.hh>`.
 *   - `onyxui::ui_app::*` — widget aliases re-exported *plus*
 *                            `app_window`, `run()`, dialog helpers.
 *                            Populated by this header.
 *
 * Widget-only consumers (engine embedders) can include just
 * `<onyxui/backend/sdlpp.hh>` and skip the app shell.
 *
 * Header order matters:
 *   1. Include `<onyxui/backend/sdlpp.hh>` so widget aliases populate
 *      `onyxui::ui` as concrete types.
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
// Each ui_app/* header #errors without this define, producing a
// readable diagnostic instead of a cascade of "unknown type" errors.
#define ONYXUI_UI_APP_BUNDLE_INCLUDED 1

// 1. Widget aliases → onyxui::ui.
#include <onyxui/backend/sdlpp.hh>

// 2. Re-export them into onyxui::ui_app so the app-shell headers can
//    reference `ui_host`, `window`, etc. without qualifying.
namespace onyxui::ui_app {
    using ::onyxui::ui::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::ui::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::ui_app

// 3. App-shell types live in onyxui::ui_app and reference the widgets
//    re-exported above.
#include <onyxui/ui_app/app_window.hh>
#include <onyxui/ui_app/run.hh>
#include <onyxui/ui_app/dialogs.hh>
