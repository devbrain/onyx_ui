// Public backend-fixed alias surface.
//
// This file is NOT standalone and intentionally has no include guard.
// Consumers define `ONYXUI_TYPE(name)` and include this file inside the
// target namespace. The canonical type registry lives in
// `type_registry.inc`; this file is just the public-alias view over it.

#ifndef ONYXUI_TYPE
#error "ONYXUI_TYPE must be defined before including public_types.inc"
#endif

#define ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES 1
#include <onyxui/detail/type_registry.inc>
#undef ONYXUI_SKIP_TYPE_REGISTRY_INCLUDES

#define ONYXUI_DETAIL_ALIAS_TYPE(ns, cls) ONYXUI_TYPE(cls)
#define ONYXUI_DETAIL_IGNORE_TYPE(ns, cls)

ONYXUI_TYPE_REGISTRY_ALL(ONYXUI_DETAIL_ALIAS_TYPE, ONYXUI_DETAIL_IGNORE_TYPE, ONYXUI_DETAIL_ALIAS_TYPE)

#undef ONYXUI_DETAIL_ALIAS_TYPE
#undef ONYXUI_DETAIL_IGNORE_TYPE

// Intentionally leave the `ONYXUI_TYPE_REGISTRY_*` macros defined.
// `widget_list.hh` exposes aggregate macros (`ONYXUI_CORE_CLASSES`,
// `ONYXUI_ALL_CLASSES`, etc.) that expand at use-site and dispatch
// back into the registry, so the registry must remain live for the
// rest of the translation unit. Re-including `type_registry.inc`
// (with SKIP) from other files is idempotent — identical token
// replacement is a silent redefinition, so there is no diagnostic.
