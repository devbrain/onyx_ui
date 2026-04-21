/**
 * @file conio.hh
 * @brief Simple-shell bundle header — conio backend.
 *
 * Mirror of `<onyxui/for/sdlpp.hh>` for the conio (terminal) backend.
 * See that header's comment for the full mechanism.
 *
 * Consumer usage (identical to the sdlpp path — only this one
 * include line differs):
 *
 * @code
 * #include <onyxui/for/conio.hh>
 *
 * int main() {
 *     using namespace onyxui::ui_app;
 *     app_window win("Hello", 80, 25);
 *     auto root = std::make_unique<vbox>();
 *     root->emplace_child<label>("Hello, terminal!");
 *     win.set_content(std::move(root));
 *     win.show();
 *     return run();
 * }
 * @endcode
 */

#pragma once

// Signal to the ui_app/* headers that the bundle chain is in use —
// see the note on ONYXUI_UI_APP_BUNDLE_INCLUDED in
// `<onyxui/for/sdlpp.hh>`.
#define ONYXUI_UI_APP_BUNDLE_INCLUDED 1

// 1. Widget aliases → onyxui::conio (canonical, ODR-safe).
#include <onyxui/backend/conio.hh>

// 2. Re-export them into onyxui::ui_app so the app-shell headers can
//    reference `ui_host`, `window`, etc. without qualifying.
namespace onyxui::ui_app {
    using ::onyxui::conio::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::conio::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::ui_app

// 3. App-shell types live in onyxui::ui_app and reference the widgets
//    re-exported above.
#include <onyxui/ui_app/app_window.hh>
#include <onyxui/ui_app/run.hh>
#include <onyxui/ui_app/dialogs.hh>
