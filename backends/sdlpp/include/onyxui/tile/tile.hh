/**
 * @file tile.hh
 * @brief Master include header for tile-based rendering (SDL-agnostic core types)
 *
 * This header provides the SDL-agnostic tile types: atlas, fonts, themes, and renderer.
 *
 * For the SDL-specific backend (sdlpp_tile_backend), include:
 * @code
 * #include <onyxui/tile/tile_backend.hh>  // From SDL backend include path
 * @endcode
 *
 * Usage:
 * @code
 * #include <onyxui/tile/tile.hh>           // Core tile types
 * #include <onyxui/tile/tile_backend.hh>   // SDL tile backend
 * #include <onyxui/ui_host.hh>
 *
 * // Set up your theme and a live tile_renderer (which the tile
 * // widgets consult via tile::get_renderer()) before you build the
 * // widget tree. See `backends/sdlpp/strategy_ui_demo.cc` for the
 * // full init sequence.
 * onyxui::tile::tile_theme my_theme = { ... };
 * onyxui::tile::set_theme(my_theme);
 *
 * // Embed via ui_host<sdlpp_tile_backend> and drive your own
 * // render/event loop — the simple shell (<onyxui/for/sdlpp.hh>)
 * // hard-wires `sdlpp_backend` and does not yet thread the tile
 * // renderer through its `app_window`. Direct ui_host embedding is
 * // the supported path until a tile-specific bundle header lands.
 * onyxui::ui_host<onyxui::tile::sdlpp_tile_backend> host(metrics);
 * host.mount(build_ui());
 * // ... call host.render(renderer, viewport) and host.handle_event(ev)
 * //     from your existing loop.
 * @endcode
 */

#pragma once

// Core types
#include <onyxui/tile/tile_types.hh>

// Theme configuration
#include <onyxui/tile/tile_theme.hh>

// Renderer
#include <onyxui/tile/tile_renderer.hh>

// Widgets
#include <onyxui/tile/widgets/tile_widgets.hh>

// NOTE: tile_backend.hh is NOT included here because it requires SDL headers.
// It is now part of the SDL backend at: backends/sdlpp/include/onyxui/tile/tile_backend.hh
