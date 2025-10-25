//
// Enum Reflection for conio Backend and core onyxui enums
// Enables YAML serialization/deserialization of box_style, icon_style, and horizontal_alignment enums
//

#pragma once

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/conio/conio_renderer.hh>
#include <onyxui/layout_strategy.hh>  // For horizontal_alignment
#include <fkYAML/node.hpp>
#include <rfl.hpp>

namespace onyxui::conio {

    // ===========================================================================
    // border_style Enum Reflection
    // ===========================================================================

    // Use reflect-cpp's RFL_ENUM_TO_STRING macro for automatic enum <-> string conversion
    inline std::string_view border_style_to_string(conio_renderer::border_style style) {
        switch (style) {
            case conio_renderer::border_style::none:        return "none";
            case conio_renderer::border_style::single_line: return "single_line";
            case conio_renderer::border_style::double_line: return "double_line";
            case conio_renderer::border_style::rounded:     return "rounded";
            case conio_renderer::border_style::heavy:       return "heavy";
        }
        throw std::runtime_error("Unknown border_style value");
    }

    inline conio_renderer::border_style border_style_from_string(std::string_view str) {
        if (str == "none")        return conio_renderer::border_style::none;
        if (str == "single_line") return conio_renderer::border_style::single_line;
        if (str == "double_line") return conio_renderer::border_style::double_line;
        if (str == "rounded")     return conio_renderer::border_style::rounded;
        if (str == "heavy")       return conio_renderer::border_style::heavy;
        throw std::runtime_error("Invalid border_style string: " + std::string(str));
    }

    // Helper functions for box_style serialization/deserialization
    inline std::string box_style_to_string(const conio_renderer::box_style& style) {
        std::string result = std::string(border_style_to_string(style.style));
        if (!style.is_solid) {
            result += ":hollow";  // Mark non-solid boxes
        }
        return result;
    }

    inline conio_renderer::box_style box_style_from_string(std::string_view str) {
        // Parse "style" or "style:hollow"
        bool is_solid = true;
        std::string_view border_str = str;

        auto colon_pos = str.find(':');
        if (colon_pos != std::string_view::npos) {
            border_str = str.substr(0, colon_pos);
            auto modifier = str.substr(colon_pos + 1);
            if (modifier == "hollow") {
                is_solid = false;
            }
        }

        auto border = border_style_from_string(border_str);
        return conio_renderer::box_style{border, is_solid};
    }

    // ===========================================================================
    // icon_style Enum Reflection
    // ===========================================================================

    inline std::string_view icon_style_to_string(conio_renderer::icon_style style) {
        switch (style) {
            case conio_renderer::icon_style::none:        return "none";
            case conio_renderer::icon_style::check:       return "check";
            case conio_renderer::icon_style::cross:       return "cross";
            case conio_renderer::icon_style::arrow_up:    return "arrow_up";
            case conio_renderer::icon_style::arrow_down:  return "arrow_down";
            case conio_renderer::icon_style::arrow_left:  return "arrow_left";
            case conio_renderer::icon_style::arrow_right: return "arrow_right";
            case conio_renderer::icon_style::bullet:      return "bullet";
            case conio_renderer::icon_style::folder:      return "folder";
            case conio_renderer::icon_style::file:        return "file";
        }
        throw std::runtime_error("Unknown icon_style value");
    }

    inline conio_renderer::icon_style icon_style_from_string(std::string_view str) {
        if (str == "none")        return conio_renderer::icon_style::none;
        if (str == "check")       return conio_renderer::icon_style::check;
        if (str == "cross")       return conio_renderer::icon_style::cross;
        if (str == "arrow_up")    return conio_renderer::icon_style::arrow_up;
        if (str == "arrow_down")  return conio_renderer::icon_style::arrow_down;
        if (str == "arrow_left")  return conio_renderer::icon_style::arrow_left;
        if (str == "arrow_right") return conio_renderer::icon_style::arrow_right;
        if (str == "bullet")      return conio_renderer::icon_style::bullet;
        if (str == "folder")      return conio_renderer::icon_style::folder;
        if (str == "file")        return conio_renderer::icon_style::file;
        throw std::runtime_error("Invalid icon_style string: " + std::string(str));
    }

} // namespace onyxui::conio

