//
// OnyxUI MVC System - List View
// Created: 2025-11-22
//

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/mvc/views/abstract_item_view.hh>
#include <onyxui/services/ui_services.hh>

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
     * @param x X coordinate (view-relative, logical units)
     * @param y Y coordinate (view-relative, logical units)
     * @return Index at position, or invalid if no item
     *
     * @details
     * Performs hit testing by walking through item positions.
     * Accounts for scroll offset - click position is in view space,
     * while item rects are in content space.
     *
     * @par Performance:
     * Uses early termination when past visible items.
     * Could be optimized with binary search for very large lists.
     */
    [[nodiscard]] model_index index_at(logical_unit x, logical_unit y) const override {
        if (!this->m_model || m_item_rects.empty()) {
            return {};
        }

        // Convert widget-relative coordinates to content-relative coordinates
        // by subtracting border offset and adding scroll offset.
        // Item rects are stored relative to content area (inside border).
        constexpr double logical_border_width = 1.0;
        double const px = x.value - logical_border_width;
        double const py = y.value - logical_border_width + m_scroll_offset_y;

        // Find item at y position
        int row_count = this->m_model->row_count();
        for (int row = 0; row < row_count; ++row) {
            if (row >= static_cast<int>(m_item_rects.size())) {
                break;  // Safety check
            }

            const auto& rect = m_item_rects[static_cast<std::size_t>(row)];

            // Early termination: if we're past this row's bottom, item rects are sorted
            // so we can stop if the click is above this item (since previous items were checked)
            if (rect.y.value > py) {
                break;  // Clicked in gap before this item, no match
            }

            // Check if point is in this item's rect
            if (point_in_rect_logical(px, py, rect)) {
                return this->m_model->index(row, 0);
            }
        }

        return {};  // Not found
    }

    /**
     * @brief Get visual rectangle for an item
     * @param index Item index
     * @return Rectangle in view coordinates (logical units), adjusted for scroll offset
     */
    [[nodiscard]] logical_rect visual_rect_logical(const model_index& index) const {
        if (!index.is_valid() || index.row < 0 || index.row >= static_cast<int>(m_item_rects.size())) {
            return logical_rect{};
        }

        // Get content-space rect and adjust for scroll offset
        logical_rect rect = m_item_rects[static_cast<std::size_t>(index.row)];
        rect.y = rect.y - logical_unit(m_scroll_offset_y);
        return rect;
    }

    /**
     * @brief Get visual rectangle for an item (Backend rect_type in physical units)
     * @param index Item index
     * @return Rectangle in physical coordinates for delegate/renderer use
     */
    [[nodiscard]] rect_type visual_rect(const model_index& index) const override {
        auto logical = visual_rect_logical(index);

        // Convert logical rect to physical using backend metrics
        if (auto const* metrics = ui_services<Backend>::metrics()) {
            return metrics->snap_rect(logical);
        }

        // Fallback: 1:1 mapping (e.g., conio backend)
        return rect_type{
            static_cast<int>(logical.x.value),
            static_cast<int>(logical.y.value),
            static_cast<int>(logical.width.value),
            static_cast<int>(logical.height.value)
        };
    }

    /**
     * @brief Scroll view to make item visible
     * @param index Item to scroll to
     *
     * @details
     * Adjusts scroll offset so the item is fully visible within the view bounds.
     * - If item is above visible area: scrolls up to show item at top
     * - If item is below visible area: scrolls down to show item at bottom
     * - If item is already visible: no change
     */
    void scroll_to(const model_index& index) override {
        if (!index.is_valid() || index.row < 0 ||
            index.row >= static_cast<int>(m_item_rects.size())) {
            return;
        }

        auto const& item_rect = m_item_rects[static_cast<std::size_t>(index.row)];
        double const view_height = this->bounds().height.value;

        // Item boundaries relative to content
        double const item_top = item_rect.y.value;
        double const item_bottom = item_top + item_rect.height.value;

        // Visible region boundaries
        double const visible_top = m_scroll_offset_y;
        double const visible_bottom = m_scroll_offset_y + view_height;

        // Adjust scroll offset if item is not fully visible
        if (item_top < visible_top) {
            // Item is above visible area - scroll up
            set_scroll_offset(item_top);
        } else if (item_bottom > visible_bottom) {
            // Item is below visible area - scroll down
            // Position so item bottom aligns with view bottom
            set_scroll_offset(item_bottom - view_height);
        }
        // else: item is fully visible, no scrolling needed
    }

    // ===================================================================
    // Scrolling
    // ===================================================================

    /**
     * @brief Get current scroll offset (Y axis)
     * @return Scroll offset in logical units
     */
    [[nodiscard]] double scroll_offset() const noexcept {
        return m_scroll_offset_y;
    }

    /**
     * @brief Set scroll offset (Y axis)
     * @param offset New scroll offset in logical units
     *
     * @details
     * Clamps offset to valid range [0, max_scroll].
     * Triggers repaint if offset changed.
     */
    void set_scroll_offset(double offset) {
        // Clamp to valid range
        double const max_scroll = max_scroll_offset();
        offset = std::max(0.0, std::min(offset, max_scroll));

        if (std::abs(offset - m_scroll_offset_y) < 0.001) {
            return;  // No significant change
        }

        m_scroll_offset_y = offset;
        this->mark_dirty();

        // Emit signal for scroll synchronization
        scroll_offset_changed.emit(m_scroll_offset_y);
    }

    /**
     * @brief Get maximum scroll offset
     * @return Maximum valid scroll offset (content_height - view_height), or 0 if no scrolling needed
     */
    [[nodiscard]] double max_scroll_offset() const noexcept {
        // Visible height excludes the border (1 logical unit on each side)
        constexpr double logical_border_width = 1.0;
        double const view_height = this->bounds().height.value - (logical_border_width * 2);
        double const content_height_d = static_cast<double>(m_content_height);
        return std::max(0.0, content_height_d - view_height);
    }

    /**
     * @brief Scroll by a delta amount
     * @param delta Amount to scroll (positive = down, negative = up)
     */
    void scroll_by(double delta) {
        set_scroll_offset(m_scroll_offset_y + delta);
    }

    /**
     * @brief Signal emitted when scroll offset changes
     *
     * @details
     * Useful for synchronizing with external scrollbars.
     */
    signal<double> scroll_offset_changed;

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
            m_content_width = 0;
            m_content_height = 0;
            return;
        }

        int row_count = this->m_model->row_count();
        if (row_count == 0) {
            m_content_width = 0;
            m_content_height = 0;
            return;
        }

        m_item_rects.reserve(static_cast<std::size_t>(row_count));

        double current_y = 0.0;
        double view_width = this->bounds().width.value;

        // Get metrics for physical→logical conversion
        // Delegates return physical units; we store logical units
        auto const* metrics = ui_services<Backend>::metrics();

        for (int row = 0; row < row_count; ++row) {
            model_index idx = this->m_model->index(row, 0);

            // Get item size hint from delegate (physical units)
            auto item_size = this->m_delegate->size_hint(idx);

            // Convert physical height to logical height
            double item_height = metrics
                ? metrics->physical_to_logical_y(item_size.h).value
                : static_cast<double>(item_size.h);  // Fallback 1:1

            double item_width = view_width;  // Use full width (already logical)

            // Store rectangle in logical coordinates
            logical_rect item_rect{
                logical_unit(0.0),
                logical_unit(current_y),
                logical_unit(item_width),
                logical_unit(item_height)
            };
            m_item_rects.push_back(item_rect);

            current_y += item_height;
        }

        m_content_width = static_cast<int>(view_width);
        m_content_height = static_cast<int>(current_y);
    }

    // ===================================================================
    // Rendering
    // ===================================================================

    /**
     * @brief Render the list view
     *
     * @details
     * Renders only visible items using the delegate.
     * Uses virtual scrolling - items outside the visible region are skipped.
     *
     * @par Virtual Scrolling Algorithm:
     * 1. Calculate visible region based on scroll offset and view height
     * 2. Find first item that intersects visible region (binary search optimization possible)
     * 3. Render items until past visible region
     * 4. Skip items completely above or below visible region
     */
    void do_render(render_context<Backend>& ctx) const override {
        // Use context position (already DPI-scaled physical coordinates)
        auto const& pos = ctx.position();
        int const base_x = static_cast<int>(pos.x);
        int const base_y = static_cast<int>(pos.y);

        // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
        auto current_bounds = this->bounds();
        auto const [physical_width, physical_height] = ctx.get_final_dims(
            current_bounds.width.to_int(), current_bounds.height.to_int());

        // Draw list background and border first (so items render on top)
        // Get theme for list colors
        auto* themes = ui_services<Backend>::themes();
        auto const* theme = themes ? themes->get_current_theme() : nullptr;

        // Get metrics for logical→physical conversion
        auto const* metrics = ui_services<Backend>::metrics();

        // Border inset: 1 logical unit for border (converts to physical)
        // Use separate X/Y scaling for non-square pixel backends
        constexpr double logical_border_width = 1.0;
        int const physical_border_x = metrics
            ? metrics->snap_to_physical_x(logical_unit(logical_border_width), snap_mode::round)
            : 1;
        int const physical_border_y = metrics
            ? metrics->snap_to_physical_y(logical_unit(logical_border_width), snap_mode::round)
            : 1;

        if (theme) {
            rect_type bg_rect{base_x, base_y, physical_width, physical_height};
            // Fill background
            ctx.fill_rect(bg_rect, theme->list.item_background);
            // Draw border frame
            ctx.draw_rect(bg_rect, theme->list.focus_box_style);
        }

        if (!this->m_model || !this->m_delegate || m_item_rects.empty()) {
            return;  // Nothing to render
        }

        // Adjust item rendering area to be inside the border (physical coordinates)
        int const content_x = base_x + physical_border_x;
        int const content_y = base_y + physical_border_y;
        int const content_width = physical_width - (physical_border_x * 2);

        // Virtual scrolling: calculate visible region (all in logical units)
        // Account for border inset in the visible height
        double const visible_height = current_bounds.height.value - (logical_border_width * 2);
        double const visible_top = m_scroll_offset_y;
        double const visible_bottom = m_scroll_offset_y + visible_height;

        int row_count = this->m_model->row_count();

        for (int row = 0; row < row_count; ++row) {
            if (row >= static_cast<int>(m_item_rects.size())) {
                break;  // Safety check
            }

            const auto& item_rect = m_item_rects[static_cast<std::size_t>(row)];

            // Virtual scrolling: skip items outside visible region (logical coords)
            double const item_top = item_rect.y.value;
            double const item_bottom = item_top + item_rect.height.value;

            // Skip items completely above visible region
            if (item_bottom <= visible_top) {
                continue;
            }

            // Stop if we've passed the visible region (items are in order)
            if (item_top >= visible_bottom) {
                break;
            }

            model_index idx = this->m_model->index(row, 0);

            // Determine selection/focus state
            bool is_selected = this->m_selection_model && this->m_selection_model->is_selected(idx);
            bool has_focus = this->m_selection_model && this->m_selection_model->current_index() == idx;

            // Convert logical item rect to physical coordinates for delegate
            // Item Y is relative to content start, adjusted for scroll offset
            double const scrolled_y_logical = item_rect.y.value - m_scroll_offset_y;

            // Convert logical Y offset and height to physical
            int const scrolled_y_physical = metrics
                ? metrics->snap_to_physical_y(logical_unit(scrolled_y_logical), snap_mode::floor)
                : static_cast<int>(scrolled_y_logical);
            int const item_height_physical = metrics
                ? metrics->snap_to_physical_y(item_rect.height, snap_mode::round)
                : static_cast<int>(item_rect.height.value);

            int abs_x = content_x;
            int abs_y = content_y + scrolled_y_physical;
            int abs_w = content_width;
            int abs_h = item_height_physical;

            rect_type abs_item_rect{abs_x, abs_y, abs_w, abs_h};

            // Delegate renders the item (receives physical coordinates)
            this->m_delegate->paint(ctx, idx, abs_item_rect, is_selected, has_focus);
        }
    }

    // ===================================================================
    // Layout (Measure/Arrange)
    // ===================================================================

