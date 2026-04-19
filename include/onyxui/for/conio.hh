/**
 * @file conio.hh
 * @brief Simple-shell bundle header — conio backend.
 *
 * Mirror of `<onyxui/for/sdlpp.hh>` for the conio (terminal) backend.
 * See that header's comment for the full mechanism; the short version
 * is:
 *
 *   1. Pull in the backend-alias header so `onyxui::conio::ui_host`
 *      etc. exist as concrete types.
 *   2. Promote those aliases into `onyxui::simple` via X-macro
 *      using-declarations.
 *   3. Include the `<onyxui/simple/*.hh>` headers — they reference
 *      the unqualified names in `onyxui::simple`, which step 2
 *      just populated.
 *
 * Consumer usage (identical to the sdlpp path — only this one
 * include line differs):
 *
 * @code
 * #include <onyxui/for/conio.hh>
 *
 * int main() {
 *     using namespace onyxui::simple;
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

// Signal to the simple/* headers that the bundle chain is in use —
// see the note on ONYXUI_SIMPLE_BUNDLE_INCLUDED in
// `<onyxui/for/sdlpp.hh>`.
#define ONYXUI_SIMPLE_BUNDLE_INCLUDED 1

// 1. Backend-fixed aliases under onyxui::conio::.
#include <onyxui/backend/conio.hh>

// 2. Re-export them into onyxui::simple BEFORE the simple/* headers
//    are parsed. Using-declarations (not a using-directive) so each
//    name is a first-class introduction in the target namespace.
namespace onyxui::simple {
    using ::onyxui::conio::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::conio::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::simple

// 3. Now the simple/* headers can be parsed — ui_host / ui_element /
//    window / etc. are visible as concrete backend-fixed types in
//    onyxui::simple.
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/run.hh>
#include <onyxui/simple/dialogs.hh>