// ===========================================================================
// horizontal_alignment Enum Reflection (onyxui namespace)
// ===========================================================================

namespace onyxui {

    inline std::string_view horizontal_alignment_to_string(horizontal_alignment align) {
        switch (align) {
            case horizontal_alignment::left:    return "left";
            case horizontal_alignment::center:  return "center";
            case horizontal_alignment::right:   return "right";
            case horizontal_alignment::stretch: return "stretch";
        }
        throw std::runtime_error("Unknown horizontal_alignment value");
    }

    inline horizontal_alignment horizontal_alignment_from_string(std::string_view str) {
        if (str == "left")    return horizontal_alignment::left;
        if (str == "center")  return horizontal_alignment::center;
        if (str == "right")   return horizontal_alignment::right;
        if (str == "stretch") return horizontal_alignment::stretch;
        throw std::runtime_error("Invalid horizontal_alignment string: " + std::string(str));
    }

} // namespace onyxui

// ===========================================================================
// fkYAML Adapter Integration
// ===========================================================================

// Add overloads in the yaml::detail namespace that will be found via overload resolution
// These are non-template overloads that take precedence over the template versions

namespace onyxui::yaml::detail {

    // Bring *_to_string functions into this namespace for ADL and concept checks
    using onyxui::conio::box_style_to_string;
    using onyxui::conio::box_style_from_string;
    using onyxui::conio::icon_style_to_string;
    using onyxui::conio::icon_style_from_string;
    using onyxui::conio::border_style_to_string;
    using onyxui::conio::border_style_from_string;
    using onyxui::horizontal_alignment_to_string;
    using onyxui::horizontal_alignment_from_string;

    // Non-template overloads for enum serialization (preferred over template version)

    inline fkyaml::node to_yaml_value(const onyxui::conio::conio_renderer::box_style& value) {
        return fkyaml::node(box_style_to_string(value));
    }

    inline fkyaml::node to_yaml_value(const onyxui::conio::conio_renderer::icon_style& value) {
        return fkyaml::node(std::string(icon_style_to_string(value)));
    }

    inline fkyaml::node to_yaml_value(const onyxui::horizontal_alignment& value) {
        return fkyaml::node(std::string(horizontal_alignment_to_string(value)));
    }

    // Helper functions for custom deserialization via tag dispatch
    // Used by the template's requires clause to detect types with custom deserialization

    inline auto enum_from_yaml_impl(const fkyaml::node& node, onyxui::conio::conio_renderer::box_style*)
        -> onyxui::conio::conio_renderer::box_style {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for box_style must be a string");
        }
        return box_style_from_string(node.get_value<std::string>());
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, onyxui::conio::conio_renderer::icon_style*)
        -> onyxui::conio::conio_renderer::icon_style {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for icon_style must be a string");
        }
        return icon_style_from_string(node.get_value<std::string>());
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, onyxui::horizontal_alignment*)
        -> onyxui::horizontal_alignment {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for horizontal_alignment must be a string");
        }
        return horizontal_alignment_from_string(node.get_value<std::string>());
    }

} // namespace onyxui::yaml::detail

// Explicit template specializations for from_yaml (not from_yaml_value)
// These take precedence over the general template

namespace onyxui::yaml {

    template<>
    inline conio::conio_renderer::box_style from_yaml<conio::conio_renderer::box_style>(const fkyaml::node& node) {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for box_style must be a string");
        }
        return conio::box_style_from_string(node.get_value<std::string>());
    }

    template<>
    inline conio::conio_renderer::icon_style from_yaml<conio::conio_renderer::icon_style>(const fkyaml::node& node) {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for icon_style must be a string");
        }
        return conio::icon_style_from_string(node.get_value<std::string>());
    }

    template<>
    inline horizontal_alignment from_yaml<horizontal_alignment>(const fkyaml::node& node) {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for horizontal_alignment must be a string");
        }
        return horizontal_alignment_from_string(node.get_value<std::string>());
    }

} // namespace onyxui::yaml

#endif // ONYXUI_ENABLE_YAML_THEMES
