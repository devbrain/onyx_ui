// Canonical OnyxUI type registry.
//
// This file is intentionally include-guard free so it can be
// re-included with different expansion macros. It is the single
// source of truth for:
//
//   - backend-fixed alias surfaces (`public_types.inc`)
//   - explicit-instantiation surfaces (`widget_list.hh`)
//   - the umbrella include set used by backend alias headers and
//     explicit-instantiation convenience headers (`all_widgets.hh`)
//
// Consumers may define `ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES` before
// including this file to suppress the `#include` block and use only
// the registry macros.

#ifndef ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES

// =============================================================================
// Core Framework
// =============================================================================

#include <onyxui/core/element.hh>
#include <onyxui/ui_host.hh>
#include <onyxui/services/ui_services.hh>

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/widgets/core/stateful_widget.hh>

// =============================================================================
// Basic Widgets
// =============================================================================

#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/icon.hh>
#include <onyxui/widgets/styled_text.hh>
#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>
#include <onyxui/widgets/progress_bar.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/widgets/text_view.hh>

// =============================================================================
// Container Widgets
// =============================================================================

#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/grid.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/anchor_panel.hh>
#include <onyxui/widgets/containers/absolute_panel.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <onyxui/widgets/containers/stack_panel.hh>

// =============================================================================
// Scrolling Widgets
// =============================================================================

#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/containers/scroll/scrollable.hh>
#include <onyxui/widgets/containers/scroll/scrollbar.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_thumb.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_arrow.hh>
#include <onyxui/widgets/containers/scroll/scroll_controller.hh>

// =============================================================================
// Input Widgets
// =============================================================================

#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/slider.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/widgets/input/list_box.hh>
#include <onyxui/widgets/input/button_group.hh>

// =============================================================================
// MVC Views
// =============================================================================

#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/mvc/views/combo_box_view.hh>
#include <onyxui/mvc/views/table_view.hh>
#include <onyxui/mvc/delegates/default_item_delegate.hh>

// =============================================================================
// Menu Widgets
// =============================================================================

#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/menu/menu_bar_item.hh>
#include <onyxui/widgets/menu/separator.hh>
#include <onyxui/widgets/menu/menu_system.hh>

// =============================================================================
// Window Widgets
// =============================================================================

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/main_window.hh>
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/window/window_content_area.hh>
#include <onyxui/widgets/window/window_list_dialog.hh>
#include <onyxui/widgets/window/presented_window.hh>

// =============================================================================
// Layout Strategies
// =============================================================================

#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/layout/grid_layout.hh>
#include <onyxui/layout/anchor_layout.hh>
#include <onyxui/layout/absolute_layout.hh>

// =============================================================================
// Rendering Contexts
// =============================================================================

#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/core/rendering/measure_context.hh>

// =============================================================================
// Actions (Command pattern)
// =============================================================================

#include <onyxui/actions/action.hh>
#include <onyxui/actions/action_group.hh>

#endif  // ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES

// =============================================================================
// Registry Macros
// =============================================================================
//
// Each registry macro accepts three callbacks, one per bucket:
//
//   1. alias-only        -> types exposed as backend-fixed aliases only.
//                           Typical when the class has backend-specific
//                           constructors that prevent explicit instantiation
//                           in a backend-agnostic TU (e.g. `ui_host`).
//   2. instantiate-only  -> types explicitly instantiated into the backend
//                           library but not part of the end-user alias
//                           surface. Internal chrome (scrollbar parts,
//                           window title bar / content area).
//   3. alias+instantiate -> types in both surfaces. The common case for
//                           end-user-visible widgets and base classes.
//
// Out of scope (deliberately NOT in the registry at all):
//
//   - Types with more than one template parameter that varies at call
//     sites (e.g. `list_model<T, Backend>`, `table_model<Row, Backend>`).
//     These need per-consumer typedefs because the non-Backend parameter
//     can't be baked into a backend-fixed alias.
//   - Types in `onyxui::detail::` or `impl` namespaces.
//   - Retired types moved to `onyxui::detail::` under WAR-58
//     (`scoped_ui_context`, legacy `ui_handle`). Use `ui_host<Backend>`
//     instead.
//
// When adding a new widget, pick the bucket based on its role. A public
// single-backend-parameter widget almost always wants ALIAS_AND_INSTANTIATE.

#define ONYXUI_TYPE_REGISTRY_CORE(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, ui_element) \
    ALIAS_ONLY(onyxui, ui_host) \
    ALIAS_ONLY(onyxui, ui_services) \
    ALIAS_AND_INSTANTIATE(onyxui, widget) \
    ALIAS_AND_INSTANTIATE(onyxui, widget_container) \
    ALIAS_AND_INSTANTIATE(onyxui, stateful_widget)

