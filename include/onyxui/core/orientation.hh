/**
 * @file orientation.hh
 * @brief Common orientation enum for UI elements
 * @author OnyxUI Framework
 * @date 2025-11-01
 */

#pragma once

#include <cstdint>

namespace onyxui {

    /**
     * @enum orientation
     * @brief Direction of UI elements (scrollbars, separators, etc.)
     */
    enum class orientation : std::uint8_t {
        horizontal,   ///< Horizontal orientation (left-to-right)
        vertical      ///< Vertical orientation (top-to-bottom)
    };

} // namespace onyxui
