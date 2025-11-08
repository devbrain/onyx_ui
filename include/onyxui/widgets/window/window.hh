/**
 * @file window.hh
 * @brief Draggable, resizable window widget with optional title bar
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * Provides a complete windowing system with:
 * - Optional themeable title bar with minimize/maximize/close buttons
 * - Draggable and resizable behavior
 * - Modal and non-modal support
 * - Minimize/maximize/restore state management
 * - Integration with window_manager service
 */

#pragma once

#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/services/ui_services.hh>
#include <memory>
#include <string>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class window_title_bar;
    template<UIBackend Backend> class window_content_area;

    /**
     * @class window
     * @brief Main window widget with draggable title bar and content area
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A window is a top-level UI container that can be:
     * - Dragged by its title bar
     * - Resized from edges/corners
     * - Minimized, maximized, restored
     * - Shown as modal or non-modal
     * - Optionally scrollable
     *
     * ## Window States
     *
     * - **normal**: Standard window state
     * - **minimized**: Hidden from view, tracked in window_manager
     * - **maximized**: Fills parent container
     *
     * ## Window Flags
     *
     * Configure window features via `window_flags` structure:
     * - Title bar visibility and buttons
     * - Resizability and draggability
     * - Modal behavior
     * - Scrollable content
     *
     * ## Signals
     *
     * - `closing`: Emitted before close (can cancel)
     * - `closed`: Emitted after close
     * - `minimized`, `maximized`, `restored`: State change signals
     * - `moved`, `resized`: Position/size change signals
     * - `focus_gained`, `focus_lost`: Focus change signals
     *
     * @example
     * @code
     * // Create simple window
     * auto win = std::make_unique<window<Backend>>("My Application");
     *
     * // Add content
     * auto content = std::make_unique<vbox<Backend>>();
     * content->emplace_child<label>("Hello, World!");
     * win->set_content(std::move(content));
     *
     * // Show window
     * win->show();
     * @endcode
     */
    template<UIBackend Backend>
    class window : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;
        using render_context_type = render_context<Backend>;

        /**
         * @brief Window state enumeration
         */
        enum class window_state : uint8_t {
            normal,      ///< Standard window state
            minimized,   ///< Hidden from view
            maximized    ///< Fills parent container
        };

        /**
         * @brief Window configuration flags
         */
        struct window_flags {
            bool has_title_bar = true;          ///< Show title bar
            bool has_menu_button = false;       ///< Show menu button in title bar
            bool has_minimize_button = true;    ///< Show minimize button
            bool has_maximize_button = true;    ///< Show maximize button
            bool has_close_button = true;       ///< Show close button
            bool is_resizable = true;           ///< Allow resizing from edges/corners
            bool is_movable = true;             ///< Allow dragging by title bar
            bool is_scrollable = false;         ///< Wrap content in scroll_view
            bool is_modal = false;              ///< Show as modal (blocks other windows)
            bool dim_background = false;        ///< Dim background when modal (optional)
        };

        /**
         * @brief Construct a window
         * @param title Window title text
         * @param flags Window configuration flags
         * @param parent Parent element (nullptr for none)
         */
        explicit window(
            std::string title = "",
            window_flags flags = {},
            ui_element<Backend>* parent = nullptr
        );

        /**
         * @brief Destructor - unregisters from window_manager
         */
        ~window() override;

        // Rule of Five
        window(const window&) = delete;
        window& operator=(const window&) = delete;
        window(window&&) noexcept = default;
        window& operator=(window&&) noexcept = default;

        // ====================================================================
        // State Management
        // ====================================================================

        /**
         * @brief Minimize window (hides and registers as minimized)
         */
        void minimize();

        /**
         * @brief Maximize window (fills parent container)
         */
        void maximize();

        /**
         * @brief Restore window from minimized or maximized state
         */
        void restore();

        /**
         * @brief Close window (emits closing/closed signals)
         */
        void close();

        /**
         * @brief Get current window state
         */
        [[nodiscard]] window_state get_state() const noexcept {
            return m_state;
        }

        // ====================================================================
        // Position and Size
        // ====================================================================

        /**
         * @brief Set window position
         * @param x Horizontal position
         * @param y Vertical position
         */
        void set_position(int x, int y);

        /**
         * @brief Set window size
         * @param width Window width
         * @param height Window height
         */
        void set_size(int width, int height);

        /**
         * @brief Get normal (non-maximized) bounds for restore
         */
        [[nodiscard]] rect_type get_normal_bounds() const noexcept {
            return m_normal_bounds;
        }

        // ====================================================================
        // Content Management
        // ====================================================================

        /**
         * @brief Set window content widget
         * @param content Content widget (takes ownership)
         */
        void set_content(std::unique_ptr<ui_element<Backend>> content);

        /**
         * @brief Get content widget
         */
        [[nodiscard]] ui_element<Backend>* get_content() noexcept;

        // ====================================================================
        // Title
        // ====================================================================

        /**
         * @brief Set window title
         * @param title New title text
         */
        void set_title(const std::string& title);

        /**
         * @brief Get window title
         */
        [[nodiscard]] const std::string& get_title() const noexcept {
            return m_title;
        }

        // ====================================================================
        // Display
        // ====================================================================

        /**
         * @brief Show window (adds to layer_manager)
         */
        void show();

        /**
         * @brief Show window as modal (blocks other windows)
         */
        void show_modal();

        /**
         * @brief Hide window (removes from layer_manager)
         */
        void hide();

        // ====================================================================
        // Signals
        // ====================================================================

        signal<> closing;        ///< Emitted before close (can cancel)
        signal<> closed;         ///< Emitted after close
        signal<> minimized_sig;  ///< Emitted after minimize
        signal<> maximized_sig;  ///< Emitted after maximize
        signal<> restored_sig;   ///< Emitted after restore
        signal<> moved;          ///< Emitted after position changes
        signal<> resized_sig;    ///< Emitted after size changes
        signal<> focus_gained;   ///< Emitted when window gains focus
        signal<> focus_lost;     ///< Emitted when window loses focus

    protected:
        /**
         * @brief Render window (title bar + content area + border)
         * @param ctx Render context
         */
        void do_render(render_context_type& ctx) const override;

        /**
         * @brief Handle events (for future drag/resize implementation)
         * @param event UI event
         * @param phase Event phase
         */
        bool handle_event(const ui_event& event, event_phase phase) override;

    private:
        // Window properties
        std::string m_title;
        window_flags m_flags;
        window_state m_state = window_state::normal;
        rect_type m_normal_bounds{};      // For restore from maximized
        rect_type m_before_minimize{};    // For restore from minimized

        // Child widgets (Phase 1: created but drag/resize not implemented yet)
        std::unique_ptr<window_title_bar<Backend>> m_title_bar;
        std::unique_ptr<window_content_area<Backend>> m_content_area;

        // Helper methods
        void register_with_window_manager();
        void unregister_from_window_manager();
    };

} // namespace onyxui

// Implementation
#include "window.inl"
