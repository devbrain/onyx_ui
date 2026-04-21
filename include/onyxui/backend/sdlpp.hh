/**
 * @file sdlpp.hh
 * @brief Per-backend alias header for the sdlpp backend.
 *
 * Consumers who want backend-fixed aliases without committing to
 * the simple-shell app framework include this header. Aliases live
 * in `namespace onyxui::sdlpp` (alongside `sdlpp_backend` itself)
 * so they don't collide with the primary `onyxui::button<B>` etc.
 * templates.
 *
 * Usage:
 *
 * @code
 * #include <onyxui/backend/sdlpp.hh>
 *
 * using namespace onyxui::sdlpp;  // one line, explicit opt-in
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
 * `<onyxui/for/sdlpp.hh>` instead — that header pulls this one in
 * transitively and re-exports the aliases into `onyxui::simple`.
 *
 * See `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.1 for the full
 * backend-pick model.
 */

#pragma once

// Backend definition (sdlpp_backend is in namespace onyxui::sdlpp).
#include <onyxui/sdlpp/sdlpp_backend.hh>

// Pull in the canonical widget/type include set once. This keeps the
// alias header and the explicit-instantiation umbrella in sync.
#include <onyxui/detail/type_registry.inc>

namespace onyxui::sdlpp {

    /// Canonical name for the backend fixed by this shell header.
    /// Prefer this to spelling `sdlpp_backend` directly — it keeps
    /// consumer code swappable with `<onyxui/backend/conio.hh>`.
    using backend = sdlpp_backend;

    // Expand the canonical type list into backend-fixed aliases.
    // See `onyx_ui/include/onyxui/detail/public_types.inc` for the
    // master list and `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.1 for
    // the design.
    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::sdlpp
