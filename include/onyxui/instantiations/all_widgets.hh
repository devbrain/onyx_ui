/**
 * @file all_widgets.hh
 * @brief Convenience header including the full canonical type registry
 *
 * Include this before `instantiate_widgets.inl` or `extern_widgets.inl`
 * when you need declarations/definitions for the entire explicit-
 * instantiation surface.
 */

#pragma once

#include <onyxui/detail/type_registry.inc>

#undef ONYXUI_TYPE_REGISTRY_ALL
#undef ONYXUI_TYPE_REGISTRY_RENDER
#undef ONYXUI_TYPE_REGISTRY_WINDOWS
#undef ONYXUI_TYPE_REGISTRY_MENU
#undef ONYXUI_TYPE_REGISTRY_MVC
#undef ONYXUI_TYPE_REGISTRY_INPUT
#undef ONYXUI_TYPE_REGISTRY_SCROLLING
#undef ONYXUI_TYPE_REGISTRY_CONTAINERS
#undef ONYXUI_TYPE_REGISTRY_BASIC
#undef ONYXUI_TYPE_REGISTRY_CORE
