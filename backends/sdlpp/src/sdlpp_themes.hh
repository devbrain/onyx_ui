/**
 * @file sdlpp_themes.hh
 * @brief Built-in theme definitions for the SDL++ backend.
 *
 * The heavy lifting lives in <onyxui/theming/windows_311_theme.hh>.
 * Here we only supply sdlpp-specific font handles; color defaults
 * come straight from the shared palette.
 */

#pragma once

#include <onyxui/sdlpp/sdlpp_backend.hh>
#include <onyxui/theming/theme.hh>
#include <onyxui/theming/windows_311_theme.hh>

namespace onyxui::sdlpp {

/**
 * @brief Classic Windows 3.11 theme for the SDL++ backend.
 *
 * Uses an MS-Sans-Serif-like 14px font (system TTF fallback) as the
 * default, and the same size in bold for titles / active tabs.
 */
[[nodiscard]] inline ui_theme<sdlpp_backend> create_windows311_theme() {
    onyxui::themes::windows_311_palette<sdlpp_backend> p;

    // sdlpp loads TTF files at size_px; 14 is roughly MS Sans Serif 8pt.
    p.default_font.size_px = 14.0f;
    p.bold_font.size_px = 14.0f;
    p.bold_font.bold = true;

    return onyxui::themes::make_windows_311<sdlpp_backend>(p);
}

} // namespace onyxui::sdlpp
