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
 * #include <onyxui/tile/tile_backend.hh>   // SDL backend (requires SDL include path)
 *
 * // Set up your theme
 * onyxui::tile::tile_theme my_theme = { ... };
 * onyxui::tile::set_theme(my_theme);
 *
 * // Run application
 * return onyxui::tile::sdlpp_tile_backend::run_app<MyWidget>("My Game", 800, 600);
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
