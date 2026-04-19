// Curated list of public OnyxUI types templated solely on Backend.
//
// This file is NOT a standalone header and intentionally has no
// include guard. It is re-included per expansion site with
// `ONYXUI_TYPE` redefined:
//
//   - `<onyxui/backend/sdlpp.hh>` expands each entry to
//     `using name = ::onyxui::name<backend>;` inside the
//     `onyxui::sdlpp` sub-namespace.
//   - `<onyxui/for/sdlpp.hh>` re-exports each entry into
//     `onyxui::simple` via using-declarations.
//   - Conformance tests can redefine `ONYXUI_TYPE` to instantiate
//     every type against a candidate backend.
//
// This file is curated, NOT automatically in sync with the public
// widget set. Adding a new public, single-backend-parameter widget
// means adding one `ONYXUI_TYPE(name)` line here — there is no
// build-time check that catches omissions, so keeping this list in
// sync is part of the PR review checklist for new widgets.
//
// Out of scope for this file:
//   - Types with more than one template parameter (list_model<T, B>,
//     table_model<Row, B>, etc.). Those need per-type consumer
//     typedefs because the extra parameter varies at call sites.
//   - Types in `detail::` / `impl` namespaces.
//   - Deep window chrome internals (`window_title_bar`,
//     `window_content_area`, `window_system_menu`) — consumers
//     rarely instantiate these directly. They can be added later
//     if a real consumer asks.
//   - Retired / retiring types (scoped_ui_context, ui_handle — see
//     WAR-58).
//
// Order: core framework first, then widgets by category, then
// overlays.

// -- Core framework --
ONYXUI_TYPE(ui_element)
ONYXUI_TYPE(ui_host)

// -- Basic widgets --
ONYXUI_TYPE(button)
ONYXUI_TYPE(label)
ONYXUI_TYPE(icon)
ONYXUI_TYPE(spacer)
ONYXUI_TYPE(spring)
ONYXUI_TYPE(status_bar)
ONYXUI_TYPE(text_view)
ONYXUI_TYPE(progress_bar)
ONYXUI_TYPE(separator)

// -- Containers --
ONYXUI_TYPE(panel)
ONYXUI_TYPE(vbox)
ONYXUI_TYPE(hbox)
ONYXUI_TYPE(grid)
ONYXUI_TYPE(group_box)
ONYXUI_TYPE(tab_widget)
ONYXUI_TYPE(stack_panel)
ONYXUI_TYPE(anchor_panel)
ONYXUI_TYPE(absolute_panel)

// -- Scrolling --
ONYXUI_TYPE(scroll_view)
ONYXUI_TYPE(scrollable)
ONYXUI_TYPE(scrollbar)
ONYXUI_TYPE(scroll_controller)

// -- Input widgets --
ONYXUI_TYPE(checkbox)
ONYXUI_TYPE(radio_button)
ONYXUI_TYPE(slider)
ONYXUI_TYPE(line_edit)
ONYXUI_TYPE(combo_box)
ONYXUI_TYPE(list_box)
ONYXUI_TYPE(button_group)

// -- Menu system --
ONYXUI_TYPE(menu)
ONYXUI_TYPE(menu_bar)
ONYXUI_TYPE(menu_bar_item)
ONYXUI_TYPE(menu_item)
ONYXUI_TYPE(menu_system)

// -- Main window chrome (tree-hosted) --
ONYXUI_TYPE(main_window)

// -- Overlay windows & presentation --
ONYXUI_TYPE(window)
ONYXUI_TYPE(dialog)
ONYXUI_TYPE(window_list_dialog)
ONYXUI_TYPE(presented_window)
