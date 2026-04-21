/**
 * @file conio.hh
 * @brief Per-backend alias header for the conio backend.
 *
 * Populates `onyxui::conio::*` with backend-fixed widget aliases.
 * Mirror of `<onyxui/backend/sdlpp.hh>` — see that header for the
 * ODR-safety rationale and the full mechanism.
 */

#pragma once

#include <onyxui/conio/conio_backend.hh>

#include <onyxui/detail/type_registry.inc>

namespace onyxui::conio {

    /// Canonical alias for the backend introduced by this header.
    using backend = conio_backend;

    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::conio
