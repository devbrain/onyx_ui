#pragma once

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <fkYAML/node.hpp>
#include <onyxui/utils/color_utils.hh>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <regex>

namespace onyxui::theme_palette {

/**
 * @brief Palette color storage
 *
 * Maps color names to hex color strings.
 * Example: {"bg": "0x0000AA", "fg": "0xFFFFFF", "accent": "0xFFFF00"}
 */
using palette_map = std::unordered_map<std::string, std::string>;

/**
 * @brief Extract palette from YAML string
 * @param yaml_str YAML content
 * @return Map of color names to hex strings
 *
 * Extracts palette section using regex matching.
 * Handles multi-line palette definitions.
 */
[[nodiscard]] inline palette_map extract_palette_from_string(const std::string& yaml_str) {
    palette_map palette;

    // Find palette section using regex
    // Pattern: palette:\n  key: "value"
    std::regex palette_section_regex(R"delim(palette:\s*\n((?:\s+\w+:\s*"[^"]+"\s*\n)+))delim");
    std::smatch section_match;

    if (!std::regex_search(yaml_str, section_match, palette_section_regex)) {
        return palette; // No palette section
    }

    std::string const palette_content = section_match[1].str();

    // Extract each key-value pair
    // Pattern:   key: "value"
    std::regex entry_regex(R"delim(\s+(\w+):\s*"([^"]+)")delim");
    std::sregex_iterator iter(palette_content.begin(), palette_content.end(), entry_regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string const key = (*iter)[1].str();
        std::string const value = (*iter)[2].str();

        // Validate that value is a hex color
        if (!color_utils::is_hex_color(value)) {
            throw std::runtime_error(
                "Invalid hex color in palette: '" + key + "' = '" + value +
                "' (expected format: 0xRRGGBB or 0xRRGGBBAA)"
            );
        }

        palette[key] = value;
    }

    return palette;
}

/**
 * @brief Resolve $references in YAML string
 * @param yaml_str YAML content with $references
 * @param palette Palette map
 * @return YAML with $references replaced
 *
 * Replaces all "$color_name" references with their hex values from palette.
 * Uses regex for replacement.
 */
[[nodiscard]] inline std::string resolve_references_in_string(
    const std::string& yaml_str,
    const palette_map& palette
) {
    std::string result = yaml_str;

    // Pattern: $color_name (word characters after $)
    std::regex reference_regex(R"(\$(\w+))");

    // Find all matches first (to avoid iterator invalidation)
    std::vector<std::pair<std::string, std::string>> replacements;
    std::sregex_iterator iter(yaml_str.begin(), yaml_str.end(), reference_regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string const reference = (*iter)[0].str(); // Full match: $color_name
        std::string const color_name = (*iter)[1].str(); // Captured group: color_name

        // Look up in palette
        auto it = palette.find(color_name);
        if (it == palette.end()) {
            throw std::runtime_error(
                "Undefined color reference: '" + reference +
                "' (color '" + color_name + "' not found in palette)"
            );
        }

        replacements.emplace_back(reference, it->second);
    }

    // Apply all replacements
    for (const auto& [reference, value] : replacements) {
        // Replace all occurrences
        std::size_t pos = 0;
        while ((pos = result.find(reference, pos)) != std::string::npos) {
            result.replace(pos, reference.length(), value);
            pos += value.length();
        }
    }

    return result;
}

/**
 * @brief Remove palette section from YAML string
 * @param yaml_str YAML content
 * @return YAML without palette section
 *
 * Removes the entire palette: section including all its entries.
 */
[[nodiscard]] inline std::string remove_palette_section(const std::string& yaml_str) {
    // Pattern: palette:\n  (indented entries)
    // Remove entire section
    std::regex palette_regex(R"delim(palette:\s*\n(?:\s+\w+:\s*"[^"]+"\s*\n)+)delim");
    return std::regex_replace(yaml_str, palette_regex, "");
}

/**
 * @brief Apply palette preprocessing to theme YAML
 * @param yaml_str Theme YAML string
 * @return Preprocessed YAML with all $references resolved
 *
 * Complete palette preprocessing pipeline:
 * 1. Extract palette section (regex)
 * 2. Resolve all $references (regex replace)
 * 3. Remove palette section
 *
 * Example:
 * ```cpp
 * std::string theme_yaml = R"(
 * palette:
 *   bg: "0x0000AA"
 *   fg: "0xFFFFFF"
 *
 * button:
 *   normal:
 *     foreground: "$fg"
 *     background: "$bg"
 * )";
 *
 * std::string resolved = apply_palette_preprocessing(theme_yaml);
 * // Result: button.normal.foreground = "0xFFFFFF", etc.
 * ```
 *
 * @throws std::runtime_error if palette is invalid or references undefined
 */
[[nodiscard]] inline std::string apply_palette_preprocessing(const std::string& yaml_str) {
    // Extract palette
    palette_map const palette = extract_palette_from_string(yaml_str);

    // If no palette, return original YAML
    if (palette.empty()) {
        return yaml_str;
    }

    // Resolve all references
    std::string resolved = resolve_references_in_string(yaml_str, palette);

    // Remove palette section
    resolved = remove_palette_section(resolved);

    return resolved;
}

} // namespace onyxui::theme_palette

#endif // ONYXUI_ENABLE_YAML_THEMES
