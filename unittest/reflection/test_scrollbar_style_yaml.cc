/**
 * @file test_scrollbar_style_yaml.cc
 * @brief Unit tests for scrollbar_style enum YAML serialization
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <../../include/onyxui/theming/theme.hh>
#include <fkYAML/node.hpp>
#include <string>

using namespace onyxui;

// =============================================================================
// scrollbar_style Enum Tests
// =============================================================================

TEST_CASE("scrollbar_style - Enum to string conversion") {
    SUBCASE("minimal") {
        CHECK(scrollbar_style_to_string(scrollbar_style::minimal) == "minimal");
    }

    SUBCASE("classic") {
        CHECK(scrollbar_style_to_string(scrollbar_style::classic) == "classic");
    }

    SUBCASE("compact") {
        CHECK(scrollbar_style_to_string(scrollbar_style::compact) == "compact");
    }
}

TEST_CASE("scrollbar_style - String to enum conversion") {
    SUBCASE("minimal") {
        CHECK(scrollbar_style_from_string("minimal") == scrollbar_style::minimal);
    }

    SUBCASE("classic") {
        CHECK(scrollbar_style_from_string("classic") == scrollbar_style::classic);
    }

    SUBCASE("compact") {
        CHECK(scrollbar_style_from_string("compact") == scrollbar_style::compact);
    }

    SUBCASE("invalid string throws") {
        CHECK_THROWS_AS(scrollbar_style_from_string("invalid"), std::runtime_error);
        CHECK_THROWS_AS(scrollbar_style_from_string(""), std::runtime_error);
        CHECK_THROWS_AS(scrollbar_style_from_string("MINIMAL"), std::runtime_error);  // Case-sensitive
    }
}

TEST_CASE("scrollbar_style - YAML serialization") {
    SUBCASE("Serialize all values") {
        // Test each enum value serializes to a string
        auto node_minimal = fkyaml::node(std::string(scrollbar_style_to_string(scrollbar_style::minimal)));
        CHECK(node_minimal.is_string());
        CHECK(node_minimal.get_value<std::string>() == "minimal");

        auto node_classic = fkyaml::node(std::string(scrollbar_style_to_string(scrollbar_style::classic)));
        CHECK(node_classic.is_string());
        CHECK(node_classic.get_value<std::string>() == "classic");

        auto node_compact = fkyaml::node(std::string(scrollbar_style_to_string(scrollbar_style::compact)));
        CHECK(node_compact.is_string());
        CHECK(node_compact.get_value<std::string>() == "compact");
    }
}

TEST_CASE("scrollbar_style - YAML deserialization") {
    SUBCASE("Deserialize all values") {
        auto node_minimal = fkyaml::node("minimal");
        CHECK(scrollbar_style_from_string(node_minimal.get_value<std::string>()) == scrollbar_style::minimal);

        auto node_classic = fkyaml::node("classic");
        CHECK(scrollbar_style_from_string(node_classic.get_value<std::string>()) == scrollbar_style::classic);

        auto node_compact = fkyaml::node("compact");
        CHECK(scrollbar_style_from_string(node_compact.get_value<std::string>()) == scrollbar_style::compact);
    }

    SUBCASE("Invalid values throw") {
        auto node_invalid = fkyaml::node("invalid_style");
        CHECK_THROWS_AS(scrollbar_style_from_string(node_invalid.get_value<std::string>()), std::runtime_error);
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("scrollbar_style Serialization - YAML themes disabled" * doctest::skip(true)) {
    // Skipped: YAML theme support not enabled in this build
}

#endif // ONYXUI_ENABLE_YAML_THEMES
