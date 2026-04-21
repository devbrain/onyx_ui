/**
 * @file sdlpp.hh
 * @brief Per-backend alias header for the sdlpp backend.
 *
 * Populates the canonical `onyxui::ui` widget namespace with
 * sdlpp-backed aliases. Consumers who want widgets without the
 * simple-shell app framework include this header directly.
 *
 * **One backend per translation unit.** Including this header along
 * with `<onyxui/backend/conio.hh>` in the same TU double-populates
 * `onyxui::ui` with conflicting type definitions — compile error.
 * Rare multi-backend code (e.g. cross-backend tests) can reach into
 * `onyxui::sdlpp::sdlpp_backend` / `onyxui::conio::conio_backend`
 * directly and skip the shared `onyxui::ui` alias.
 *
 * Usage:
 *
 * @code
 * #include <onyxui/backend/sdlpp.hh>
 *
 * using namespace onyxui::ui;  // one line, explicit opt-in
 *
 * int main() {
 *     ui_host host;
 *     auto root = std::make_unique<vbox>();
 *     root->emplace_child<label>("hi");
 *     host.mount(std::move(root));
 *     // …
 * }
 * @endcode
 *
 * Consumers who also want the simple-shell types
 * (`app_window`, `run()`, dialog helpers) should include
 * `<onyxui/for/sdlpp.hh>` — that header pulls this one in
 * transitively and adds the app-shell types into `onyxui::ui_app`.
 *
 * See `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.1 for the full
 * backend-pick model.
 */

#pragma once

// Backend definition (sdlpp_backend stays in onyxui::sdlpp as its
// canonical type home; widget aliases live in onyxui::ui below).
#include <onyxui/sdlpp/sdlpp_backend.hh>

// Pull in the canonical widget/type include set once. This keeps the
// alias header and the explicit-instantiation umbrella in sync.
#include <onyxui/detail/type_registry.inc>

namespace onyxui::ui {

    /// Canonical alias for the backend picked by this TU's include
    /// of `<onyxui/backend/sdlpp.hh>`. Consumer code should prefer
    /// `onyxui::ui::backend` over spelling `sdlpp_backend` directly.
    using backend = ::onyxui::sdlpp::sdlpp_backend;

    // Expand the canonical type list into backend-fixed aliases.
    // See `onyx_ui/include/onyxui/detail/public_types.inc` for the
    // master list and `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.1 for
    // the design.
    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::ui
