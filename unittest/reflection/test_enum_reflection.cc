//
// Unit Tests for Enum Reflection
// Tests box_style and icon_style enum serialization/deserialization
// using backend-agnostic test::test_box_style and test::test_icon_style types
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/yaml/fkyaml_adapter.hh>
#include "test_types.hh"
#include "test_types_yaml.hh"

using namespace onyxui::yaml;

// ===========================================================================
// test_box_style Enum Tests
// ===========================================================================

TEST_CASE("box_style - Enum to string conversion") {
    SUBCASE("none") {
        CHECK(test::to_string(test::test_box_style::none) == "none");
    }

    SUBCASE("single_line") {
        CHECK(test::to_string(test::test_box_style::single_line) == "single_line");
    }

    SUBCASE("double_line") {
        CHECK(test::to_string(test::test_box_style::double_line) == "double_line");
    }

    SUBCASE("rounded") {
        CHECK(test::to_string(test::test_box_style::rounded) == "rounded");
    }

    SUBCASE("heavy") {
        CHECK(test::to_string(test::test_box_style::heavy) == "heavy");
    }
}

TEST_CASE("box_style - String to enum conversion") {
    SUBCASE("none") {
        CHECK(test::box_style_from_string("none") == test::test_box_style::none);
    }

    SUBCASE("single_line") {
        CHECK(test::box_style_from_string("single_line") == test::test_box_style::single_line);
    }

    SUBCASE("double_line") {
        CHECK(test::box_style_from_string("double_line") == test::test_box_style::double_line);
    }

    SUBCASE("rounded") {
        CHECK(test::box_style_from_string("rounded") == test::test_box_style::rounded);
    }

    SUBCASE("heavy") {
        CHECK(test::box_style_from_string("heavy") == test::test_box_style::heavy);
    }

    SUBCASE("invalid string throws") {
        CHECK_THROWS_AS(test::box_style_from_string("invalid"), std::runtime_error);
        CHECK_THROWS_AS(test::box_style_from_string(""), std::runtime_error);
        CHECK_THROWS_AS(test::box_style_from_string("NONE"), std::runtime_error);  // Case-sensitive
    }
}

TEST_CASE("box_style - YAML serialization") {
    SUBCASE("Serialize all values") {
        // Test each enum value serializes to a string
        auto node_none = fkyaml::node(std::string(test::to_string(test::test_box_style::none)));
        CHECK(node_none.is_string());
        CHECK(node_none.get_value<std::string>() == "none");

        auto node_single = fkyaml::node(std::string(test::to_string(test::test_box_style::single_line)));
        CHECK(node_single.is_string());
        CHECK(node_single.get_value<std::string>() == "single_line");

        auto node_double = fkyaml::node(std::string(test::to_string(test::test_box_style::double_line)));
        CHECK(node_double.is_string());
        CHECK(node_double.get_value<std::string>() == "double_line");

        auto node_rounded = fkyaml::node(std::string(test::to_string(test::test_box_style::rounded)));
        CHECK(node_rounded.is_string());
        CHECK(node_rounded.get_value<std::string>() == "rounded");

        auto node_heavy = fkyaml::node(std::string(test::to_string(test::test_box_style::heavy)));
        CHECK(node_heavy.is_string());
        CHECK(node_heavy.get_value<std::string>() == "heavy");
    }
}

TEST_CASE("box_style - YAML deserialization") {
    SUBCASE("Deserialize all values") {
        auto node_none = fkyaml::node("none");
        CHECK(test::box_style_from_string(node_none.get_value<std::string>()) == test::test_box_style::none);

        auto node_single = fkyaml::node("single_line");
        CHECK(test::box_style_from_string(node_single.get_value<std::string>()) == test::test_box_style::single_line);

        auto node_double = fkyaml::node("double_line");
        CHECK(test::box_style_from_string(node_double.get_value<std::string>()) == test::test_box_style::double_line);

        auto node_rounded = fkyaml::node("rounded");
        CHECK(test::box_style_from_string(node_rounded.get_value<std::string>()) == test::test_box_style::rounded);

        auto node_heavy = fkyaml::node("heavy");
        CHECK(test::box_style_from_string(node_heavy.get_value<std::string>()) == test::test_box_style::heavy);
    }

    SUBCASE("Invalid values throw") {
        auto node_invalid = fkyaml::node("invalid_style");
        CHECK_THROWS_AS(test::box_style_from_string(node_invalid.get_value<std::string>()), std::runtime_error);
    }
}

