// Canonical list of public OnyxUI types templated on Backend.
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
//   - Conformance tests can redefine `ONYXUI_TYPE` to
//     instantiate every type against a candidate backend.
//
// Adding a new public, single-backend-parameter type: add one
// `ONYXUI_TYPE(name)` line here. Removing a type: delete the line
// and audit any consumer code that referenced the alias.
//
// Out of scope for this file:
//   - Types with more than one template parameter (list_model<T, B>,
//     table_model<Row, B>, etc.). Those need per-type consumer
//     typedefs because the extra parameter varies at call sites.
//   - Types in `detail::` / `impl` namespaces.
//   - Retired / retiring types (scoped_ui_context, ui_handle — see
//     WAR-58).
//
// Order: core framework first, then widgets by category, then
// overlays. Internal helpers last.

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
ONYXUI_TYPE(menu_item)

// -- Main window chrome (tree-hosted) --
ONYXUI_TYPE(main_window)

// -- Overlay windows & presentation --
ONYXUI_TYPE(window)
ONYXUI_TYPE(dialog)
ONYXUI_TYPE(presented_window)
