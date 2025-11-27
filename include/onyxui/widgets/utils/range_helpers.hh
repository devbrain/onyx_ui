// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cmath>

namespace onyxui::range_helpers {

/// Map position (in backend units) to value within range
///
/// @param position Position along track (in logical units)
/// @param track_length Length of track (in backend units)
/// @param min_value Minimum value in range
/// @param max_value Maximum value in range
/// @return Value corresponding to position (clamped to range)
///
/// @note Logical units
template<typename T = int>
[[nodiscard]] inline T position_to_value(
    int position,
    int track_length,
    T min_value,
    T max_value
) {
    if (track_length <= 0 || max_value <= min_value) {
        return min_value;
    }

    // Clamp position to track bounds
    int const clamped_pos = std::clamp(position, 0, track_length);

    // Calculate ratio (0.0 - 1.0)
    float const ratio = static_cast<float>(clamped_pos) / static_cast<float>(track_length);

    // Map to value range
    float const range = static_cast<float>(max_value - min_value);
    return min_value + static_cast<T>(ratio * range);
}

/// Map value to position (in backend units) within track
///
/// @param value Current value
/// @param min_value Minimum value in range
/// @param max_value Maximum value in range
/// @param track_length Length of track (in backend units)
/// @return Position along track (in backend units: 0 = start, track_length = end)
///
/// @note Logical units
template<typename T = int>
[[nodiscard]] inline int value_to_position(
    T value,
    T min_value,
    T max_value,
    int track_length
) {
    if (max_value <= min_value || track_length <= 0) {
        return 0;
    }

    // Clamp value to range
    T const clamped_value = std::clamp(value, min_value, max_value);

    // Calculate ratio (0.0 - 1.0)
    float const range = static_cast<float>(max_value - min_value);
    float const ratio = static_cast<float>(clamped_value - min_value) / range;

    // Map to position (in backend units)
    return static_cast<int>(ratio * static_cast<float>(track_length));
}

/// Snap value to step increments
///
/// @param value Value to snap
/// @param step Step size (0 = no snapping)
/// @return Snapped value (or original if step == 0)
template<typename T = int>
[[nodiscard]] inline T snap_to_step(T value, T step) {
    if (step <= 0) {
        return value;  // No snapping
    }

    // Round to nearest multiple of step
    T const remainder = value % step;
    if (remainder > step / 2) {
        return value + (step - remainder);  // Round up
    } else {
        return value - remainder;  // Round down
    }
}

/// Calculate fill width/height from value
///
/// @param value Current value
/// @param min_value Minimum value
/// @param max_value Maximum value
/// @param track_length Track length (in backend units)
/// @return Fill length (in backend units: 0 to track_length)
///
/// @note Logical units
template<typename T = int>
[[nodiscard]] inline int calculate_fill_length(
    T value,
    T min_value,
    T max_value,
    int track_length
) {
    if (max_value <= min_value || track_length <= 0) {
        return 0;
    }

    T const clamped = std::clamp(value, min_value, max_value);
    float const ratio = static_cast<float>(clamped - min_value) / static_cast<float>(max_value - min_value);
    return static_cast<int>(ratio * static_cast<float>(track_length));
}

/// Calculate proportional thumb size (for scrollbar)
///
/// @param track_length Total track length (in backend units)
/// @param content_size Total content size (in backend units)
/// @param viewport_size Visible viewport size (in backend units)
/// @param min_thumb_size Minimum thumb size (prevents thumb from becoming too small)
/// @return Thumb size (in backend units, clamped to [min_thumb_size, track_length])
///
/// @note Logical units
[[nodiscard]] inline int calculate_proportional_thumb_size(
    int track_length,
    int content_size,
    int viewport_size,
    int min_thumb_size
) {
    if (content_size <= viewport_size || track_length <= 0 || viewport_size <= 0) {
        return 0;  // No scrolling needed
    }

    // Thumb size proportional to viewport/content ratio
    float const ratio = static_cast<float>(viewport_size) / static_cast<float>(content_size);
    int const thumb_size = static_cast<int>(ratio * static_cast<float>(track_length));

    // Enforce minimum size and clamp to track
    return std::clamp(thumb_size, min_thumb_size, track_length);
}

/// Calculate thumb position for proportional thumb (scrollbar)
///
/// @param scroll_offset Current scroll position
/// @param max_scroll Maximum scroll value (content_size - viewport_size)
/// @param track_length Track length (in backend units)
/// @param thumb_size Thumb size (in backend units)
/// @return Thumb position (in backend units: 0 = start of track)
///
/// @note Logical units
[[nodiscard]] inline int calculate_proportional_thumb_position(
    int scroll_offset,
    int max_scroll,
    int track_length,
    int thumb_size
) {
    if (max_scroll <= 0 || track_length <= 0 || thumb_size >= track_length) {
        return 0;
    }

    // Clamp scroll offset
    int const clamped_scroll = std::clamp(scroll_offset, 0, max_scroll);

    // Calculate available space for thumb movement
    int const max_thumb_pos = track_length - thumb_size;

    // Map scroll position to thumb position
    float const ratio = static_cast<float>(clamped_scroll) / static_cast<float>(max_scroll);
    return static_cast<int>(ratio * static_cast<float>(max_thumb_pos));
}

}  // namespace onyxui::range_helpers
