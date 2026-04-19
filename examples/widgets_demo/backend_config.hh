//
// Backend configuration header for widgets_demo.
//
// widgets_demo was ported onto the simple-shell sdlpp bundle under
// WAR-57. Conio parity is tracked under WAR-59 — once the
// `<onyxui/for/conio.hh>` bundle header ships with its conio-side
// `app_window`/`run()`/dialog impls, this header picks the backend
// from the compile flag again. Until then, sdlpp is the only
// supported target for the demo.
//

#pragma once

#include <onyxui/for/sdlpp.hh>
#include <onyxui/instantiations/all_widgets.hh>

#define ONYXUI_BACKEND onyxui::sdlpp::sdlpp_backend
#include <onyxui/instantiations/extern_widgets.inl>

// `onyxui::simple::backend` is the bundle-header-supplied alias for
// whichever backend this file has selected. Consumers who need a
// backend-typed template (e.g. `onyxui::action<Backend>`) use this.
using Backend = onyxui::simple::backend;
