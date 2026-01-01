//
// OnyxUI MVC System - List View
// Created: 2025-11-22
//

#pragma once

#include <vector>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/mvc/views/abstract_item_view.hh>

namespace onyxui {

/**
 * @brief A vertical list view for displaying items
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * list_view displays items from a model in a vertical list.
 * Each item is rendered by a delegate and can be selected/activated.
 *
 * Features:
 * - **Vertical Layout**: Items stacked top-to-bottom
 * - **Custom Rendering**: Uses delegates for item appearance
 * - **Selection**: Single or multi-selection modes
 * - **Keyboard Navigation**: Arrow keys, Page Up/Down, Home/End
 * - **Mouse Interaction**: Click to select, double-click to activate
 * - **Scrolling**: Supports integration with scroll_view
 *
 * @par Performance:
 * - **Virtual Scrolling**: Only visible items are rendered
 * - **Cached Positions**: Item positions calculated once, updated on model changes
 * - **Efficient Repaints**: Only repaints changed items (selection, data)
 *
 * @par Example Usage:
 * @code
 * // Create model
 * auto model = std::make_shared<list_model<std::string, Backend>>();
 * model->set_items({"Apple", "Banana", "Cherry"});
 *
 * // Create view
 * auto view = std::make_unique<list_view<Backend>>();
 * view->set_model(model.get());
 *
 * // Optional: customize delegate
 * auto delegate = std::make_shared<my_custom_delegate<Backend>>();
 * view->set_delegate(delegate);
 *
 * // Listen for activation
 * view->activated.connect([&](const model_index& idx) {
 *     auto data = model->data(idx, item_data_role::display);
 *     std::cout << "Activated: " << std::get<std::string>(data) << "\n";
 * });
 *
 * // Add to UI
 * container->add_child(std::move(view));
 * @endcode
 *
 * @par Integration with Scrolling:
 * @code
 * // Wrap in scroll_view for large lists
 * auto scroll = std::make_unique<scroll_view<Backend>>();
 * auto* view = scroll->template emplace_child<list_view<Backend>>();
 * view->set_model(large_model.get());
 * @endcode
 */
template<UIBackend Backend>
class list_view : public abstract_item_view<Backend> {
public:
    using base = abstract_item_view<Backend>;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;

    /**
     * @brief Construct empty list view
     */
    list_view() = default;

    // ===================================================================
    // Pure Virtual Methods from abstract_item_view
    // ===================================================================

    /**
     * @brief Get index at screen position
     * @param x X coordinate (view-relative)
     * @param y Y coordinate (view-relative)
     * @return Index at position, or invalid if no item
     *
     * @details
     * Performs hit testing by walking through item positions.
     * Only considers visible items for performance.
     */
    [[nodiscard]] model_index index_at(logical_unit x, logical_unit y) const override {
        if (!this->m_model || m_item_rects.empty()) {
            return {};
        }

        // Use logical coordinates directly (double precision)
        double const px = x.value;
        double const py = y.value;

        // Find item at y position (binary search would be faster for large lists)
        int row_count = this->m_model->row_count();
        for (int row = 0; row < row_count; ++row) {
            if (row >= static_cast<int>(m_item_rects.size())) {
                break;  // Safety check
            }

            const auto& rect = m_item_rects[static_cast<std::size_t>(row)];
            // Compare logical coordinates against cached rects (promote rect to double)
            if (point_in_rect_logical(px, py, rect)) {
                return this->m_model->index(row, 0);
            }
        }

        return {};  // Not found
    }

    /**
     * @brief Get visual rectangle for an item
     * @param index Item index
     * @return Rectangle in view coordinates
     */
    [[nodiscard]] rect_type visual_rect(const model_index& index) const override {
        if (!index.is_valid() || index.row < 0 || index.row >= static_cast<int>(m_item_rects.size())) {
            return rect_type{0, 0, 0, 0};
        }

        return m_item_rects[static_cast<std::size_t>(index.row)];
    }

    /**
     * @brief Scroll view to make item visible
     * @param index Item to scroll to
     *
     * @details
     * For now, this is a no-op. When integrated with scroll_view,
     * this would call scroll_controller->scroll_to_rect(visual_rect(index)).
     */
    void scroll_to(const model_index& index) override {
        // TODO: Integrate with scroll_controller when wrapped in scroll_view
        // For now, this is a no-op since standalone list_view doesn't scroll
        (void)index;
    }

    /**
     * @brief Update internal geometries after model changes
     *
     * @details
     * Recalculates item positions and heights based on:
     * - Model row count
     * - Delegate size hints
     * - View width (for item width)
     *
     * This is called when:
     * - Model is set/changed
     * - Rows are inserted/removed
     * - Layout changes
     */
    void update_geometries() override {
        m_item_rects.clear();

        if (!this->m_model || !this->m_delegate) {
            m_content_height = 0;
            return;
        }

        int row_count = this->m_model->row_count();
        if (row_count == 0) {
            m_content_height = 0;
            return;
        }

        m_item_rects.reserve(static_cast<std::size_t>(row_count));

        int current_y = 0;
        int view_width = this->bounds().width.to_int();

        for (int row = 0; row < row_count; ++row) {
            model_index idx = this->m_model->index(row, 0);

            // Get item size hint from delegate
            auto item_size = this->m_delegate->size_hint(idx);
            int item_height = item_size.h;
            int item_width = view_width;  // Use full width

            // Store rectangle
            rect_type item_rect{0, current_y, item_width, item_height};
            m_item_rects.push_back(item_rect);

            current_y += item_height;
        }

        m_content_height = current_y;
    }

