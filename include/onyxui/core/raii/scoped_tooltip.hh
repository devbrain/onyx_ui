/**
 * @file scoped_tooltip.hh
 * @brief RAII wrapper for text tooltips that owns both content and layer
 * @author Assistant
 * @date 2025-10-19
 *
 * @details
 * Specialized version of scoped_layer that also owns the tooltip content (label).
 * This allows simple usage like `show_tooltip_scoped("Help text")` without
 * the caller needing to manage the label lifetime.
 */

#pragma once

#include <core/raii/scoped_layer.hh>
#include <onyxui/widgets/label.hh>
#include <memory>
#include <string>

namespace onyxui {

    /**
     * @class scoped_tooltip
     * @brief RAII wrapper for text tooltips with automatic content and layer cleanup
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Combines ownership of:
     * 1. The tooltip content (a label widget)
     * 2. The layer displaying the tooltip
     *
     * Both are automatically cleaned up when scoped_tooltip is destroyed or reassigned.
     *
     * @example
     * @code
     * class help_button : public button<Backend> {
     *     scoped_tooltip<Backend> m_tooltip;
     *
     *     void on_mouse_enter() override {
     *         m_tooltip = scoped_tooltip<Backend>("Click for help", this->bounds());
     *     }
     * };
     * @endcode
     */
    template<UIBackend Backend>
    class scoped_tooltip {
    private:
        std::unique_ptr<label<Backend>> m_content;
        scoped_layer<Backend> m_layer;

    public:
        scoped_tooltip() = default;

        /**
         * @brief Create tooltip with text and position
         * @param text Tooltip text
         * @param anchor_bounds Widget bounds to position tooltip below
         */
        scoped_tooltip(const std::string& text, const typename Backend::rect_type& anchor_bounds) {
            auto* layers = ui_services<Backend>::layers();
            if (!layers) return;

            // Create label for tooltip
            m_content = std::make_unique<label<Backend>>(text);
            m_content->set_padding(4);

            // Position below widget
            int x = rect_utils::get_x(anchor_bounds);
            int y = rect_utils::get_y(anchor_bounds) + rect_utils::get_height(anchor_bounds);

            // Show tooltip
            layer_id id = layers->show_tooltip(m_content.get(), x, y);
            m_layer = scoped_layer<Backend>(layers, id);
        }

        // Move-only
        scoped_tooltip(scoped_tooltip&&) noexcept = default;
        scoped_tooltip& operator=(scoped_tooltip&&) noexcept = default;

        // No copy
        scoped_tooltip(const scoped_tooltip&) = delete;
        scoped_tooltip& operator=(const scoped_tooltip&) = delete;

        void close() noexcept {
            m_layer.close();
            m_content.reset();
        }

        [[nodiscard]] bool is_valid() const noexcept {
            return m_layer.is_valid();
        }
    };

} // namespace onyxui
