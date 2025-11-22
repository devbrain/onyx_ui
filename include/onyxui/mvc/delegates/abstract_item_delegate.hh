//
// OnyxUI MVC System - Abstract Item Delegate
// Created: 2025-11-22
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/models/abstract_item_model.hh>

namespace onyxui {

/**
 * @brief Abstract base class for item delegates
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * Item delegates are responsible for rendering individual items in views.
 * They provide separation of presentation from data, allowing:
 * - Custom rendering without subclassing views
 * - Reusable rendering logic across different views
 * - Per-item rendering customization
 *
 * Delegates handle:
 * - **Painting**: Draw items with custom colors, fonts, icons, etc.
 * - **Size Hints**: Provide preferred size for each item
 * - **Future: Editing**: Create editor widgets for in-place editing
 *
 * @par Design Pattern:
 * This follows the Qt delegate pattern where views delegate item rendering
 * to separate delegate objects.
 *
 * @par Typical Usage:
 * @code
 * // Use default delegate
 * auto view = std::make_unique<list_view<Backend>>();
 * // view already has default_item_delegate
 *
 * // Use custom delegate
 * auto custom_delegate = std::make_shared<my_custom_delegate<Backend>>();
 * view->set_delegate(custom_delegate);
 * @endcode
 *
 * @par Custom Delegate Example:
 * @code
 * template<UIBackend Backend>
 * class icon_delegate : public abstract_item_delegate<Backend> {
 * public:
 *     void paint(render_context<Backend>& ctx,
 *                const model_index& index,
 *                const typename Backend::rect_type& rect,
 *                bool is_selected,
 *                bool has_focus) const override {
 *         // Custom rendering with icon + text
 *         draw_icon(...);
 *         draw_text(...);
 *     }
 *
 *     typename Backend::size_type size_hint(const model_index& index) const override {
 *         return {200, 32};  // Custom size
 *     }
 * };
 * @endcode
 */
template<UIBackend Backend>
class abstract_item_delegate {
public:
    virtual ~abstract_item_delegate() = default;

    /**
     * @brief Render an item
     *
     * @param ctx Render context for drawing
     * @param index Model index of the item to paint
     * @param rect Rectangle where the item should be painted
     * @param is_selected true if the item is selected
     * @param has_focus true if the item has keyboard focus
     *
     * @details
     * This method is called by the view for each visible item.
     * The delegate should:
     * 1. Query the model for data (display, colors, decoration)
     * 2. Draw background (consider selection state)
     * 3. Draw content (text, icons, etc.)
     * 4. Draw focus indicator if has_focus
     *
     * The rect is in view coordinates and already accounts for:
     * - Item position in the view
     * - View scrolling (if any)
     * - View padding/margins
     *
     * @par Selection Handling:
     * When is_selected is true, delegates typically:
     * - Use selection background color (e.g., blue)
     * - Use selection text color (e.g., white on blue)
     * - Override any custom item colors
     *
     * @par Focus Handling:
     * When has_focus is true, delegates typically:
     * - Draw a dotted focus rectangle
     * - Highlight the item differently than selection
     * - Only one item can have focus at a time
     *
     * @par Performance:
     * This method is called for EVERY visible item on EVERY repaint.
     * Keep it fast - avoid expensive operations.
     *
     * @par Example Implementation:
     * @code
     * void paint(render_context<Backend>& ctx, ...) const override {
     *     auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);
     *
     *     // Get data
     *     auto text_data = model->data(index, item_data_role::display);
     *     std::string text = std::get<std::string>(text_data);
     *
     *     // Background
     *     auto bg = is_selected ? SELECTION_COLOR : NORMAL_COLOR;
     *     ctx.fill_rect(rect, bg);
     *
     *     // Text
     *     auto fg = is_selected ? WHITE : BLACK;
     *     ctx.draw_text(text, rect.x + 4, rect.y + 2, fg);
     *
     *     // Focus rect
     *     if (has_focus) {
     *         ctx.draw_rect(rect, BLACK, box_border_style::dotted);
     *     }
     * }
     * @endcode
     */
    virtual void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const typename Backend::rect_type& rect,
        bool is_selected,
        bool has_focus
    ) const = 0;

    /**
     * @brief Get the preferred size for an item
     *
     * @param index Model index of the item
     * @return Preferred size for the item
     *
     * @details
     * Views use this to:
     * - Calculate total content size
     * - Determine item heights in lists
     * - Layout items in grids/tables
     *
     * Return value:
     * - width: Can be 0 for flexible width (use view width)
     * - height: Typically fixed for consistent row heights
     *
     * @par Performance:
     * This may be called frequently (on layout, scrolling, etc.).
     * Consider caching if size calculation is expensive.
     *
     * @par Example:
     * @code
     * size_type size_hint(const model_index& index) const override {
     *     // Fixed height, flexible width
     *     return {0, 24};
     *
     *     // Or measure text
     *     auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);
     *     auto text_data = model->data(index, item_data_role::display);
     *     std::string text = std::get<std::string>(text_data);
     *     int width = measure_text_width(text);
     *     return {width + 8, 24};  // +8 for padding
     * }
     * @endcode
     */
    [[nodiscard]] virtual typename Backend::size_type size_hint(
        const model_index& index
    ) const = 0;

    // ===================================================================
    // Future: In-Place Editing Support
    // ===================================================================

    /**
     * @brief Create an editor widget for editing an item
     * @param parent Parent widget for the editor
     * @param index Model index of the item to edit
     * @return Pointer to editor widget, or nullptr if not editable
     *
     * @details
     * FUTURE: This will be used for in-place editing (double-click to edit).
     * Currently not implemented - requires text_edit, spin_box, etc.
     *
     * When implemented, the delegate should:
     * 1. Check if item is editable (model->flags(index) & item_flag::editable)
     * 2. Create appropriate editor (text_edit, spin_box, combo_box, etc.)
     * 3. Size/position the editor to cover the item
     * 4. Return pointer to the editor
     *
     * The view will:
     * 1. Call create_editor() when editing starts
     * 2. Call set_editor_data() to initialize the editor
     * 3. Wait for editing to complete (Enter key, focus loss, etc.)
     * 4. Call set_model_data() to save changes
     * 5. Delete the editor widget
     */
    // virtual ui_element<Backend>* create_editor(
    //     ui_element<Backend>* parent,
    //     const model_index& index
    // ) const {
    //     return nullptr;  // Not implemented yet
    // }

    /**
     * @brief Set editor data from model
     * @param editor Editor widget created by create_editor()
     * @param index Model index of the item being edited
     *
     * @details
     * FUTURE: Initialize the editor with data from the model.
     * E.g., set text_edit->set_text(model->data(index, edit_role))
     */
    // virtual void set_editor_data(
    //     ui_element<Backend>* editor,
    //     const model_index& index
    // ) const {
    //     // Not implemented yet
    // }

    /**
     * @brief Save editor data to model
     * @param editor Editor widget with user changes
     * @param index Model index of the item being edited
     *
     * @details
     * FUTURE: Save the edited data back to the model.
     * E.g., model->set_data(index, text_edit->text(), edit_role)
     */
    // virtual void set_model_data(
    //     ui_element<Backend>* editor,
    //     const model_index& index
    // ) const {
    //     // Not implemented yet
    // }
};

} // namespace onyxui