#define ONYXUI_TYPE_REGISTRY_BASIC(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, button) \
    ALIAS_AND_INSTANTIATE(onyxui, label) \
    ALIAS_AND_INSTANTIATE(onyxui, icon) \
    ALIAS_AND_INSTANTIATE(onyxui, spacer) \
    ALIAS_AND_INSTANTIATE(onyxui, spring) \
    ALIAS_AND_INSTANTIATE(onyxui, progress_bar) \
    ALIAS_AND_INSTANTIATE(onyxui, status_bar)

#define ONYXUI_TYPE_REGISTRY_CONTAINERS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, panel) \
    ALIAS_AND_INSTANTIATE(onyxui, vbox) \
    ALIAS_AND_INSTANTIATE(onyxui, hbox) \
    ALIAS_AND_INSTANTIATE(onyxui, grid) \
    ALIAS_AND_INSTANTIATE(onyxui, group_box) \
    ALIAS_AND_INSTANTIATE(onyxui, anchor_panel) \
    ALIAS_AND_INSTANTIATE(onyxui, absolute_panel) \
    ALIAS_AND_INSTANTIATE(onyxui, tab_widget) \
    ALIAS_AND_INSTANTIATE(onyxui, stack_panel)

#define ONYXUI_TYPE_REGISTRY_SCROLLING(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, scroll_view) \
    ALIAS_AND_INSTANTIATE(onyxui, scrollable) \
    ALIAS_AND_INSTANTIATE(onyxui, scrollbar) \
    INSTANTIATE_ONLY(onyxui, scrollbar_thumb) \
    INSTANTIATE_ONLY(onyxui, scrollbar_arrow) \
    ALIAS_AND_INSTANTIATE(onyxui, scroll_controller)

#define ONYXUI_TYPE_REGISTRY_INPUT(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, checkbox) \
    ALIAS_AND_INSTANTIATE(onyxui, radio_button) \
    ALIAS_AND_INSTANTIATE(onyxui, slider) \
    ALIAS_AND_INSTANTIATE(onyxui, line_edit) \
    ALIAS_AND_INSTANTIATE(onyxui, text_view) \
    ALIAS_AND_INSTANTIATE(onyxui, combo_box) \
    ALIAS_AND_INSTANTIATE(onyxui, list_box) \
    ALIAS_AND_INSTANTIATE(onyxui, button_group)

#define ONYXUI_TYPE_REGISTRY_MVC(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, list_view) \
    ALIAS_AND_INSTANTIATE(onyxui, combo_box_view) \
    ALIAS_AND_INSTANTIATE(onyxui, table_view) \
    ALIAS_AND_INSTANTIATE(onyxui, default_item_delegate)

#define ONYXUI_TYPE_REGISTRY_MENU(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, menu_bar) \
    ALIAS_AND_INSTANTIATE(onyxui, menu) \
    ALIAS_AND_INSTANTIATE(onyxui, menu_item) \
    ALIAS_AND_INSTANTIATE(onyxui, menu_bar_item) \
    ALIAS_AND_INSTANTIATE(onyxui, separator) \
    ALIAS_AND_INSTANTIATE(onyxui, menu_system)

#define ONYXUI_TYPE_REGISTRY_WINDOWS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, window) \
    ALIAS_AND_INSTANTIATE(onyxui, dialog) \
    ALIAS_AND_INSTANTIATE(onyxui, main_window) \
    INSTANTIATE_ONLY(onyxui, window_title_bar) \
    INSTANTIATE_ONLY(onyxui, window_content_area) \
    ALIAS_ONLY(onyxui, window_list_dialog) \
    ALIAS_ONLY(onyxui, presented_window)

#define ONYXUI_TYPE_REGISTRY_LAYOUTS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, layout_strategy) \
    ALIAS_AND_INSTANTIATE(onyxui, linear_layout) \
    ALIAS_AND_INSTANTIATE(onyxui, grid_layout) \
    ALIAS_AND_INSTANTIATE(onyxui, anchor_layout) \
    ALIAS_AND_INSTANTIATE(onyxui, absolute_layout)

#define ONYXUI_TYPE_REGISTRY_RENDER(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, render_context) \
    ALIAS_AND_INSTANTIATE(onyxui, draw_context) \
    ALIAS_AND_INSTANTIATE(onyxui, measure_context)

#define ONYXUI_TYPE_REGISTRY_ACTIONS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ALIAS_AND_INSTANTIATE(onyxui, action) \
    ALIAS_AND_INSTANTIATE(onyxui, action_group)

#define ONYXUI_TYPE_REGISTRY_ALL(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_CORE(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_BASIC(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_CONTAINERS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_SCROLLING(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_INPUT(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_MVC(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_MENU(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_WINDOWS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_LAYOUTS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_RENDER(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE) \
    ONYXUI_TYPE_REGISTRY_ACTIONS(ALIAS_ONLY, INSTANTIATE_ONLY, ALIAS_AND_INSTANTIATE)
