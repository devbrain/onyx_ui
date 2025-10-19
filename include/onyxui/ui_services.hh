/**
 * @file ui_services.hh
 * @brief Global service registry for UI operations
 * @author Assistant
 * @date 2025-10-19
 *
 * @details
 * Provides centralized access to UI services like layer management and focus management.
 * Services are registered once by ui_handle and accessible everywhere through static methods.
 *
 * ## Design Rationale
 *
 * The service locator pattern is used here because:
 * 1. UI frameworks inherently have global state (event loops, windows, etc.)
 * 2. It avoids complex dependency injection through widget hierarchies
 * 3. It's the standard pattern in all major UI frameworks (Qt, ImGui, etc.)
 * 4. It makes testing easier (mock services globally)
 *
 * ## Thread Safety
 *
 * Not thread-safe. All access must be on UI thread. If thread-local storage is needed,
 * change `static inline` to `static inline thread_local`.
 *
 * ## Usage Example
 *
 * @code
 * // Registration (done by ui_handle)
 * ui_services<Backend>::set_layer_manager(&layer_mgr);
 * ui_services<Backend>::set_focus_manager(&focus_mgr);
 *
 * // Access (done by widgets)
 * auto* layers = ui_services<Backend>::layers();
 * if (layers) {
 *     layers->show_popup(...);
 * }
 *
 * // Cleanup (done by ui_handle destructor)
 * ui_services<Backend>::clear();
 * @endcode
 */

#pragma once

#include <onyxui/concepts/backend.hh>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class layer_manager;
    template<UIBackend Backend> class ui_element;
    template<UIBackend Backend, typename ElementType> class hierarchical_focus_manager;

    /**
     * @class ui_services
     * @brief Global service registry for UI operations
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Provides static access to commonly-needed UI services without requiring
     * dependency injection through the widget hierarchy. Services are registered
     * once by ui_handle and accessible everywhere.
     *
     * ## Services Provided
     *
     * - **layer_manager**: Overlay management (popups, tooltips, dialogs)
     * - **focus_manager**: Focus navigation and management
     *
     * ## Lifetime Management
     *
     * Services are registered when ui_handle is created and cleared when it's destroyed.
     * Accessing services after ui_handle destruction returns nullptr (safe).
     *
     * ## Why Service Locator?
     *
     * Alternative approaches were considered:
     * - Context propagation through hierarchy: Complex, lots of boilerplate
     * - Dependency injection: Over-engineered for this use case
     * - Mixin traits: Multiple inheritance complexity
     *
     * Service locator provides the best balance of simplicity and usability for UI code.
     *
     * @example Basic Usage
     * @code
     * // In widget code
     * void button::show_help_tooltip() {
     *     auto* layers = ui_services<Backend>::layers();
     *     if (layers) {
     *         layers->show_tooltip(create_help_content(), this->bounds());
     *     }
     * }
     * @endcode
     *
     * @example Testing
     * @code
     * TEST_CASE("Widget with tooltip") {
     *     mock_layer_manager<Backend> mock;
     *     ui_services<Backend>::set_layer_manager(&mock);
     *
     *     widget->show_tooltip("Test");
     *
     *     CHECK(mock.tooltip_shown);
     *
     *     ui_services<Backend>::clear();
     * }
     * @endcode
     */
    template<UIBackend Backend>
    class ui_services {
    private:
        // Service storage (inline static = one instance per template instantiation)
        static inline layer_manager<Backend>* s_layer_manager = nullptr;
        static inline hierarchical_focus_manager<Backend, ui_element<Backend>>* s_focus_manager = nullptr;

    public:
        // ================================================================
        // Service Registration (called by ui_handle)
        // ================================================================

        /**
         * @brief Register the layer manager service
         * @param mgr Pointer to layer manager (non-owning)
         *
         * @details
         * Called by ui_handle constructor. The pointer is non-owning - ui_handle
         * retains ownership of the layer manager.
         */
        static void set_layer_manager(layer_manager<Backend>* mgr) noexcept {
            s_layer_manager = mgr;
        }

        /**
         * @brief Register the focus manager service
         * @param mgr Pointer to focus manager (non-owning)
         *
         * @details
         * Called by ui_handle constructor. The pointer is non-owning - ui_handle
         * retains ownership of the focus manager.
         */
        static void set_focus_manager(hierarchical_focus_manager<Backend, ui_element<Backend>>* mgr) noexcept {
            s_focus_manager = mgr;
        }

        // ================================================================
        // Service Access (used by widgets)
        // ================================================================

        /**
         * @brief Get the layer manager
         * @return Pointer to layer manager, or nullptr if not registered
         *
         * @details
         * Returns nullptr if ui_handle hasn't been created yet or has been destroyed.
         * Widgets should check for nullptr before using.
         *
         * @example
         * @code
         * auto* layers = ui_services<Backend>::layers();
         * if (layers) {
         *     layers->show_popup(content, bounds);
         * }
         * @endcode
         */
        [[nodiscard]] static layer_manager<Backend>* layers() noexcept {
            return s_layer_manager;
        }

        /**
         * @brief Get the focus manager
         * @return Pointer to focus manager, or nullptr if not registered
         *
         * @details
         * Returns nullptr if ui_handle hasn't been created yet or has been destroyed.
         * Widgets should check for nullptr before using.
         *
         * @example
         * @code
         * auto* focus = ui_services<Backend>::focus();
         * if (focus) {
         *     focus->set_focus(this);
         * }
         * @endcode
         */
        [[nodiscard]] static hierarchical_focus_manager<Backend, ui_element<Backend>>* focus() noexcept {
            return s_focus_manager;
        }

        // ================================================================
        // Cleanup (called by ui_handle destructor)
        // ================================================================

        /**
         * @brief Clear all registered services
         *
         * @details
         * Called by ui_handle destructor. Sets all service pointers to nullptr.
         * After this call, all service access will return nullptr (safe).
         */
        static void clear() noexcept {
            s_layer_manager = nullptr;
            s_focus_manager = nullptr;
        }
    };

} // namespace onyxui
