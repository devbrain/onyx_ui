#pragma once

/**
 * @file test_types_yaml.hh
 * @brief YAML serialization support for backend-agnostic test types
 *
 * Provides fkYAML integration for test::* types defined in test_types.hh.
 * This allows YAML infrastructure tests to be completely independent of
 * backend implementations.
 */

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include "test_types.hh"
#include <fkYAML/node.hpp>

// ===========================================================================
// fkYAML Adapter Integration for test types
// ===========================================================================

// Add overloads in the yaml::detail namespace that will be found via ADL
// Following the same pattern as conio enum_reflection.hh

namespace onyxui::yaml::detail {

    // ===========================================================================
    // test::test_box_style serialization
    // ===========================================================================

    inline fkyaml::node to_yaml_value(const test::test_box_style& value) {
        return fkyaml::node(std::string(test::to_string(value)));
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, test::test_box_style*)
        -> test::test_box_style {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for test_box_style must be a string");
        }
        return test::box_style_from_string(node.get_value<std::string>());
    }

    // ===========================================================================
    // test::test_alignment serialization
    // ===========================================================================

    inline fkyaml::node to_yaml_value(const test::test_alignment& value) {
        return fkyaml::node(std::string(test::to_string(value)));
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, test::test_alignment*)
        -> test::test_alignment {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for test_alignment must be a string");
        }
        return test::alignment_from_string(node.get_value<std::string>());
    }

    // ===========================================================================
    // test::test_icon_style serialization
    // ===========================================================================

    inline fkyaml::node to_yaml_value(const test::test_icon_style& value) {
        return fkyaml::node(std::string(test::to_string(value)));
    }

    inline auto enum_from_yaml_impl(const fkyaml::node& node, test::test_icon_style*)
        -> test::test_icon_style {
        if (!node.is_string()) {
            throw std::runtime_error("YAML node for test_icon_style must be a string");
        }
        return test::icon_style_from_string(node.get_value<std::string>());
    }

} // namespace onyxui::yaml::detail

#endif // ONYXUI_ENABLE_YAML_THEMES
