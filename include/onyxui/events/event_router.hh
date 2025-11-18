/**
 * @file event_router.hh
 * @brief Three-phase event routing engine (capture/target/bubble)
 * @author OnyxUI Framework
 * @date 2025-01-04
 *
 * @details
 * Implements the DOM/WPF event routing model with three distinct phases:
 * - CAPTURE: Event travels down from root to target (parent intercepts first)
 * - TARGET: Event delivered to target element
 * - BUBBLE: Event travels up from target to root (child handles first)
 *
 * ## Design Philosophy
 *
 * This router enables composite widgets to intercept events before their children,
 * solving the text_view focus problem where clicking anywhere should give the
 * text_view focus, not just the deepest label.
 */

#pragma once

#include <onyxui/events/ui_event.hh>
#include <onyxui/events/event_phase.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/concepts/backend.hh>

// Forward declare to avoid circular dependency
namespace onyxui {
    template<UIBackend Backend> class ui_element;
}

namespace onyxui {

    /**
     * @brief Route event through three phases (capture/target/bubble)
     * @tparam Backend The UI backend type
     * @param event The event to route
     * @param path The hit test path from root to target
     * @return true if event was handled/consumed by any element
     *
     * @details
     * Routes the event through all three phases in order:
     *
     * 1. **CAPTURE Phase** (forward: root → target)
     *    - Traverses path from index 0 to size-1
     *    - Each element gets event in capture phase
     *    - Parents see event BEFORE children
     *    - If any element returns true, routing stops
     *
     * 2. **TARGET Phase** (at target only)
     *    - Delivers event to target element (path[size-1])
     *    - Most widgets handle events here
     *    - If target returns true, routing stops
     *
     * 3. **BUBBLE Phase** (backward: target → root)
     *    - Traverses path from size-1 to 0
     *    - Each element gets event in bubble phase
     *    - Children handle BEFORE parents
     *    - If any element returns true, routing stops
     *
     * ## Example Flow
     *
     * User clicks at (10, 10) on label inside text_view:
     *
     * ```
     * Widget Tree:
     *   root
     *     └─ text_view
     *          └─ scroll_view
     *               └─ label  ← clicked here
     *
     * Hit Test Path: [root, text_view, scroll_view, label]
     *
     * Event Routing:
     *   1. root->handle_event(evt, CAPTURE)        ← Capture phase
     *   2. text_view->handle_event(evt, CAPTURE)   ← Can request focus here!
     *   3. scroll_view->handle_event(evt, CAPTURE)
     *   4. label->handle_event(evt, TARGET)        ← Target phase
     *   5. scroll_view->handle_event(evt, BUBBLE)  ← Bubble phase
     *   6. text_view->handle_event(evt, BUBBLE)
     *   7. root->handle_event(evt, BUBBLE)
     * ```
     *
     * If text_view handles the capture event and returns true, routing stops
     * and label never sees the event (useful for preventing selection).
     *
     * ## Usage
     *
     * @code
     * // After hit testing
     * hit_test_path<Backend> path;
     * auto* target = root->hit_test(x, y, path);
     *
     * if (target) {
     *     // Route event through three phases
     *     bool handled = route_event(mouse_event, path);
     *
     *     if (handled) {
     *         std::cout << "Event consumed\n";
     *     }
     * }
     * @endcode
     *
     * @note Empty paths are no-ops (returns false)
     * @note Stops immediately if any handler returns true
     * @note Exception safety: No-throw if element handlers don't throw
     *
     * @see event_phase For phase documentation
     * @see hit_test_path For path structure
     */
    template<UIBackend Backend>
    bool route_event(const ui_event& event, const hit_test_path<Backend>& path) {
        if (path.empty()) {
            return false;
        }

        // EVENT ROUTING PHASE 1 OF 3: CAPTURE (root → target, forward iteration)
        // Parents get event BEFORE children
        for (size_t i = 0; i < path.size(); ++i) {
            auto* element = path[i];
            if (element->handle_event(event, event_phase::capture)) {
                return true;  // Event consumed in capture phase
            }
        }

        // EVENT ROUTING PHASE 2 OF 3: TARGET (at target only)
        // Event delivered to the element that was hit
        auto* target = path.target();
        if (target->handle_event(event, event_phase::target)) {
            return true;  // Event consumed at target
        }

        // EVENT ROUTING PHASE 3 OF 3: BUBBLE (target → root, backward iteration)
        // Children handle BEFORE parents
        for (size_t i = path.size(); i > 0; --i) {
            auto* element = path[i - 1];
            if (element->handle_event(event, event_phase::bubble)) {
                return true;  // Event consumed in bubble phase
            }
        }

        return false;  // Event not consumed by any element
    }

    /**
     * @brief Route event with custom phase order (advanced usage)
     * @tparam Backend The UI backend type
     * @param event The event to route
     * @param path The hit test path
     * @param phases Array of phases to execute (in order)
     * @param phase_count Number of phases in array
     * @return true if event was handled
     *
     * @details
     * Allows custom routing patterns for special cases.
     * Most code should use the standard route_event() instead.
     *
     * @code
     * // Route only capture and target (skip bubble)
     * event_phase custom_phases[] = {
     *     event_phase::capture,
     *     event_phase::target
     * };
     * bool handled = route_event_custom(evt, path, custom_phases, 2);
     * @endcode
     */
    template<UIBackend Backend>
    bool route_event_custom(
        const ui_event& event,
        const hit_test_path<Backend>& path,
        const event_phase* phases,
        size_t phase_count)
    {
        if (path.empty()) {
            return false;
        }

        for (size_t p = 0; p < phase_count; ++p) {
            event_phase phase = phases[p];

            switch (phase) {
                case event_phase::capture:
                    // Forward iteration (root → target)
                    for (size_t i = 0; i < path.size(); ++i) {
                        if (path[i]->handle_event(event, phase)) {
                            return true;
                        }
                    }
                    break;

                case event_phase::target:
                    // Target only
                    if (path.target()->handle_event(event, phase)) {
                        return true;
                    }
                    break;

                case event_phase::bubble:
                    // Backward iteration (target → root)
                    for (size_t i = path.size(); i > 0; --i) {
                        if (path[i - 1]->handle_event(event, phase)) {
                            return true;
                        }
                    }
                    break;
            }
        }

        return false;
    }

} // namespace onyxui
