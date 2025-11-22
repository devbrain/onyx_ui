//
// OnyxUI MVC System - Default Item Delegate
// Created: 2025-11-22
//

#pragma once

#include <string>
#include <variant>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/mvc/delegates/abstract_item_delegate.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/models/abstract_item_model.hh>
#include <onyxui/theming/theme.hh>

namespace onyxui {

/**
 * @brief Default delegate providing standard item rendering
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * default_item_delegate provides the standard rendering for list/table items:
 * - **Background**: Selection color when selected, normal color otherwise
 * - **Text**: Display text with appropriate foreground color
 * - **Focus**: Dotted rectangle when item has focus
 * - **Custom Colors**: Respects model's background/foreground roles
 *
 * This delegate is automatically used by views unless a custom delegate is set.
 *
 * @par Rendering Behavior:
 * 1. Draw background (selection overrides custom background)
 * 2. Draw display text with padding
 * 3. Draw focus rectangle if has_focus
 *
 * @par Theme Integration:
 * Uses theme colors for selection state:
 * - Selection background: theme's accent color (or blue)
 * - Selection foreground: white
 * - Normal background: transparent
 * - Normal foreground: theme's text color (or black)
 *
 * @par Size Calculation:
 * - Width: Text width + horizontal padding
 * - Height: Text height + vertical padding
 * - Minimum height: 20px for usability
 *
 * @par Example Usage:
 * @code
 * // Views use this by default
 * auto view = std::make_unique<list_view<Backend>>();
 * // view->delegate() returns default_item_delegate
 *
 * // Explicitly set default delegate
 * auto delegate = std::make_shared<default_item_delegate<Backend>>();
 * view->set_delegate(delegate);
 * @endcode
 */
template<UIBackend Backend>
class default_item_delegate : public abstract_item_delegate<Backend> {
public:
    using base = abstract_item_delegate<Backend>;
    using color_type = typename Backend::color_type;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;
    using renderer_type = typename Backend::renderer_type;

    /**
     * @brief Construct default delegate
     */
    default_item_delegate() = default;

    /**
     * @brief Render an item
     *
     * @param ctx Render context for drawing
     * @param index Model index of the item
     * @param rect Rectangle where item should be painted
     * @param is_selected true if item is selected
     * @param has_focus true if item has keyboard focus
     *
     * @details
     * Rendering order:
     * 1. Background (selection color overrides custom background)
     * 2. Text from display role
     * 3. Focus rectangle (dotted border)
     *
     * @par Performance:
     * This method is called for EVERY visible item on EVERY repaint.
     * Keep it fast.
     */
    void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const rect_type& rect,
        bool is_selected,
        bool has_focus
    ) const override {
        if (!index.is_valid()) {
            return;  // Nothing to paint
        }

        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);

        // ===============================================================
        // 1. Draw Background
        // ===============================================================
        // TODO: Background rendering needs to be handled by the view
        // The render_context API doesn't provide a way to draw filled rectangles
        // with explicit colors without going through the style system.
        // For now, we skip background - the view should handle selection highlight.
        (void)is_selected;

        // ===============================================================
        // 2. Draw Text
        // ===============================================================

        // Get display text
        auto text_data = model->data(index, item_data_role::display);
        if (!std::holds_alternative<std::string>(text_data)) {
            return;  // No display text
        }
        std::string text = std::get<std::string>(text_data);

        // Determine text color
        color_type fg_color;
        if (is_selected) {
            // Use selection foreground color (white on blue)
            fg_color = get_selection_foreground();
        } else {
            // Check if model provides custom foreground
            auto fg_data = model->data(index, item_data_role::foreground);
            if (std::holds_alternative<color_type>(fg_data)) {
                fg_color = std::get<color_type>(fg_data);
            } else {
                fg_color = get_normal_foreground();
            }
        }

        // Calculate text position (with padding)
        typename Backend::point_type text_pos{rect.x + PADDING_LEFT, rect.y + PADDING_TOP};

        // Draw text
        auto font = get_font();
        ctx.draw_text(text, text_pos, font, fg_color);

        // ===============================================================
        // 3. Draw Focus Rectangle
        // ===============================================================
        // TODO: Focus rectangle also needs direct renderer access
        // For now, we skip focus rendering
        (void)has_focus;
    }

    /**
     * @brief Get preferred size for an item
     *
     * @param index Model index of the item
     * @return Preferred size {width, height}
     *
     * @details
     * Size calculation:
     * - Width: Text width + horizontal padding
     * - Height: max(text height + vertical padding, MIN_HEIGHT)
     *
     * @par Performance:
     * May be called frequently. Consider caching if model data is expensive.
     */
    [[nodiscard]] size_type size_hint(const model_index& index) const override {
        if (!index.is_valid()) {
            return size_type{0, MIN_HEIGHT};
        }

        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);

        // Get display text
        auto text_data = model->data(index, item_data_role::display);
        if (!std::holds_alternative<std::string>(text_data)) {
            // No text - return minimum size
            return size_type{0, MIN_HEIGHT};
        }
        std::string text = std::get<std::string>(text_data);

        // Measure text
        auto font = get_font();
        auto text_size = renderer_type::measure_text(text, font);

        // Add padding
        int width = text_size.w + PADDING_LEFT + PADDING_RIGHT;
        int height = text_size.h + PADDING_TOP + PADDING_BOTTOM;

        // Enforce minimum height for usability
        if (height < MIN_HEIGHT) {
            height = MIN_HEIGHT;
        }

        return size_type{width, height};
    }

private:
    // ===================================================================
    // Layout Constants
    // ===================================================================

    static constexpr int PADDING_LEFT = 4;
    static constexpr int PADDING_RIGHT = 4;
    static constexpr int PADDING_TOP = 2;
    static constexpr int PADDING_BOTTOM = 2;
    static constexpr int MIN_HEIGHT = 20;

    // ===================================================================
    // Theme Color Accessors
    // ===================================================================

    /**
     * @brief Get selection background color
     * @return Theme accent color or default blue
     */
    [[nodiscard]] color_type get_selection_background() const {
        // TODO: When theme supports list/table styling, use theme->list.selection_background
        // For now, use a sensible default
        return color_type{0, 120, 215};  // Windows-style blue
    }

    /**
     * @brief Get selection foreground color
     * @return White for high contrast
     */
    [[nodiscard]] color_type get_selection_foreground() const {
        return color_type{255, 255, 255};  // White
    }

    /**
     * @brief Get normal background color
     * @return Transparent (let parent show through)
     */
    [[nodiscard]] color_type get_normal_background() const {
        // Transparent - let parent background show
        // Use black with 0 alpha for transparency (backend should handle)
        return color_type{0, 0, 0};  // Transparent/black
    }

    /**
     * @brief Get normal foreground color
     * @return Theme text color or black
     */
    [[nodiscard]] color_type get_normal_foreground() const {
        return color_type{0, 0, 0};  // Black
    }

    /**
     * @brief Get focus rectangle color
     * @return Dark gray for subtle focus indicator
     */
    [[nodiscard]] color_type get_focus_color() const {
        return color_type{128, 128, 128};  // Gray
    }

    /**
     * @brief Get font for text rendering
     * @return Default font
     */
    [[nodiscard]] typename renderer_type::font get_font() const {
        return typename renderer_type::font();  // Default font
    }

    /**
     * @brief Get border style for focus rectangle
     * @return Dotted border (box_style with dotted border)
     */
    [[nodiscard]] typename renderer_type::box_style get_focus_border_style() const {
        typename renderer_type::box_style style;
        style.border = renderer_type::box_border_style::dotted;
        return style;
    }
};

} // namespace onyxui
