/**
 * @file event_phase.hh
 * @brief Event propagation phases for capture/bubble routing
 * @author OnyxUI Framework
 * @date 2025-01-04
 *
 * @details
 * Defines the three phases of event propagation through the widget tree,
 * following the industry-standard DOM/WPF event routing model.
 */

#pragma once

#include <cstdint>

namespace onyxui {

    /**
     * @enum event_phase
     * @brief Phase of event propagation through the widget tree
     *
     * @details
     * Follows the DOM/WPF event routing model with three distinct phases:
     *
     * 1. **CAPTURE**: Event travels DOWN from root to target (parent first)
     *    - Ancestors get event before children
     *    - Used by composite widgets to intercept/preprocess events
     *    - Example: text_view captures clicks to request focus
     *
     * 2. **TARGET**: Event delivered to target element
     *    - Event at the element returned by hit_test()
     *    - Most widgets handle events in this phase
     *    - Example: button handles click to trigger action
     *
     * 3. **BUBBLE**: Event travels UP from target to root (child first)
     *    - Ancestors get event after children
     *    - Used for event delegation and cleanup
     *    - Example: panel logs all child interactions
     *
     * ## Example Flow
     *
     * User clicks at (10, 10) on a label inside text_view:
     *
     * ```
     * Widget Tree:
     *   root
     *     └─ panel
     *          └─ text_view
     *               └─ scroll_view
     *                    └─ label  ← clicked here
     *
     * Event Flow:
     *   1. root->handle_event(CAPTURE)       ← Capture phase
     *   2. panel->handle_event(CAPTURE)
     *   3. text_view->handle_event(CAPTURE)  ← Can request focus here
     *   4. scroll_view->handle_event(CAPTURE)
     *   5. label->handle_event(TARGET)       ← Target phase
     *   6. scroll_view->handle_event(BUBBLE) ← Bubble phase
     *   7. text_view->handle_event(BUBBLE)
     *   8. panel->handle_event(BUBBLE)
     *   9. root->handle_event(BUBBLE)
     * ```
     *
     * Any handler can stop propagation by returning `true`.
     */
    enum class event_phase : std::uint8_t {
        /**
         * @brief Capture phase - event traveling down to target
         *
         * Handlers in this phase see events BEFORE children.
         * Used by composite widgets to intercept/preprocess events.
         *
         * **When to use:**
         * - Composite widgets requesting focus
         * - Input validation before child processes
         * - Event filtering/transformation
         *
         * **Example:**
         * ```cpp
         * bool handle_event(const ui_event& event, event_phase phase) override {
         *     if (phase == event_phase::capture) {
         *         if (is_mouse_press(event)) {
         *             request_focus();  // Before children handle
         *             return false;     // Let children handle too
         *         }
         *     }
         *     return base::handle_event(event, phase);
         * }
         * ```
         */
        capture = 0,

        /**
         * @brief Target phase - event at target element
         *
         * Event delivered to the element returned by hit_test().
         * Most widgets handle events in this phase.
         *
         * **When to use:**
         * - Button clicks
         * - Text input
         * - Direct widget interaction
         *
         * **Example:**
         * ```cpp
         * bool handle_event(const ui_event& event, event_phase phase) override {
         *     if (phase == event_phase::target) {
         *         if (is_mouse_press(event)) {
         *             on_click();
         *             return true;  // Consumed
         *         }
         *     }
         *     return base::handle_event(event, phase);
         * }
         * ```
         */
        target = 1,

        /**
         * @brief Bubble phase - event traveling up from target
         *
         * Handlers in this phase see events AFTER children.
         * Used for event delegation and logging patterns.
         *
         * **When to use:**
         * - Event logging/analytics
         * - Cleanup after child processing
         * - Event delegation patterns
         *
         * **Example:**
         * ```cpp
         * bool handle_event(const ui_event& event, event_phase phase) override {
         *     if (phase == event_phase::bubble) {
         *         log_user_interaction(event);
         *         return false;  // Don't consume
         *     }
         *     return base::handle_event(event, phase);
         * }
         * ```
         */
        bubble = 2
    };

    /**
     * @brief Convert event_phase to string for debugging
     * @param phase The event phase to convert
     * @return String representation of the phase
     *
     * @details
     * Returns lowercase string names for each phase:
     * - capture → "capture"
     * - target  → "target"
     * - bubble  → "bubble"
     *
     * Useful for debug logging and error messages.
     *
     * @code
     * std::cerr << "Event in " << to_string(phase) << " phase\n";
     * @endcode
     */
    constexpr const char* to_string(event_phase phase) noexcept {
        switch (phase) {
            case event_phase::capture: return "capture";
            case event_phase::target: return "target";
            case event_phase::bubble: return "bubble";
        }
        return "unknown";
    }

} // namespace onyxui
