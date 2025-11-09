/**
 * @file window_title_bar.hh
 * @brief Composite window title bar with title text and control buttons
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * The title bar is a horizontal composite widget containing:
 * - Optional menu button (left side)
 * - Title label (center, expands to fill)
 * - Optional minimize button
 * - Optional maximize/restore button
 * - Optional close button
 */

#pragma once

#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/layout/linear_layout.hh>
#include <string>
#include <memory>

namespace onyxui {

    // Forward declaration
    template<UIBackend Backend> class window;

    /**
     * @class window_title_bar
     * @brief Title bar for window widget
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A horizontal container with:
     * - Title text (left or center aligned)
     * - Control buttons (right aligned)
     *
     * Layout:
     * ```
     * [Menu?] [Title                    ] [_] [□] [X]
     * ```
     *
     * ## Signals
     *
     * - `menu_clicked`: Menu button clicked (if enabled)
     * - `minimize_clicked`: Minimize button clicked
     * - `maximize_clicked`: Maximize button clicked
     * - `close_clicked`: Close button clicked
     *
     * ## Phase 1 Implementation
     *
     * - Basic rendering and button signals
     * - Drag functionality will be added in Phase 2
     *
     * @example
     * @code
     * auto title_bar = std::make_unique<window_title_bar<Backend>>(
     *     "My Window",
     *     window_flags
     * );
     *
     * title_bar->close_clicked.connect([&]() {
     *     window->close();
     * });
     * @endcode
     */
    template<UIBackend Backend>
    class window_title_bar : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using render_context_type = render_context<Backend>;
        using window_flags = typename window<Backend>::window_flags;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a title bar
         * @param title Title text
         * @param flags Window flags (determines which buttons to show)
         * @param parent Parent element (typically the window)
         */
        explicit window_title_bar(
            std::string title,
            const window_flags& flags,
            ui_element<Backend>* parent = nullptr
        );

        /**
         * @brief Destructor
         */
        ~window_title_bar() override = default;

        // Rule of Five
        window_title_bar(const window_title_bar&) = delete;
        window_title_bar& operator=(const window_title_bar&) = delete;
        window_title_bar(window_title_bar&&) noexcept = default;
        window_title_bar& operator=(window_title_bar&&) noexcept = default;

        // ====================================================================
        // Title
        // ====================================================================

        /**
         * @brief Set title text
         * @param title New title text
         */
        void set_title(const std::string& title);

        /**
         * @brief Get title text
         */
        [[nodiscard]] const std::string& get_title() const noexcept {
            return m_title;
        }

        // ====================================================================
        // Button Access
        // ====================================================================

        /**
         * @brief Get menu button (nullptr if not enabled)
         */
        [[nodiscard]] button<Backend>* get_menu_button() noexcept {
            return m_menu_button;
        }

        /**
         * @brief Get minimize button (nullptr if not enabled)
         */
        [[nodiscard]] button<Backend>* get_minimize_button() noexcept {
            return m_minimize_button;
        }

        /**
         * @brief Get maximize button (nullptr if not enabled)
         */
        [[nodiscard]] button<Backend>* get_maximize_button() noexcept {
            return m_maximize_button;
        }

        /**
         * @brief Get close button (nullptr if not enabled)
         */
        [[nodiscard]] button<Backend>* get_close_button() noexcept {
            return m_close_button;
        }

        // ====================================================================
        // Signals
        // ====================================================================

        signal<> menu_clicked;        ///< Menu button clicked
        signal<> minimize_clicked;    ///< Minimize button clicked
        signal<> maximize_clicked;    ///< Maximize button clicked
        signal<> close_clicked;       ///< Close button clicked

        // Phase 2: Drag signals
        signal<> drag_started;        ///< Mouse drag started on title bar
        signal<int, int> dragging;    ///< Mouse dragging (delta_x, delta_y from start)
        signal<> drag_ended;          ///< Mouse drag ended

    protected:
        /**
         * @brief Render title bar
         * @param ctx Render context
         */
        void do_render(render_context_type& ctx) const override;

        /**
         * @brief Handle mouse events for dragging (Phase 2)
         * @param event UI event
         * @param phase Event phase
         */
        bool handle_event(const ui_event& event, event_phase phase) override;

        /**
         * @brief Get theme-specific style for title bar (Phase 8)
         * @param theme Theme to extract properties from
         * @return Resolved style with title bar colors based on parent window focus state
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override;

    private:
        std::string m_title;

        // Child widgets (pointers owned by base class children list)
        label<Backend>* m_title_label = nullptr;
        button<Backend>* m_menu_button = nullptr;
        button<Backend>* m_minimize_button = nullptr;
        button<Backend>* m_maximize_button = nullptr;
        button<Backend>* m_close_button = nullptr;

        // Phase 2: Drag state tracking
        bool m_is_dragging = false;
        int m_drag_start_x = 0;
        int m_drag_start_y = 0;

        // Helper: Create and wire up buttons
        void create_buttons(const window_flags& flags);
    };

} // namespace onyxui

// Implementation
#include "window_title_bar.inl"
