/**
 * @file instantiate_widgets.inl
 * @brief Explicit template instantiation for all widget classes
 *
 * @details
 * Include this file in backend .cc files to explicitly instantiate
 * all widget templates for that backend. This reduces compilation
 * times by compiling each template once per backend library.
 *
 * Required macros before inclusion:
 * - ONYXUI_BACKEND: The backend type (e.g., onyxui::conio::conio_backend)
 * - ONYXUI_EXPORT: Export macro for DLL visibility (e.g., ONYXUI_CONIO_EXPORT)
 *
 * Required includes before this file:
 * - All widget headers (use all_widgets.hh)
 * - Backend export header
 *
 * @example
 * @code
 * #include <onyxui/instantiations/all_widgets.hh>
 * #include <onyxui/backends/conio/onyxui_conio_export.h>
 *
 * #define ONYXUI_BACKEND onyxui::conio::conio_backend
 * #define ONYXUI_EXPORT ONYXUI_CONIO_EXPORT
 * #include <onyxui/instantiations/instantiate_widgets.inl>
 * @endcode
 */

#ifndef ONYXUI_BACKEND
#error "ONYXUI_BACKEND must be defined before including instantiate_widgets.inl"
#endif

#ifndef ONYXUI_EXPORT
#define ONYXUI_EXPORT
#endif

#include <onyxui/instantiations/widget_list.hh>

// Generate explicit instantiation for each class
#define ONYXUI_INSTANTIATE_(ns, cls) \
    template class ONYXUI_EXPORT ns::cls<ONYXUI_BACKEND>;

ONYXUI_ALL_CLASSES(ONYXUI_INSTANTIATE_)

#undef ONYXUI_INSTANTIATE_

// Clean up - don't pollute includer's namespace
#undef ONYXUI_BACKEND
#undef ONYXUI_EXPORT
