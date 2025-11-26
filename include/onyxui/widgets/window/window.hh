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
#include <onyxui/services/layer_manager.hh>  // For layer_id and layer_type
#include <onyxui/core/raii/scoped_layer.hh>  // For system menu popup
#include <memory>
#include <string>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class window_title_bar;
    template<UIBackend Backend> class window_content_area;
    template<UIBackend Backend> class window_system_menu;

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
    class window : public widget_container<Backend> {  // NOLINT(cppcoreguidelines-virtual-class-destructor) - destructor is correctly virtual via override
    public:
        using base = widget_container<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;
        using render_context_type = render_context<Backend>;
        using theme_type = typename base::theme_type;

        /**
         * @brief Window state enumeration
         */
        enum class window_state : uint8_t {
            normal,      ///< Standard window state
            minimized,   ///< Hidden from view
            maximized    ///< Fills parent container
        };

        /**
         * @brief Resize handle zones for window edges and corners
         * @details Identifies which edge or corner the user is dragging for resize operations
         */
        enum class resize_handle : uint8_t {
            none,        ///< Not on a resize handle
            north,       ///< Top edge
            south,       ///< Bottom edge
            east,        ///< Right edge
            west,        ///< Left edge
            north_east,  ///< Top-right corner
            north_west,  ///< Top-left corner
            south_east,  ///< Bottom-right corner
            south_west   ///< Bottom-left corner
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

            // Size constraints for window resize
            int min_width = 100;                ///< Minimum window width
            int min_height = 50;                ///< Minimum window height
            int max_width = 0;                  ///< Maximum window width (0 = no limit)
            int max_height = 0;                 ///< Maximum window height (0 = no limit)
            int resize_border_width = 4;        ///< Width of resize border zone
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

        /**
         * @brief Auto-size window to fit its content
         *
         * @details
         * Measures the content widget's natural size and resizes the window
         * to accommodate it. This is useful for dialogs and utility windows
         * that should be sized based on their content rather than manually.
         *
         * The method:
         * 1. Measures the content widget's natural size
         * 2. Adds space for window chrome (title bar, borders, scrollbars)
         * 3. Respects min/max size constraints from window_flags
         * 4. Updates window size via set_size()
         *
         * @note Call this after setting window content and before show()
         * @note For dynamic content, call whenever content size changes
         *
         * @example
         * @code
         * // Create window with content
         * auto win = std::make_unique<window<Backend>>("Dialog");
         * auto content = std::make_unique<vbox<Backend>>();
         * content->emplace_child<label>("Message text");
         * content->emplace_child<button>("OK");
         * win->set_content(std::move(content));
         *
         * // Auto-size to fit content
         * win->fit_content();
         *
         * // Show window
         * win->show();
         * @endcode
         */
        void fit_content();

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

        /**
         * @brief Check if window is modal
         * @return true if window is modal (blocks other windows)
         */
        [[nodiscard]] bool is_modal() const noexcept {
            return m_flags.is_modal;
        }

        /**
         * @brief Get window flags
         * @return Window configuration flags
         */
        [[nodiscard]] const window_flags& get_flags() const noexcept {
            return m_flags;
        }

        // ====================================================================
        // Focus Management
        // ====================================================================

        // Bring base class keyboard focus methods into scope (prevents name hiding)
        using event_target<Backend>::has_focus;      // Keyboard focus from focus_manager
        using event_target<Backend>::is_focusable;
        using event_target<Backend>::set_focusable;

        /**
         * @brief Set window-level focus state (active/foreground window)
         * @param focused Whether window is the active/foreground window
         *
         * @details
         * This controls window-level focus (which window is active), which is
         * distinct from keyboard focus (which element receives keyboard input).
         * - Window focus: Managed by window_manager (active window)
         * - Keyboard focus: Managed by focus_manager (focused element)
         */
        void set_window_focus(bool focused);

        /**
         * @brief Check if this is the active/foreground window
         * @return True if window is active/foreground
         *
         * @details
         * This checks window-level focus, NOT keyboard focus.
         * - has_window_focus() → Is this the active window?
         * - has_focus() → Does this element have keyboard focus? (from event_target)
         */
        [[nodiscard]] bool has_window_focus() const noexcept {
            return m_has_focus;
        }

        // ====================================================================
        // Testing Accessors (internal implementation details exposed for testing)
        // ====================================================================

        /**
         * @brief Get system menu (for testing)
         * @return Pointer to system menu, or nullptr if not enabled
         */
        [[nodiscard]] window_system_menu<Backend>* get_system_menu() noexcept {
            return m_system_menu.get();
        }

        /**
         * @brief Get content area (for testing)
         * @return Pointer to window content area
         */
        [[nodiscard]] window_content_area<Backend>* get_content_area() noexcept {
            return m_content_area;
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

        /**
         * @brief Set workspace area for maximize bounds
         * @param workspace Workspace element (e.g., main_window::central_widget())
         *
         * @details
         * Sets a workspace reference for floating layer windows.
         * When maximized, the window will fill the workspace bounds instead
         * of the entire viewport. This allows windows to respect UI chrome
         * (menu bars, status bars, etc.) without being parented to the workspace.
         *
         * **Key difference from parent:**
         * - **Parent**: Window becomes child in widget tree, uses relative coords,
         *   events route through widget hierarchy
         * - **Workspace**: Window remains independent floating layer, uses absolute
         *   coords, events route through layer_manager, but maximizes to workspace area
         *
         * **Maximize behavior priority**: parent > workspace > viewport
         *
         * **Use case**: Floating windows in main_window applications
         * @code
         * auto main = std::make_unique<main_window<Backend>>();
         * auto win = std::make_shared<window<Backend>>("Tool", flags);
         * win->set_workspace(main->central_widget());  // Respect workspace
         * win->show();  // Still a floating layer
         * win->maximize();  // Fills central_widget area, not entire screen
         * @endcode
         */
        void set_workspace(ui_element<Backend>* workspace) noexcept {
            m_workspace = workspace;
        }

        /**
         * @brief Get workspace area (if set)
         * @return Workspace element, or nullptr if not set
         */
        [[nodiscard]] ui_element<Backend>* get_workspace() const noexcept {
            return m_workspace;
        }

        /**
         * @brief Bring window to front and request focus
         *
         * @details
         * Moves window to highest z-order in layer_manager and
         * requests keyboard focus from focus_manager. Also sets
         * this window as active in window_manager.
         */
        void bring_to_front();

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

        // Resize helpers (protected for testing and subclass customization)
        resize_handle get_resize_handle_at(int x, int y) const;
        void apply_size_constraints(rect_type& bounds) const;

        /**
         * @brief Hook called when window is closing (Phase 5)
         * @details
         * Override this in subclasses to add custom close behavior.
         * Called after closing signal but before hide() and closed signal.
         */
        virtual void on_close() {
            // Default: no-op
        }

        /**
         * @brief Get theme-specific style for window (Phase 8)
         * @param theme Theme to extract properties from
         * @return Resolved style with window border colors based on focus state
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override;

    private:
        // Window properties
        std::string m_title;
        window_flags m_flags;
        window_state m_state = window_state::normal;
        rect_type m_normal_bounds{};      // For restore from maximized
        rect_type m_before_minimize{};    // For restore from minimized

        // Phase 8: Focus state
        bool m_has_focus = false;

        // Phase 2: Drag state
        rect_type m_drag_initial_bounds{};  // Window bounds when drag started

        // Phase 3: Resize state
        bool m_is_resizing = false;         // Currently resizing
        resize_handle m_resize_handle = resize_handle::none;  // Which handle is being dragged
        rect_type m_resize_initial_bounds{};  // Window bounds when resize started
        int m_resize_start_x = 0;           // Mouse X when resize started
        int m_resize_start_y = 0;           // Mouse Y when resize started

        // Phase 5: Layer manager integration
        layer_id m_layer_id{};              // Layer ID when shown in layer_manager
        std::shared_ptr<ui_element<Backend>> m_layer_handle;  // Keeps weak_ptr in layer_manager alive
        window<Backend>* m_previous_active_window = nullptr;  // For restoring focus after modal closes

        // Workspace reference (not parent - just for maximize bounds)
        ui_element<Backend>* m_workspace = nullptr;  // Optional workspace area for maximize bounds

        // Child widgets (Phase 1: created but drag/resize not implemented yet)
        // Note: Raw pointers - ownership transferred to base class children list
        window_title_bar<Backend>* m_title_bar = nullptr;
        window_content_area<Backend>* m_content_area = nullptr;

        // Phase 7: System menu (owned by window, not a child widget)
        std::unique_ptr<window_system_menu<Backend>> m_system_menu;
        scoped_layer<Backend> m_system_menu_layer;  // RAII popup layer for system menu
        scoped_connection m_system_menu_closing_connection;  // Connection to menu's closing signal

        // Helper methods
        void register_with_window_manager();
        void unregister_from_window_manager();
    };

} // namespace onyxui

// Implementation
#include <onyxui/widgets/window/window.inl>
