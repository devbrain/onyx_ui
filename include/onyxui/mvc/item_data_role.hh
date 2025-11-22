//
// OnyxUI MVC System - Item Data Roles
// Created: 2025-11-22
//

#pragma once

#include <cstdint>

namespace onyxui {

/**
 * @brief Defines different aspects/roles of data for a single model item
 *
 * @details
 * A single item in a model can provide different types of data depending
 * on the role requested. For example, a Person item might return:
 * - display: "John Doe (30)"
 * - edit: Person{name="John Doe", age=30}
 * - tooltip: "Email: john@example.com"
 * - foreground: blue_color (for special items)
 *
 * This allows models to provide rich information without requiring
 * multiple parallel data structures.
 *
 * @par Standard Roles:
 * - **display**: Text shown to the user (std::string)
 * - **edit**: Data for editing (any type via std::variant or std::any)
 * - **decoration**: Icon or image (future: icon type)
 * - **tooltip**: Tooltip text (std::string)
 * - **background**: Background color (Backend::color_type)
 * - **foreground**: Text color (Backend::color_type)
 *
 * @par Custom Roles:
 * Applications can define custom roles starting from `user_role`:
 * @code
 * enum class MyRoles : uint8_t {
 *     sort_key = static_cast<uint8_t>(item_data_role::user_role) + 0,
 *     timestamp = static_cast<uint8_t>(item_data_role::user_role) + 1,
 *     priority = static_cast<uint8_t>(item_data_role::user_role) + 2
 * };
 * @endcode
 *
 * @par Example Usage:
 * @code
 * // Get display text for an item
 * auto display_data = model->data(index, item_data_role::display);
 * std::string text = std::get<std::string>(display_data);
 *
 * // Get custom background color
 * auto bg_data = model->data(index, item_data_role::background);
 * if (std::holds_alternative<color_type>(bg_data)) {
 *     color_type bg = std::get<color_type>(bg_data);
 * }
 * @endcode
 */
enum class item_data_role : std::uint8_t {
    /**
     * @brief Display text for the item
     *
     * @details
     * Returns the text that should be displayed to the user.
     * Type: std::string
     *
     * This is the primary data role used by default delegates.
     * Models should always provide meaningful display text.
     *
     * @par Example:
     * - For Person{name="Alice", age=30}: "Alice (30)"
     * - For int(42): "42"
     * - For Product{name="Widget", price=9.99}: "Widget - $9.99"
     */
    display = 0,

    /**
     * @brief Editable data for the item
     *
     * @details
     * Returns the actual data for editing, which may be a different
     * type than the display string.
     * Type: Any (via std::variant or std::any)
     *
     * Used by editor widgets when in-place editing is enabled.
     *
     * @par Example:
     * - For Person: Person{name="Alice", age=30} (struct)
     * - For int(42): 42 (int)
     * - For date: Date{2025, 11, 22} (date struct)
     */
    edit = 1,

    /**
     * @brief Icon or decoration for the item
     *
     * @details
     * Returns an icon, image, or other decoration to display with the item.
     * Type: Icon type (future implementation)
     *
     * Delegates can render icons before or after text.
     *
     * @par Example:
     * - File items: folder/file icons
     * - Status items: warning/error/success icons
     * - Priority items: star/flag icons
     */
    decoration = 2,

    /**
     * @brief Tooltip text for the item
     *
     * @details
     * Returns detailed text to show in a tooltip when hovering.
     * Type: std::string
     *
     * Views can show tooltips on hover if this role returns data.
     *
     * @par Example:
     * - For files: "Size: 1.2 MB\nModified: 2025-11-22"
     * - For emails: "From: alice@example.com\nSubject: Hello"
     */
    tooltip = 3,

    /**
     * @brief Background color for the item
     *
     * @details
     * Returns a custom background color for the item.
     * Type: Backend::color_type
     *
     * Delegates use this to override the default background.
     * Selected items typically override this with selection color.
     *
     * @par Example:
     * - Error items: red background
     * - Warning items: yellow background
     * - Success items: green background
     */
    background = 4,

    /**
     * @brief Foreground (text) color for the item
     *
     * @details
     * Returns a custom text color for the item.
     * Type: Backend::color_type
     *
     * Delegates use this to override the default text color.
     * Selected items typically override this with selection text color.
     *
     * @par Example:
     * - Important items: bold red text
     * - Disabled items: gray text
     * - Links: blue text
     */
    foreground = 5,

    /**
     * @brief Starting point for user-defined roles
     *
     * @details
     * Applications can define custom roles starting from this value.
     * User roles should be cast to/from uint8_t:
     *
     * @code
     * enum class MyRoles : uint8_t {
     *     sort_key = static_cast<uint8_t>(item_data_role::user_role),
     *     timestamp = static_cast<uint8_t>(item_data_role::user_role) + 1
     * };
     *
     * // Query custom role
     * auto data = model->data(index, static_cast<item_data_role>(MyRoles::sort_key));
     * @endcode
     */
    user_role = 32
};

} // namespace onyxui
