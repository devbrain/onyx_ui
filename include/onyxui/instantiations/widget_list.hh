/**
 * @file widget_list.hh
 * @brief X-macro definitions for widget template instantiation
 *
 * @details
 * This file defines macros listing all widget classes that should be
 * explicitly instantiated for each backend. Uses X-macro pattern to
 * allow generating both `template class` and `extern template class`
 * declarations from a single source of truth.
 *
 * Usage:
 * @code
 * #define MY_MACRO(ns, cls) template class ns::cls<MyBackend>;
 * ONYXUI_ALL_CLASSES(MY_MACRO)
 * #undef MY_MACRO
 * @endcode
 */

#pragma once

// =============================================================================
// Core Framework Classes
// =============================================================================

#define ONYXUI_CORE_CLASSES(X) \
    X(onyxui, ui_element) \
    X(onyxui, widget) \
    X(onyxui, widget_container) \
    X(onyxui, stateful_widget)

// =============================================================================
// Basic Widgets
// =============================================================================

#define ONYXUI_BASIC_WIDGETS(X) \
    X(onyxui, button) \
    X(onyxui, label) \
    X(onyxui, icon) \
    X(onyxui, spacer) \
    X(onyxui, spring) \
    X(onyxui, progress_bar) \
    X(onyxui, status_bar)

// =============================================================================
// Container Widgets
// =============================================================================

#define ONYXUI_CONTAINER_WIDGETS(X) \
    X(onyxui, panel) \
    X(onyxui, vbox) \
    X(onyxui, hbox) \
    X(onyxui, grid) \
    X(onyxui, group_box) \
    X(onyxui, anchor_panel) \
    X(onyxui, absolute_panel) \
    X(onyxui, tab_widget)

// =============================================================================
// Scrolling Widgets
// =============================================================================

#define ONYXUI_SCROLL_WIDGETS(X) \
    X(onyxui, scroll_view) \
    X(onyxui, scrollable) \
    X(onyxui, scrollbar) \
    X(onyxui, scrollbar_thumb) \
    X(onyxui, scrollbar_arrow) \
    X(onyxui, scroll_controller)

// =============================================================================
// Input Widgets
// =============================================================================

#define ONYXUI_INPUT_WIDGETS(X) \
    X(onyxui, checkbox) \
    X(onyxui, radio_button) \
    X(onyxui, slider) \
    X(onyxui, line_edit) \
    X(onyxui, text_view) \
    X(onyxui, combo_box) \
    X(onyxui, list_box)

// =============================================================================
// MVC Views
// =============================================================================

#define ONYXUI_MVC_VIEWS(X) \
    X(onyxui, list_view) \
    X(onyxui, combo_box_view) \
    X(onyxui, table_view) \
    X(onyxui, default_item_delegate)

// =============================================================================
// Menu Widgets
// =============================================================================

#define ONYXUI_MENU_WIDGETS(X) \
    X(onyxui, menu_bar) \
    X(onyxui, menu) \
    X(onyxui, menu_item) \
    X(onyxui, menu_bar_item) \
    X(onyxui, separator)

// =============================================================================
// Window Widgets
// =============================================================================

#define ONYXUI_WINDOW_WIDGETS(X) \
    X(onyxui, window) \
    X(onyxui, dialog) \
    X(onyxui, main_window) \
    X(onyxui, window_title_bar) \
    X(onyxui, window_content_area)

// =============================================================================
// Rendering Context Classes
// =============================================================================

#define ONYXUI_RENDER_CLASSES(X) \
    X(onyxui, render_context) \
    X(onyxui, draw_context) \
    X(onyxui, measure_context)

// =============================================================================
// Service Classes
// =============================================================================
// Note: ui_handle is NOT instantiated because it requires backend-specific
// renderer constructors. Users instantiate it explicitly when needed.

#define ONYXUI_SERVICE_CLASSES(X)

// =============================================================================
// Combined Macro - All Widget Classes
// =============================================================================

#define ONYXUI_ALL_WIDGET_CLASSES(X) \
    ONYXUI_BASIC_WIDGETS(X) \
    ONYXUI_CONTAINER_WIDGETS(X) \
    ONYXUI_SCROLL_WIDGETS(X) \
    ONYXUI_INPUT_WIDGETS(X) \
    ONYXUI_MVC_VIEWS(X) \
    ONYXUI_MENU_WIDGETS(X) \
    ONYXUI_WINDOW_WIDGETS(X)

// =============================================================================
// Combined Macro - All Classes (Core + Widgets + Render + Services)
// =============================================================================

#define ONYXUI_ALL_CLASSES(X) \
    ONYXUI_CORE_CLASSES(X) \
    ONYXUI_ALL_WIDGET_CLASSES(X) \
    ONYXUI_RENDER_CLASSES(X) \
    ONYXUI_SERVICE_CLASSES(X)
