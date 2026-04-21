//
// Backend configuration header for widgets_demo.
//
// widgets_demo is a simple-shell consumer: we want widgets + the
// FLTK-grade app framework (app_window, run, dialogs), so we include
// the <onyxui/for/sdlpp.hh> bundle.
//
// Namespace convention for demo code:
//   ui::button, ui::label, ui::vbox, ...            (widgets)
//   ui::app_window, ui::run, ui::modal_dialog, ...  (app shell)
// Both are reachable through the single `ui` alias below because
// onyxui::ui_app is a superset of onyxui::ui that also carries the
// app-shell types.
//
// Conio parity is tracked under WAR-59 — once the
// <onyxui/for/conio.hh> bundle header ships with its conio-side
// app_window/run/dialog impls, this header picks the backend from
// a compile flag again. Until then, sdlpp is the only supported
// target for the demo.
//

#pragma once

#include <onyxui/for/sdlpp.hh>

#define ONYXUI_BACKEND onyxui::sdlpp::sdlpp_backend
#include <onyxui/instantiations/extern_widgets.inl>

// Backend type for code that needs to name it explicitly
// (e.g. `onyxui::action<Backend>`).
using Backend = onyxui::ui_app::backend;

// Short alias for the simple-shell namespace (widgets + app shell).
// Demo code writes `ui::button`, `ui::app_window`, etc. The same
// `ui::` reads the same as warlords-style embedders who instead
// alias `namespace ui = onyxui::ui;` for widgets-only.
namespace ui = onyxui::ui_app;
