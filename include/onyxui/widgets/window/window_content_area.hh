/**
 * @file window_content_area.hh
 * @brief Window content container with optional scrolling
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * The content area is a simple container that:
 * - Holds the user's content widget
 * - Optionally wraps it in a scroll_view (if scrollable)
 * - Provides content area bounds for window layout
 */

#pragma once

#include <onyxui/widgets/core/widget_container.hh>
#include <memory>

namespace onyxui {

    // Forward declaration
    template<UIBackend Backend> class scroll_view;

    /**
     * @class window_content_area
     * @brief Content container for window widget
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A simple container that holds the window's content widget.
     * If scrollable, wraps content in a scroll_view for automatic scrolling.
     *
     * ## Layout
     *
     * Non-scrollable:
     * ```
     * ┌──────────────┐
     * │ [Content]    │
     * │              │
     * └──────────────┘
     * ```
     *
     * Scrollable:
     * ```
     * ┌──────────────┐
     * │ [ScrollView] │
     * │   [Content]  │ ║
     * └──────────────┘
     * ```
     *
     * @example
     * @code
     * auto content_area = std::make_unique<window_content_area<Backend>>(false);
     *
     * auto content = std::make_unique<vbox<Backend>>();
     * content->emplace_child<label>("Hello");
     * content_area->set_content(std::move(content));
     * @endcode
     */
    template<UIBackend Backend>
    class window_content_area : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using render_context_type = render_context<Backend>;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a content area
         * @param scrollable Whether to wrap content in scroll_view
         * @param parent Parent element (typically the window)
         */
        explicit window_content_area(
            bool scrollable = false,
            ui_element<Backend>* parent = nullptr
        );

        /**
         * @brief Destructor
         */
        ~window_content_area() override = default;

        // Rule of Five
        window_content_area(const window_content_area&) = delete;
        window_content_area& operator=(const window_content_area&) = delete;
        window_content_area(window_content_area&&) noexcept = default;
        window_content_area& operator=(window_content_area&&) noexcept = default;

        // ====================================================================
        // Border Management
        // ====================================================================

        /**
         * @brief Set whether content area has a border
         * @param has_border True to draw border around content area
         */
        void set_has_border(bool has_border) {
            this->m_has_border = has_border;
        }

        // ====================================================================
        // Content Management
        // ====================================================================

        /**
         * @brief Set content widget
         * @param content Content widget (takes ownership)
         *
         * @details
         * If scrollable, content is added to internal scroll_view.
         * Otherwise, content is added directly as child.
         */
        void set_content(std::unique_ptr<ui_element<Backend>> content);

        /**
         * @brief Get content widget
         * @return Pointer to content widget (nullptr if none)
         */
        [[nodiscard]] ui_element<Backend>* get_content() noexcept {
            return m_content;
        }

        // ====================================================================
        // Scrolling
        // ====================================================================

        /**
         * @brief Set scrollable flag
         * @param scrollable Whether to enable scrolling
         *
         * @details
         * If changing from non-scrollable to scrollable, existing content
         * will be moved into a scroll_view.
         */
        void set_scrollable(bool scrollable);

        /**
         * @brief Check if scrollable
         */
        [[nodiscard]] bool is_scrollable() const noexcept {
            return m_scrollable;
        }

        /**
         * @brief Get scroll view (nullptr if not scrollable)
         */
        [[nodiscard]] scroll_view<Backend>* get_scroll_view() noexcept {
            return m_scroll_view;
        }

    protected:
        /**
         * @brief Render content area
         * @param ctx Render context
         */
        void do_render(render_context_type& ctx) const override;

        /**
         * @brief Get theme-specific style for content area (Phase 8)
         * @param theme Theme to extract properties from
         * @return Resolved style with content area background color
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override;

    private:
        bool m_scrollable = false;
        scroll_view<Backend>* m_scroll_view = nullptr;  // Non-owning (managed by children list)
        ui_element<Backend>* m_content = nullptr;       // Non-owning (managed by children list or scroll_view)
    };

} // namespace onyxui

// Implementation
#include <onyxui/widgets/window/window_content_area.inl>
