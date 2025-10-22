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
    // box_style Enum Reflection
    // ===========================================================================

    // Use reflect-cpp's RFL_ENUM_TO_STRING macro for automatic enum <-> string conversion
    inline std::string_view box_style_to_string(conio_renderer::box_style style) {
        switch (style) {
            case conio_renderer::box_style::none:        return "none";
            case conio_renderer::box_style::single_line: return "single_line";
            case conio_renderer::box_style::double_line: return "double_line";
            case conio_renderer::box_style::rounded:     return "rounded";
            case conio_renderer::box_style::heavy:       return "heavy";
        }
        throw std::runtime_error("Unknown box_style value");
    }

    inline conio_renderer::box_style box_style_from_string(std::string_view str) {
        if (str == "none")        return conio_renderer::box_style::none;
        if (str == "single_line") return conio_renderer::box_style::single_line;
        if (str == "double_line") return conio_renderer::box_style::double_line;
        if (str == "rounded")     return conio_renderer::box_style::rounded;
        if (str == "heavy")       return conio_renderer::box_style::heavy;
        throw std::runtime_error("Invalid box_style string: " + std::string(str));
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

    // Non-template overloads for enum serialization (preferred over template version)

    inline fkyaml::node to_yaml_value(const onyxui::conio::conio_renderer::box_style& value) {
        return fkyaml::node(std::string(onyxui::conio::box_style_to_string(value)));
    }

    inline fkyaml::node to_yaml_value(const onyxui::conio::conio_renderer::icon_style& value) {
        return fkyaml::node(std::string(onyxui::conio::icon_style_to_string(value)));
    }

    inline fkyaml::node to_yaml_value(const onyxui::horizontal_alignment& value) {
        return fkyaml::node(std::string(onyxui::horizontal_alignment_to_string(value)));
    }

    // Helper functions that will be called by the template version
    // We can't overload from_yaml_value directly because it's a template with deduced return type
    // Instead, we inject these into the detail namespace and the template's if constexpr will find them

    inline auto enum_from_yaml_impl(const fkyaml::node& node, onyxui::conio::conio_renderer::box_style*)
        -> onyxui::conio::conio_renderer::box_style {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for box_style must be a string");
        }
        return onyxui::conio::box_style_from_string(node.get_value<std::string>());
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, onyxui::conio::conio_renderer::icon_style*)
        -> onyxui::conio::conio_renderer::icon_style {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for icon_style must be a string");
        }
        return onyxui::conio::icon_style_from_string(node.get_value<std::string>());
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, onyxui::horizontal_alignment*)
        -> onyxui::horizontal_alignment {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for horizontal_alignment must be a string");
        }
        return onyxui::horizontal_alignment_from_string(node.get_value<std::string>());
    }

} // namespace onyxui::yaml::detail

#endif // ONYXUI_ENABLE_YAML_THEMES
