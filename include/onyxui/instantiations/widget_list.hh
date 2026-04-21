/**
 * @file widget_list.hh
 * @brief Explicit-instantiation view over the canonical type registry
 *
 * Usage:
 * @code
 * #define MY_MACRO(ns, cls) template class ns::cls<MyBackend>;
 * ONYXUI_ALL_CLASSES(MY_MACRO)
 * #undef MY_MACRO
 * @endcode
 */

#pragma once

// Pull in the registry macros (skipping the include block — callers
// must already have the widget definitions in scope, typically via
// `all_widgets.hh` or a backend alias header). The `ONYXUI_*_CLASSES`
// aggregates below dispatch back into `ONYXUI_TYPE_REGISTRY_*` at
// use-site, so those must remain defined for the rest of the TU; we
// deliberately do not `#undef` them at the end of this file.
#define ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES 1
#include <onyxui/detail/type_registry.inc>
#undef ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES

#define ONYXUI_DETAIL_IGNORE_TYPE(ns, cls)

#define ONYXUI_CORE_CLASSES(X) \
    ONYXUI_TYPE_REGISTRY_CORE(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_BASIC_WIDGETS(X) \
    ONYXUI_TYPE_REGISTRY_BASIC(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_CONTAINER_WIDGETS(X) \
    ONYXUI_TYPE_REGISTRY_CONTAINERS(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_SCROLL_WIDGETS(X) \
    ONYXUI_TYPE_REGISTRY_SCROLLING(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_INPUT_WIDGETS(X) \
    ONYXUI_TYPE_REGISTRY_INPUT(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_MVC_VIEWS(X) \
    ONYXUI_TYPE_REGISTRY_MVC(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_MENU_WIDGETS(X) \
    ONYXUI_TYPE_REGISTRY_MENU(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_WINDOW_WIDGETS(X) \
    ONYXUI_TYPE_REGISTRY_WINDOWS(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_LAYOUT_CLASSES(X) \
    ONYXUI_TYPE_REGISTRY_LAYOUTS(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_RENDER_CLASSES(X) \
    ONYXUI_TYPE_REGISTRY_RENDER(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

#define ONYXUI_ACTION_CLASSES(X) \
    ONYXUI_TYPE_REGISTRY_ACTIONS(ONYXUI_DETAIL_IGNORE_TYPE, X, X)

// Note: ui_handle is NOT instantiated because it requires backend-specific
// renderer constructors. Users instantiate it explicitly when needed.
#define ONYXUI_SERVICE_CLASSES(X)

#define ONYXUI_ALL_WIDGET_CLASSES(X) \
    ONYXUI_BASIC_WIDGETS(X) \
    ONYXUI_CONTAINER_WIDGETS(X) \
    ONYXUI_SCROLL_WIDGETS(X) \
    ONYXUI_INPUT_WIDGETS(X) \
    ONYXUI_MVC_VIEWS(X) \
    ONYXUI_MENU_WIDGETS(X) \
    ONYXUI_WINDOW_WIDGETS(X)

#define ONYXUI_ALL_CLASSES(X) \
    ONYXUI_CORE_CLASSES(X) \
    ONYXUI_ALL_WIDGET_CLASSES(X) \
    ONYXUI_LAYOUT_CLASSES(X) \
    ONYXUI_RENDER_CLASSES(X) \
    ONYXUI_ACTION_CLASSES(X) \
    ONYXUI_SERVICE_CLASSES(X)
