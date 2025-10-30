/**
 * @file element_fwd.hh
 * @brief Forward declarations for ui_element and related types
 * @author Assistant
 * @date 2025-10-27
 *
 * @details
 * Provides forward declarations to break circular dependencies.
 * This allows files to reference ui_element without including the full
 * element.hh header, which is especially useful for:
 *
 * - layer_manager.hh (uses ui_element* in std::shared_ptr/weak_ptr)
 * - ui_services.hh (breaks circular dependency with themeable.hh)
 *
 * ## Circular Dependency Resolution
 *
 * **Problem**: themeable → ui_services → ui_context → layer_manager → element → themeable
 *
 * **Solution**: layer_manager includes element_fwd.hh (not element.hh)
 *
 * This breaks the cycle:
 * - themeable → ui_services → ui_context → layer_manager → element_fwd (STOPS)
 * - themeable can now safely include ui_services.hh
 */

#pragma once

#include <onyxui/concepts/backend.hh>

namespace onyxui {

    /**
     * @class ui_element
     * @brief Forward declaration of the core UI element class
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * This forward declaration allows using ui_element in:
     * - Pointers (ui_element*)
     * - References (ui_element&)
     * - Smart pointers (std::unique_ptr<ui_element>, std::shared_ptr<ui_element>)
     * - Template parameters
     *
     * For the full definition, include <onyxui/element.hh>
     */
    template<UIBackend Backend>
    class ui_element;

} // namespace onyxui
