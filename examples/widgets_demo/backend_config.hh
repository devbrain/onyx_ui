//
// Backend configuration header for widgets_demo.
//
// widgets_demo is a simple-shell consumer (WAR-57) — it wants
// widgets *and* the FLTK-grade app framework (app_window, run,
// dialog helpers). The sdlpp bundle header provides both, layered
// into `onyxui::ui_app::*`.
//
// Demo code aliases `ui = onyxui::ui_app`. Widgets and shell types
// both live there:
//   ui::button, ui::label, ui::vbox, ...  → widgets
//   ui::app_window, ui::run, ui::dialog   → app shell
//
// Conio parity is tracked under WAR-59. Until the
// <onyxui/for/conio.hh> bundle ships its conio-side app shell, this
// header picks sdlpp unconditionally.
//
// Note: `<onyxui/ui.hh>` (the program-level `onyxui::ui::*` alias
// with the ONYXUI_UI_BACKEND macro) is aimed at engine embedders
// that skip the simple shell. Simple-shell consumers like this demo
// use `onyxui::ui_app::*` directly and don't need it.
//

#pragma once

#include <onyxui/for/sdlpp.hh>

#define ONYXUI_BACKEND onyxui::sdlpp::sdlpp_backend
#include <onyxui/instantiations/extern_widgets.inl>

// Backend type for code that needs to name it explicitly
// (e.g. `onyxui::action<Backend>`).
using Backend = onyxui::ui_app::backend;

// Short alias for the simple-shell namespace (widgets + app shell).
namespace ui = onyxui::ui_app;
