/**
 * @file conio.hh
 * @brief Simple-shell bundle header — conio backend.
 *
 * Mirror of `<onyxui/for/sdlpp.hh>` for the conio backend. See that
 * header's comment for the full mechanism.
 *
 * NOTE: the conio-specific implementations of
 * `onyxui::simple::app_window` / `run()` / dialog helpers are
 * tracked under WAR-59. Until WAR-59 lands, including this header
 * compiles — but linking will fail with unresolved symbols for the
 * `simple::app_window` / `simple::run` methods until the conio
 * backend library provides them.
 */

#pragma once

// Signal to the simple/* headers that the bundle chain is in use —
// see the note on ONYXUI_SIMPLE_BUNDLE_INCLUDED in
// `<onyxui/for/sdlpp.hh>`.
#define ONYXUI_SIMPLE_BUNDLE_INCLUDED 1

#include <onyxui/backend/conio.hh>

namespace onyxui::simple {
    using ::onyxui::conio::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::conio::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::simple

#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/run.hh>
#include <onyxui/simple/dialogs.hh>