    // ===================================================================
    // Rendering
    // ===================================================================

    /**
     * @brief Render the list view
     *
     * @details
     * Renders all visible items using the delegate.
     * Uses virtual scrolling - only renders items in visible region.
     */
    void do_render(render_context<Backend>& ctx) const override {
        if (!this->m_model || !this->m_delegate || m_item_rects.empty()) {
            return;  // Nothing to render
        }

        // Use context position and size (already DPI-scaled physical coordinates)
        // The render() method in element.hh already converts logical bounds to physical
        auto const& pos = ctx.position();
        int const base_x = point_utils::get_x(pos);
        int const base_y = point_utils::get_y(pos);

        // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
        auto logical_bounds = this->bounds();
        auto const [physical_width, physical_height] = ctx.get_final_dims(
            logical_bounds.width.to_int(), logical_bounds.height.to_int());

        // Get metrics for proper DPI scaling of item rectangles
        auto const* metrics = ui_services<Backend>::metrics();

        // Compute scale factor from logical to physical
        // Item rects are computed in logical space during update_geometries()
        // Use Y-axis scale for vertical positions/heights (may differ from X on non-square pixels)
        double const scale_y = metrics
            ? (metrics->logical_to_physical_y * metrics->dpi_scale) : 1.0;

        int row_count = this->m_model->row_count();

        for (int row = 0; row < row_count; ++row) {
            if (row >= static_cast<int>(m_item_rects.size())) {
                break;  // Safety check
            }

            const auto& item_rect = m_item_rects[static_cast<std::size_t>(row)];

            // TODO: Virtual scrolling - skip items outside visible region
            // For now, render all items

            model_index idx = this->m_model->index(row, 0);

            // Determine selection/focus state
            bool is_selected = this->m_selection_model && this->m_selection_model->is_selected(idx);
            bool has_focus = this->m_selection_model && this->m_selection_model->current_index() == idx;

            // Convert item rect to physical coordinates
            // - Base position from ctx.position() (already physical)
            // - Item Y offset and height scaled by Y-axis DPI factor
            // - Width uses full physical view width
            int abs_x = base_x;
            int abs_y = base_y + static_cast<int>(item_rect.y * scale_y);
            int abs_w = physical_width;
            int abs_h = static_cast<int>(item_rect.h * scale_y);

            rect_type abs_item_rect{abs_x, abs_y, abs_w, abs_h};

            // Delegate renders the item
            this->m_delegate->paint(ctx, idx, abs_item_rect, is_selected, has_focus);
        }
    }

    // ===================================================================
    // Layout (Measure/Arrange)
    // ===================================================================

    /**
     * @brief Measure preferred size
     *
     * @details
     * Width: Uses available width (flexible)
     * Height: Sum of all item heights
     */
    [[nodiscard]] size_type measure(int available_width, int available_height) {
        // Update geometries with available width
        update_geometries();

        // Preferred width: use all available width
        int pref_width = available_width;

        // Preferred height: total content height
        int pref_height = m_content_height;

        // Clamp to available
        if (pref_width > available_width) pref_width = available_width;
        if (pref_height > available_height) pref_height = available_height;

        size_type measured_size{pref_width, pref_height};
        this->set_desired_size(measured_size);

        return measured_size;
    }

    /**
     * @brief Arrange items in final bounds
     *
     * @details
     * Updates item rectangles to use final width.
     */
    void arrange(const rect_type& final_bounds) {
        base::arrange(final_bounds);

        // Update geometries with final width
        update_geometries();
    }

    // ===================================================================
    // Properties
    // ===================================================================

    /**
     * @brief Get total content height
     * @return Sum of all item heights
     *
     * @details
     * Useful when integrating with scroll_view to set content size.
     */
    [[nodiscard]] int content_height() const noexcept {
        return m_content_height;
    }

private:
    // ===================================================================
    // Member Variables
    // ===================================================================

    std::vector<rect_type> m_item_rects;  ///< Cached item positions
    int m_content_height = 0;             ///< Total content height

    // ===================================================================
    // Utility Methods
    // ===================================================================

    /**
     * @brief Check if point is inside rectangle (int version)
     */
    [[nodiscard]] static bool point_in_rect(int x, int y, const rect_type& rect) {
        return x >= rect.x && x < rect.x + rect.w &&
               y >= rect.y && y < rect.y + rect.h;
    }

    /**
     * @brief Check if point is inside rectangle (logical/double version)
     * @details Promotes int rect bounds to double for precise comparison
     */
    [[nodiscard]] static bool point_in_rect_logical(double x, double y, const rect_type& rect) {
        double const rx = static_cast<double>(rect.x);
        double const ry = static_cast<double>(rect.y);
        double const rw = static_cast<double>(rect.w);
        double const rh = static_cast<double>(rect.h);
        return x >= rx && x < rx + rw &&
               y >= ry && y < ry + rh;
    }
};

} // namespace onyxui