TEST_CASE("box_style - Round-trip via YAML") {
    struct style_wrapper {
        test::test_box_style style;
    };

    SUBCASE("Round-trip all enum values") {
        std::vector<test::test_box_style> styles = {
            test::test_box_style::none,
            test::test_box_style::single_line,
            test::test_box_style::double_line,
            test::test_box_style::rounded,
            test::test_box_style::heavy
        };

        for (const auto& original_style : styles) {
            style_wrapper original{original_style};

            // Serialize
            auto yaml = to_yaml(original);
            CHECK(yaml["style"].is_string());

            // Deserialize
            std::string yaml_str = "style: " + std::string(test::to_string(original_style));
            auto restored = from_yaml_string<style_wrapper>(yaml_str);

            CHECK(restored.style == original_style);
        }
    }
}

// ===========================================================================
// test_icon_style Enum Tests
// ===========================================================================

TEST_CASE("icon_style - Enum to string conversion") {
    SUBCASE("All icon_style values") {
        CHECK(test::to_string(test::test_icon_style::none) == "none");
        CHECK(test::to_string(test::test_icon_style::check) == "check");
        CHECK(test::to_string(test::test_icon_style::cross) == "cross");
        CHECK(test::to_string(test::test_icon_style::arrow_up) == "arrow_up");
        CHECK(test::to_string(test::test_icon_style::arrow_down) == "arrow_down");
        CHECK(test::to_string(test::test_icon_style::arrow_left) == "arrow_left");
        CHECK(test::to_string(test::test_icon_style::arrow_right) == "arrow_right");
        CHECK(test::to_string(test::test_icon_style::bullet) == "bullet");
        CHECK(test::to_string(test::test_icon_style::folder) == "folder");
        CHECK(test::to_string(test::test_icon_style::file) == "file");
    }
}

TEST_CASE("icon_style - String to enum conversion") {
    SUBCASE("Valid strings") {
        CHECK(test::icon_style_from_string("none") == test::test_icon_style::none);
        CHECK(test::icon_style_from_string("check") == test::test_icon_style::check);
        CHECK(test::icon_style_from_string("cross") == test::test_icon_style::cross);
        CHECK(test::icon_style_from_string("arrow_up") == test::test_icon_style::arrow_up);
        CHECK(test::icon_style_from_string("arrow_down") == test::test_icon_style::arrow_down);
        CHECK(test::icon_style_from_string("arrow_left") == test::test_icon_style::arrow_left);
        CHECK(test::icon_style_from_string("arrow_right") == test::test_icon_style::arrow_right);
        CHECK(test::icon_style_from_string("bullet") == test::test_icon_style::bullet);
        CHECK(test::icon_style_from_string("folder") == test::test_icon_style::folder);
        CHECK(test::icon_style_from_string("file") == test::test_icon_style::file);
    }

    SUBCASE("Invalid strings throw") {
        CHECK_THROWS_AS(test::icon_style_from_string("invalid"), std::runtime_error);
        CHECK_THROWS_AS(test::icon_style_from_string(""), std::runtime_error);
        CHECK_THROWS_AS(test::icon_style_from_string("CHECK"), std::runtime_error);  // Case-sensitive
    }
}

TEST_CASE("icon_style - YAML round-trip") {
    struct icon_wrapper {
        test::test_icon_style icon;
    };

    SUBCASE("Round-trip all enum values") {
        std::vector<test::test_icon_style> icons = {
            test::test_icon_style::none,
            test::test_icon_style::check,
            test::test_icon_style::cross,
            test::test_icon_style::arrow_up,
            test::test_icon_style::arrow_down,
            test::test_icon_style::arrow_left,
            test::test_icon_style::arrow_right,
            test::test_icon_style::bullet,
            test::test_icon_style::folder,
            test::test_icon_style::file
        };

        for (const auto& original_icon : icons) {
            icon_wrapper original{original_icon};

            // Serialize
            auto yaml = to_yaml(original);
            CHECK(yaml["icon"].is_string());

            // Deserialize
            std::string yaml_str = "icon: " + std::string(test::to_string(original_icon));
            auto restored = from_yaml_string<icon_wrapper>(yaml_str);

            CHECK(restored.icon == original_icon);
        }
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Enum Reflection - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
