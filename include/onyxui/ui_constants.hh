/**
 * @file ui_constants.hh
 * @brief Centralized UI layout and spacing constants
 * @author igor
 * @date 29/10/2025
 */

#pragma once

/**
 * @brief Default UI layout and spacing constants
 *
 * @details
 * These constants provide sensible defaults when theme values are not specified.
 * Using named constants improves maintainability and makes default values explicit.
 *
 * **Usage:**
 * @code
 * #include <onyxui/ui_constants.hh>
 *
 * int padding = ctx.style().padding_horizontal.value
 *     .value_or(ui_constants::DEFAULT_BUTTON_PADDING_HORIZONTAL);
 * @endcode
 */
namespace onyxui::ui_constants {
    // =========================================================================
    // Widget Padding Defaults
    // =========================================================================

    /// Default horizontal padding for buttons (left/right)
    inline constexpr int DEFAULT_BUTTON_PADDING_HORIZONTAL = 2;

    /// Default vertical padding for buttons (top/bottom)
    inline constexpr int DEFAULT_BUTTON_PADDING_VERTICAL = 2;

    /// Default padding for labels (all sides)
    inline constexpr int DEFAULT_LABEL_PADDING = 1;

    /// Default padding for menu items (all sides)
    inline constexpr int DEFAULT_MENU_ITEM_PADDING = 1;

    /// Default horizontal padding for menu bar items (left/right)
    inline constexpr int DEFAULT_MENU_BAR_ITEM_PADDING_HORIZONTAL = 2;

    /// Default vertical padding for menu bar items (top/bottom)
    inline constexpr int DEFAULT_MENU_BAR_ITEM_PADDING_VERTICAL = 0;

    // =========================================================================
    // Scrollbar Defaults
    // =========================================================================

    /// Default scrollbar width/height in logical units
    inline constexpr int DEFAULT_SCROLLBAR_THICKNESS = 1;

    /// Default size of scrollbar arrow buttons (1 character for text UI)
    inline constexpr int DEFAULT_SCROLLBAR_ARROW_SIZE = 1;

    /// Minimum thumb size to ensure usability
    inline constexpr int DEFAULT_SCROLLBAR_MIN_THUMB_SIZE = 3;

    /// Minimum size to render scrollbar without corruption
    inline constexpr int DEFAULT_SCROLLBAR_MIN_RENDER_SIZE = 8;

    // =========================================================================
    // Layout Spacing Defaults
    // =========================================================================

    /// Default spacing between widgets in linear layouts (vbox/hbox)
    inline constexpr int DEFAULT_LAYOUT_SPACING = 4;

    /// Default spacing between grid cells
    inline constexpr int DEFAULT_GRID_SPACING = 2;

    // =========================================================================
    // Animation Defaults (for future use)
    // =========================================================================

    /// Default animation duration in milliseconds
    inline constexpr int DEFAULT_ANIMATION_DURATION_MS = 200;

    /// Default hover delay before showing tooltips
    inline constexpr int DEFAULT_HOVER_DELAY_MS = 100;
} // namespace onyxui::ui_constants
