/**
 * @file all_widgets.hh
 * @brief Convenience header including all widget definitions
 *
 * @details
 * This header includes all widget class definitions needed for
 * explicit template instantiation. Include this before the
 * instantiate_widgets.inl file.
 */

#pragma once

// =============================================================================
// Core Framework
// =============================================================================

#include <onyxui/core/element.hh>
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
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/widgets/input/list_box.hh>

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

// =============================================================================
// Window Widgets
// =============================================================================

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/main_window.hh>
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/window/window_content_area.hh>

// =============================================================================
// Rendering Contexts
// =============================================================================

#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/core/rendering/measure_context.hh>

// =============================================================================
// Services
// =============================================================================
// Note: ui_handle is NOT included because it's not explicitly instantiated
// (requires backend-specific renderer constructors)
