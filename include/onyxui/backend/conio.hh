/**
 * @file conio.hh
 * @brief Per-backend alias header for the conio backend.
 *
 * Mirrors `<onyxui/backend/sdlpp.hh>` under `namespace onyxui::conio`.
 * See that file's doc comment for the full mechanism.
 */

#pragma once

#include <onyxui/conio/conio_backend.hh>

#include <onyxui/detail/type_registry.inc>

namespace onyxui::conio {

    /// Canonical name for the backend fixed by this shell header.
    using backend = conio_backend;

    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::conio
