/**
 * @file extern_widgets.inl
 * @brief Extern template declarations for all widget classes
 *
 * @details
 * Include this file in user code to declare extern templates,
 * telling the compiler to use the pre-instantiated templates from
 * the backend library instead of instantiating them again.
 *
 * This is optional but can significantly reduce compilation times
 * for projects using OnyxUI.
 *
 * Required macros before inclusion:
 * - ONYXUI_BACKEND: The backend type (e.g., onyxui::conio::conio_backend)
 *
 * Required includes before this file:
 * - All widget headers (use all_widgets.hh)
 *
 * @example
 * @code
 * #include <onyxui/instantiations/all_widgets.hh>
 *
 * #define ONYXUI_BACKEND onyxui::conio::conio_backend
 * #include <onyxui/instantiations/extern_widgets.inl>
 * @endcode
 */

#ifndef ONYXUI_BACKEND
#error "ONYXUI_BACKEND must be defined before including extern_widgets.inl"
#endif

#include <onyxui/instantiations/widget_list.hh>

// Generate extern template declaration for each class
#define ONYXUI_EXTERN_(ns, cls) \
    extern template class ns::cls<ONYXUI_BACKEND>;

ONYXUI_ALL_CLASSES(ONYXUI_EXTERN_)

#undef ONYXUI_EXTERN_

// Clean up - don't pollute includer's namespace
#undef ONYXUI_BACKEND
