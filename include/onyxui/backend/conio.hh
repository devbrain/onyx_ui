/**
 * @file conio.hh
 * @brief Per-backend alias header for the conio backend.
 *
 * Mirrors `<onyxui/backend/sdlpp.hh>` — populates `onyxui::ui` with
 * conio-backed widget aliases. See that header for the one-backend-
 * per-TU rule and the full mechanism.
 */

#pragma once

#include <onyxui/conio/conio_backend.hh>

#include <onyxui/detail/type_registry.inc>

namespace onyxui::ui {

    /// Canonical alias for the backend picked by this TU's include
    /// of `<onyxui/backend/conio.hh>`.
    using backend = ::onyxui::conio::conio_backend;

    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::ui
