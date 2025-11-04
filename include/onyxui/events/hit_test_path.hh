/**
 * @file hit_test_path.hh
 * @brief Path from root to target element for event routing
 * @author OnyxUI Framework
 * @date 2025-01-04
 *
 * @details
 * Stores the complete path from root to target element discovered during hit testing.
 * Used by the event routing engine to implement three-phase event propagation
 * (capture/target/bubble).
 */

#pragma once

#include <vector>
#include <cstddef>
#include <onyxui/concepts/backend.hh>

namespace onyxui {

    // Forward declaration
    template<UIBackend Backend>
    class ui_element;

    /**
     * @class hit_test_path
     * @brief Stores the routing path from root to target element
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * During hit testing, elements record themselves as the path traverses
     * from root to target. The resulting path enables three-phase event routing:
     *
     * - **Capture Phase**: Traverse forward (root → target)
     * - **Target Phase**: Deliver to target (last element)
     * - **Bubble Phase**: Traverse backward (target → root)
     *
     * ## Example Path
     *
     * User clicks at (10, 10) on a nested widget:
     *
     * ```
     * Widget Tree:
     *   root_panel
     *     └─ group_box
     *          └─ text_view
     *               └─ scroll_view
     *                    └─ label  ← clicked here
     *
     * Hit Test Path (index 0 → 4):
     *   [0] root_panel      ← Root
     *   [1] group_box
     *   [2] text_view
     *   [3] scroll_view
     *   [4] label           ← Target
     *
     * Event Routing:
     *   CAPTURE:  0 → 1 → 2 → 3 → 4  (forward)
     *   TARGET:   4                   (at target)
     *   BUBBLE:   4 → 3 → 2 → 1 → 0  (backward)
     * ```
     *
     * ## Usage
     *
     * @code
     * hit_test_path<Backend> path;
     *
     * // During hit test, elements record themselves
     * ui_element<Backend>* result = root->hit_test(x, y, path);
     *
     * // Path now contains: [root, ..., result]
     * CHECK(path.target() == result);
     * CHECK(path.size() == depth_of_tree);
     *
     * // Use for routing
     * for (size_t i = 0; i < path.size(); ++i) {
     *     path[i]->handle_event(evt, event_phase::capture);
     * }
     * @endcode
     */
    template<UIBackend Backend>
    class hit_test_path {
    public:
        using element_type = ui_element<Backend>;

        /**
         * @brief Construct empty path
         */
        hit_test_path() = default;

        /**
         * @brief Add element to path
         * @param elem Element to add (must not be nullptr)
         *
         * @details
         * Called by ui_element::hit_test() as it traverses the tree.
         * Elements are added in order from root to target.
         */
        void push(element_type* elem) {
            m_elements.push_back(elem);
        }

        /**
         * @brief Get target element (last in path)
         * @return Pointer to target element, or nullptr if path is empty
         *
         * @details
         * The target is the deepest element that contains the hit point.
         * This is the element that will receive events in the TARGET phase.
         */
        [[nodiscard]] element_type* target() const noexcept {
            return m_elements.empty() ? nullptr : m_elements.back();
        }

        /**
         * @brief Get root element (first in path)
         * @return Pointer to root element, or nullptr if path is empty
         *
         * @details
         * The root is the topmost element in the path.
         * This is where the CAPTURE phase begins.
         */
        [[nodiscard]] element_type* root() const noexcept {
            return m_elements.empty() ? nullptr : m_elements.front();
        }

        /**
         * @brief Check if path is empty
         * @return true if no elements in path
         */
        [[nodiscard]] bool empty() const noexcept {
            return m_elements.empty();
        }

        /**
         * @brief Get number of elements in path
         * @return Element count (0 if empty)
         *
         * @details
         * The size represents the depth of the tree from root to target.
         * Size 1 means hit test stopped at root (no children).
         */
        [[nodiscard]] size_t size() const noexcept {
            return m_elements.size();
        }

        /**
         * @brief Access element at index
         * @param index Index in path (0 = root, size()-1 = target)
         * @return Pointer to element at index
         *
         * @warning No bounds checking - caller must ensure index < size()
         *
         * @details
         * Allows iteration through path for event routing:
         * - Forward iteration (0 → size-1): CAPTURE phase
         * - Backward iteration (size-1 → 0): BUBBLE phase
         */
        [[nodiscard]] element_type* operator[](size_t index) const noexcept {
            return m_elements[index];
        }

        /**
         * @brief Access element at index with bounds checking
         * @param index Index in path (0 = root, size()-1 = target)
         * @return Pointer to element at index
         * @throws std::out_of_range if index >= size()
         */
        [[nodiscard]] element_type* at(size_t index) const {
            return m_elements.at(index);
        }

        /**
         * @brief Clear all elements from path
         *
         * @details
         * Resets path to empty state. Typically called before hit testing
         * to ensure path doesn't contain stale data from previous hit tests.
         */
        void clear() noexcept {
            m_elements.clear();
        }

        /**
         * @brief Check if path contains element
         * @param elem Element to search for
         * @return true if element is in path
         *
         * @details
         * Useful for checking if an element is an ancestor of the target.
         */
        [[nodiscard]] bool contains(const element_type* elem) const noexcept {
            for (const auto* e : m_elements) {
                if (e == elem) {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Get depth of element in path
         * @param elem Element to find
         * @return Index of element, or size_t(-1) if not found
         *
         * @details
         * Returns the distance from root:
         * - Root element: depth 0
         * - Root's child: depth 1
         * - Target: depth size()-1
         */
        [[nodiscard]] size_t depth_of(const element_type* elem) const noexcept {
            for (size_t i = 0; i < m_elements.size(); ++i) {
                if (m_elements[i] == elem) {
                    return i;
                }
            }
            return static_cast<size_t>(-1);
        }

    private:
        std::vector<element_type*> m_elements;
    };

} // namespace onyxui
