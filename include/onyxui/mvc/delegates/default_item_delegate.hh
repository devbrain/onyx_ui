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
#include <onyxui/services/ui_services.hh>
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

        // Get theme for styling (use list theme if available)
        auto* themes = ui_services<Backend>::themes();
        auto const* theme = themes ? themes->get_current_theme() : nullptr;

        // ===============================================================
        // 1. Draw Background
        // ===============================================================
        if (is_selected) {
            color_type bg_color = theme
                ? theme->list.selection_background
                : get_selection_background();
            ctx.fill_rect(rect, bg_color);
        } else {
            // Check if model provides custom background for this item
            auto bg_data = model->data(index, item_data_role::background);
            if (std::holds_alternative<color_type>(bg_data)) {
                ctx.fill_rect(rect, std::get<color_type>(bg_data));
            }
            // If no custom background, parent shows through (transparent)
        }

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
            fg_color = theme
                ? theme->list.selection_foreground
                : get_selection_foreground();
        } else {
            // Check if model provides custom foreground
            auto fg_data = model->data(index, item_data_role::foreground);
            if (std::holds_alternative<color_type>(fg_data)) {
                fg_color = std::get<color_type>(fg_data);
            } else {
                fg_color = theme
                    ? theme->list.item_foreground
                    : get_normal_foreground();
            }
        }

        // Calculate text position (with padding from theme or defaults)
        int const pad_left = theme ? theme->list.padding_horizontal : PADDING_LEFT;
        int const pad_top = theme ? theme->list.padding_vertical : PADDING_TOP;
        typename Backend::point_type text_pos{rect.x + pad_left, rect.y + pad_top};

        // Get font from theme or use default
        auto font = theme ? theme->list.font : get_font();
        ctx.draw_text(text, text_pos, font, fg_color);

        // ===============================================================
        // 3. Draw Focus Rectangle
        // ===============================================================
        if (has_focus) {
            // Draw focus border using theme styling
            auto focus_style = theme
                ? theme->list.focus_box_style
                : get_focus_border_style();
            ctx.draw_rect(rect, focus_style);
        }
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
        // Get theme for layout values
        auto* themes = ui_services<Backend>::themes();
        auto const* theme = themes ? themes->get_current_theme() : nullptr;
        int const min_height = theme ? theme->list.min_item_height : MIN_HEIGHT;

        if (!index.is_valid()) {
            return size_type{0, min_height};
        }

        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);

        // Get display text
        auto text_data = model->data(index, item_data_role::display);
        if (!std::holds_alternative<std::string>(text_data)) {
            // No text - return minimum size
            return size_type{0, min_height};
        }
        std::string text = std::get<std::string>(text_data);

        // Measure text with theme font
        auto font = theme ? theme->list.font : get_font();
        auto text_size = renderer_type::measure_text(text, font);

        // Add padding from theme
        int const pad_h = theme ? theme->list.padding_horizontal : PADDING_LEFT;
        int const pad_v = theme ? theme->list.padding_vertical : PADDING_TOP;
        int width = text_size.w + (pad_h * 2);
        int height = text_size.h + (pad_v * 2);

        // Enforce minimum height for usability
        if (height < min_height) {
            height = min_height;
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
     * @return Default box_style (backend defines actual appearance)
     */
    [[nodiscard]] typename renderer_type::box_style get_focus_border_style() const {
        return typename renderer_type::box_style{};
    }
};

} // namespace onyxui