protected:
    /**
     * @brief Get intrinsic content size
     *
     * @details
     * Width: Uses available width (flexible)
     * Height: Sum of all item heights
     */
    [[nodiscard]] logical_size get_content_size() const override {
        if (!this->m_model || !this->m_delegate) {
            return {logical_unit(0), logical_unit(0)};
        }

        // Update geometries (const_cast needed since this modifies cache)
        const_cast<list_view*>(this)->update_geometries();

        return {
            logical_unit(static_cast<double>(m_content_width)),
            logical_unit(static_cast<double>(m_content_height))
        };
    }

    /**
     * @brief Arrange items in final bounds
     *
     * @details
     * Updates item rectangles to use final width.
     */
    void do_arrange(const logical_rect& final_bounds) override {
        base::do_arrange(final_bounds);

        // Update geometries with final width
        update_geometries();
    }

public:

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

    std::vector<logical_rect> m_item_rects;  ///< Cached item positions (logical coords)
    int m_content_width = 0;                  ///< Total content width
    int m_content_height = 0;                 ///< Total content height
    double m_scroll_offset_y = 0.0;           ///< Current scroll offset (logical units)

    // ===================================================================
    // Utility Methods
    // ===================================================================

    /**
     * @brief Check if point is inside rectangle (logical_rect version)
     */
    [[nodiscard]] static bool point_in_rect_logical(double x, double y, const logical_rect& rect) {
        double const rx = rect.x.value;
        double const ry = rect.y.value;
        double const rw = rect.width.value;
        double const rh = rect.height.value;
        return x >= rx && x < rx + rw &&
               y >= ry && y < ry + rh;
    }
};

} // namespace onyxui
