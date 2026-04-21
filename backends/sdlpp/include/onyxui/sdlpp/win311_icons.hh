/**
 * @file win311_icons.hh
 * @brief Embedded Windows 3.11 icon XPM data
 *
 * Data-only header — pure `const char* const[]` arrays in XPM3 format,
 * compiled into the backend. Decoding is done at load time via
 * `onyx_image::xpm_decoder` (see backends/sdlpp/src/sdlpp_renderer.cc).
 */

#pragma once

namespace onyxui::sdlpp::win311_icons {

// Window control buttons (16x16)
extern const char* const close_active[];
extern const char* const close_inactive[];
extern const char* const minimize_active[];
extern const char* const minimize_inactive[];
extern const char* const maximize_active[];
extern const char* const maximize_inactive[];
extern const char* const restore_active[];
extern const char* const restore_inactive[];
extern const char* const menu_active[];
extern const char* const menu_inactive[];

// Checkbox icons (13x13 - standard Win3.11 size)
extern const char* const checkbox_unchecked[];
extern const char* const checkbox_checked[];
extern const char* const checkbox_indeterminate[];
extern const char* const checkbox_unchecked_disabled[];
extern const char* const checkbox_checked_disabled[];

// Radio button icons (16x16)
extern const char* const radio_unchecked[];
extern const char* const radio_checked[];
extern const char* const radio_unchecked_disabled[];
extern const char* const radio_checked_disabled[];

// Scrollbar arrows (16x16)
extern const char* const arrow_up[];
extern const char* const arrow_down[];
extern const char* const arrow_left[];
extern const char* const arrow_right[];
extern const char* const arrow_up_disabled[];
extern const char* const arrow_down_disabled[];
extern const char* const arrow_left_disabled[];
extern const char* const arrow_right_disabled[];

} // namespace onyxui::sdlpp::win311_icons
