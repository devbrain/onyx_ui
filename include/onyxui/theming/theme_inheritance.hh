#pragma once

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <fkYAML/node.hpp>
#include <string>
#include <unordered_set>
#include <stdexcept>

namespace onyxui::theme_inheritance {

/**
 * @brief Deep merge two YAML nodes (child overrides parent)
 * @param parent Parent YAML node
 * @param child Child YAML node (overrides)
 * @return Merged YAML node
 *
 * Merging rules:
 * - Scalars: child overrides parent
 * - Sequences: child replaces parent entirely
 * - Mappings: recursive deep merge (child fields override, parent fields preserved)
 *
 * Example:
 * Parent: { a: 1, b: { c: 2, d: 3 } }
 * Child:  { b: { c: 99 }, e: 4 }
 * Result: { a: 1, b: { c: 99, d: 3 }, e: 4 }
 */
[[nodiscard]] inline fkyaml::node merge_yaml_nodes(
    const fkyaml::node& parent,
    const fkyaml::node& child
) {
    // If child is not a mapping, it completely replaces parent
    if (!child.is_mapping()) {
        return child;
    }

    // If parent is not a mapping, child replaces it
    if (!parent.is_mapping()) {
        return child;
    }

    // Both are mappings - deep merge
    fkyaml::node result = parent;  // Start with parent

    // Iterate over child keys and merge
    for (auto it = child.begin(); it != child.end(); ++it) {
        std::string const key = it.key().template get_value<std::string>();

        // If parent doesn't have this key, just add child's value
        if (!parent.contains(key)) {
            result[key] = *it;
            continue;
        }

        // Parent has this key - recursively merge if both are mappings
        fkyaml::node const& parent_value = parent[key];
        fkyaml::node const& child_value = *it;

        if (parent_value.is_mapping() && child_value.is_mapping()) {
            // Recursive merge for nested mappings
            result[key] = merge_yaml_nodes(parent_value, child_value);
        } else {
            // Child overrides parent (scalar, sequence, or type mismatch)
            result[key] = child_value;
        }
    }

    return result;
}

/**
 * @brief Check if theme YAML has extends field
 * @param theme_yaml Theme YAML node
 * @return True if extends field exists
 */
[[nodiscard]] inline bool has_extends_field(const fkyaml::node& theme_yaml) {
    return theme_yaml.is_mapping() && theme_yaml.contains("extends");
}

/**
 * @brief Get parent theme name from extends field
 * @param theme_yaml Theme YAML node
 * @return Parent theme name
 * @throws std::runtime_error if extends field is missing or invalid
 */
[[nodiscard]] inline std::string get_extends_value(const fkyaml::node& theme_yaml) {
    if (!has_extends_field(theme_yaml)) {
        throw std::runtime_error("Theme does not have 'extends' field");
    }

    fkyaml::node const& extends_node = theme_yaml["extends"];
    if (!extends_node.is_string()) {
        throw std::runtime_error("Theme 'extends' field must be a string");
    }

    return extends_node.template get_value<std::string>();
}

/**
 * @brief Remove extends field from theme YAML
 * @param theme_yaml Theme YAML node
 * @return YAML without extends field
 *
 * The extends field is metadata for loading, not part of the theme data.
 * It must be removed before deserialization to avoid struct field errors.
 */
[[nodiscard]] inline fkyaml::node remove_extends_field(const fkyaml::node& theme_yaml) {
    if (!theme_yaml.is_mapping()) {
        return theme_yaml;
    }

    fkyaml::node result;
    result = fkyaml::node::mapping();

    for (auto it = theme_yaml.begin(); it != theme_yaml.end(); ++it) {
        std::string const key = it.key().template get_value<std::string>();

        // Skip extends field
        if (key == "extends") {
            continue;
        }

        result[key] = *it;
    }

    return result;
}

/**
 * @brief Detect circular inheritance
 * @param theme_name Current theme name
 * @param visited Set of already visited theme names
 * @throws std::runtime_error if circular inheritance detected
 */
inline void check_circular_inheritance(
    const std::string& theme_name,
    std::unordered_set<std::string>& visited
) {
    if (visited.count(theme_name) > 0) {
        throw std::runtime_error(
            "Circular theme inheritance detected: '" + theme_name +
            "' appears multiple times in inheritance chain"
        );
    }
    visited.insert(theme_name);
}

} // namespace onyxui::theme_inheritance

#endif // ONYXUI_ENABLE_YAML_THEMES
