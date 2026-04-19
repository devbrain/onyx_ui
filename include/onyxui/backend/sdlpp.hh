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

// Core / host.
#include <onyxui/core/element.hh>
#include <onyxui/ui_host.hh>

// Widgets and containers pulled in by the public_types.inc list.
// (Transitive includes in the host type aren't enough to instantiate
// every alias — each type needs its full definition in scope.)
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/icon.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/progress_bar.hh>
#include <onyxui/widgets/main_window.hh>

#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/grid.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <onyxui/widgets/containers/stack_panel.hh>
#include <onyxui/widgets/containers/anchor_panel.hh>
#include <onyxui/widgets/containers/absolute_panel.hh>
#include <onyxui/widgets/containers/scroll_view.hh>

#include <onyxui/widgets/containers/scroll/scrollable.hh>
#include <onyxui/widgets/containers/scroll/scrollbar.hh>
#include <onyxui/widgets/containers/scroll/scroll_controller.hh>

#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>

#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/slider.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/widgets/input/list_box.hh>
#include <onyxui/widgets/input/button_group.hh>

#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu_bar_item.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/menu/menu_system.hh>
#include <onyxui/widgets/menu/separator.hh>

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/window/window_list_dialog.hh>
#include <onyxui/widgets/window/presented_window.hh>

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
