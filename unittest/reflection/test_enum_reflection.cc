//
// Unit Tests for Enum Reflection
// Tests box_style and icon_style enum serialization/deserialization
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/conio/enum_reflection.hh>
#include <onyxui/yaml/fkyaml_adapter.hh>

using namespace onyxui::conio;
using namespace onyxui::yaml;

// ===========================================================================
// box_style Enum Tests
// ===========================================================================

TEST_CASE("box_style - Enum to string conversion") {
    SUBCASE("none") {
        CHECK(box_style_to_string(conio_renderer::box_style::none) == "none");
    }

    SUBCASE("single_line") {
        CHECK(box_style_to_string(conio_renderer::box_style::single_line) == "single_line");
    }

    SUBCASE("double_line") {
        CHECK(box_style_to_string(conio_renderer::box_style::double_line) == "double_line");
    }

    SUBCASE("rounded") {
        CHECK(box_style_to_string(conio_renderer::box_style::rounded) == "rounded");
    }

    SUBCASE("heavy") {
        CHECK(box_style_to_string(conio_renderer::box_style::heavy) == "heavy");
    }
}

TEST_CASE("box_style - String to enum conversion") {
    SUBCASE("none") {
        CHECK(box_style_from_string("none") == conio_renderer::box_style::none);
    }

    SUBCASE("single_line") {
        CHECK(box_style_from_string("single_line") == conio_renderer::box_style::single_line);
    }

    SUBCASE("double_line") {
        CHECK(box_style_from_string("double_line") == conio_renderer::box_style::double_line);
    }

    SUBCASE("rounded") {
        CHECK(box_style_from_string("rounded") == conio_renderer::box_style::rounded);
    }

    SUBCASE("heavy") {
        CHECK(box_style_from_string("heavy") == conio_renderer::box_style::heavy);
    }

    SUBCASE("invalid string throws") {
        CHECK_THROWS_AS(box_style_from_string("invalid"), std::runtime_error);
        CHECK_THROWS_AS(box_style_from_string(""), std::runtime_error);
        CHECK_THROWS_AS(box_style_from_string("NONE"), std::runtime_error);  // Case-sensitive
    }
}

TEST_CASE("box_style - YAML serialization") {
    SUBCASE("Serialize all values") {
        // Test each enum value serializes to a string
        auto node_none = fkyaml::node(std::string(box_style_to_string(conio_renderer::box_style::none)));
        CHECK(node_none.is_string());
        CHECK(node_none.get_value<std::string>() == "none");

        auto node_single = fkyaml::node(std::string(box_style_to_string(conio_renderer::box_style::single_line)));
        CHECK(node_single.is_string());
        CHECK(node_single.get_value<std::string>() == "single_line");

        auto node_double = fkyaml::node(std::string(box_style_to_string(conio_renderer::box_style::double_line)));
        CHECK(node_double.is_string());
        CHECK(node_double.get_value<std::string>() == "double_line");

        auto node_rounded = fkyaml::node(std::string(box_style_to_string(conio_renderer::box_style::rounded)));
        CHECK(node_rounded.is_string());
        CHECK(node_rounded.get_value<std::string>() == "rounded");

        auto node_heavy = fkyaml::node(std::string(box_style_to_string(conio_renderer::box_style::heavy)));
        CHECK(node_heavy.is_string());
        CHECK(node_heavy.get_value<std::string>() == "heavy");
    }
}

TEST_CASE("box_style - YAML deserialization") {
    SUBCASE("Deserialize all values") {
        auto node_none = fkyaml::node("none");
        CHECK(box_style_from_string(node_none.get_value<std::string>()) == conio_renderer::box_style::none);

        auto node_single = fkyaml::node("single_line");
        CHECK(box_style_from_string(node_single.get_value<std::string>()) == conio_renderer::box_style::single_line);

        auto node_double = fkyaml::node("double_line");
        CHECK(box_style_from_string(node_double.get_value<std::string>()) == conio_renderer::box_style::double_line);

        auto node_rounded = fkyaml::node("rounded");
        CHECK(box_style_from_string(node_rounded.get_value<std::string>()) == conio_renderer::box_style::rounded);

        auto node_heavy = fkyaml::node("heavy");
        CHECK(box_style_from_string(node_heavy.get_value<std::string>()) == conio_renderer::box_style::heavy);
    }

    SUBCASE("Invalid values throw") {
        CHECK_THROWS_AS(box_style_from_string("invalid_style"), std::runtime_error);
    }
}

TEST_CASE("box_style - Round-trip preservation") {
    SUBCASE("All values survive round-trip") {
        auto test_round_trip = [](conio_renderer::box_style original) {
            auto str = box_style_to_string(original);
            auto restored = box_style_from_string(str);
            CHECK(restored == original);
        };

        test_round_trip(conio_renderer::box_style::none);
        test_round_trip(conio_renderer::box_style::single_line);
        test_round_trip(conio_renderer::box_style::double_line);
        test_round_trip(conio_renderer::box_style::rounded);
        test_round_trip(conio_renderer::box_style::heavy);
    }
}

// ===========================================================================
// icon_style Enum Tests
// ===========================================================================

TEST_CASE("icon_style - Enum to string conversion") {
    CHECK(icon_style_to_string(conio_renderer::icon_style::none) == "none");
    CHECK(icon_style_to_string(conio_renderer::icon_style::check) == "check");
    CHECK(icon_style_to_string(conio_renderer::icon_style::cross) == "cross");
    CHECK(icon_style_to_string(conio_renderer::icon_style::arrow_up) == "arrow_up");
    CHECK(icon_style_to_string(conio_renderer::icon_style::arrow_down) == "arrow_down");
    CHECK(icon_style_to_string(conio_renderer::icon_style::arrow_left) == "arrow_left");
    CHECK(icon_style_to_string(conio_renderer::icon_style::arrow_right) == "arrow_right");
    CHECK(icon_style_to_string(conio_renderer::icon_style::bullet) == "bullet");
    CHECK(icon_style_to_string(conio_renderer::icon_style::folder) == "folder");
    CHECK(icon_style_to_string(conio_renderer::icon_style::file) == "file");
}

TEST_CASE("icon_style - String to enum conversion") {
    CHECK(icon_style_from_string("none") == conio_renderer::icon_style::none);
    CHECK(icon_style_from_string("check") == conio_renderer::icon_style::check);
    CHECK(icon_style_from_string("cross") == conio_renderer::icon_style::cross);
    CHECK(icon_style_from_string("arrow_up") == conio_renderer::icon_style::arrow_up);
    CHECK(icon_style_from_string("arrow_down") == conio_renderer::icon_style::arrow_down);
    CHECK(icon_style_from_string("arrow_left") == conio_renderer::icon_style::arrow_left);
    CHECK(icon_style_from_string("arrow_right") == conio_renderer::icon_style::arrow_right);
    CHECK(icon_style_from_string("bullet") == conio_renderer::icon_style::bullet);
    CHECK(icon_style_from_string("folder") == conio_renderer::icon_style::folder);
    CHECK(icon_style_from_string("file") == conio_renderer::icon_style::file);

    SUBCASE("invalid string throws") {
        CHECK_THROWS_AS(icon_style_from_string("invalid"), std::runtime_error);
        CHECK_THROWS_AS(icon_style_from_string(""), std::runtime_error);
        CHECK_THROWS_AS(icon_style_from_string("CHECK"), std::runtime_error);  // Case-sensitive
    }
}

TEST_CASE("icon_style - YAML serialization") {
    SUBCASE("Serialize sample values") {
        auto node_check = fkyaml::node(std::string(icon_style_to_string(conio_renderer::icon_style::check)));
        CHECK(node_check.is_string());
        CHECK(node_check.get_value<std::string>() == "check");

        auto node_arrow = fkyaml::node(std::string(icon_style_to_string(conio_renderer::icon_style::arrow_up)));
        CHECK(node_arrow.is_string());
        CHECK(node_arrow.get_value<std::string>() == "arrow_up");

        auto node_bullet = fkyaml::node(std::string(icon_style_to_string(conio_renderer::icon_style::bullet)));
        CHECK(node_bullet.is_string());
        CHECK(node_bullet.get_value<std::string>() == "bullet");
    }
}

TEST_CASE("icon_style - YAML deserialization") {
    SUBCASE("Deserialize sample values") {
        auto node_check = fkyaml::node("check");
        CHECK(icon_style_from_string(node_check.get_value<std::string>()) == conio_renderer::icon_style::check);

        auto node_arrow = fkyaml::node("arrow_down");
        CHECK(icon_style_from_string(node_arrow.get_value<std::string>()) == conio_renderer::icon_style::arrow_down);

        auto node_folder = fkyaml::node("folder");
        CHECK(icon_style_from_string(node_folder.get_value<std::string>()) == conio_renderer::icon_style::folder);
    }

    SUBCASE("Invalid values throw") {
        CHECK_THROWS_AS(icon_style_from_string("invalid_icon"), std::runtime_error);
    }
}

TEST_CASE("icon_style - Round-trip preservation") {
    auto test_round_trip = [](conio_renderer::icon_style original) {
        auto str = icon_style_to_string(original);
        auto restored = icon_style_from_string(str);
        CHECK(restored == original);
    };

    test_round_trip(conio_renderer::icon_style::none);
    test_round_trip(conio_renderer::icon_style::check);
    test_round_trip(conio_renderer::icon_style::cross);
    test_round_trip(conio_renderer::icon_style::arrow_up);
    test_round_trip(conio_renderer::icon_style::arrow_down);
    test_round_trip(conio_renderer::icon_style::arrow_left);
    test_round_trip(conio_renderer::icon_style::arrow_right);
    test_round_trip(conio_renderer::icon_style::bullet);
    test_round_trip(conio_renderer::icon_style::folder);
    test_round_trip(conio_renderer::icon_style::file);
}

// ===========================================================================
// Integration Tests - Enums in Structs (CRITICAL)
// ===========================================================================

TEST_CASE("Enums in structs - Basic struct with enum fields") {
    struct widget_style {
        conio_renderer::box_style border;
        conio_renderer::icon_style icon;
    };

    SUBCASE("Serialize struct with enums") {
        widget_style style{
            .border = conio_renderer::box_style::double_line,
            .icon = conio_renderer::icon_style::check
        };

        auto yaml = to_yaml(style);
        CHECK(yaml.is_mapping());
        CHECK(yaml["border"].is_string());
        CHECK(yaml["border"].get_value<std::string>() == "double_line");
        CHECK(yaml["icon"].is_string());
        CHECK(yaml["icon"].get_value<std::string>() == "check");
    }

    SUBCASE("Deserialize struct with enums from YAML") {
        std::string yaml = R"(
border: rounded
icon: folder
)";
        auto style = from_yaml<widget_style>(fkyaml::node::deserialize(yaml));
        CHECK(style.border == conio_renderer::box_style::rounded);
        CHECK(style.icon == conio_renderer::icon_style::folder);
    }

    SUBCASE("Round-trip struct with enums") {
        widget_style original{
            .border = conio_renderer::box_style::heavy,
            .icon = conio_renderer::icon_style::bullet
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<widget_style>(yaml);

        CHECK(restored.border == original.border);
        CHECK(restored.icon == original.icon);
    }

    SUBCASE("All box_style values serialize correctly in struct") {
        auto test_value = [](conio_renderer::box_style value, const char* expected) {
            widget_style style{.border = value, .icon = conio_renderer::icon_style::none};
            auto yaml = to_yaml(style);
            CHECK(yaml["border"].get_value<std::string>() == expected);
        };

        test_value(conio_renderer::box_style::none, "none");
        test_value(conio_renderer::box_style::single_line, "single_line");
        test_value(conio_renderer::box_style::double_line, "double_line");
        test_value(conio_renderer::box_style::rounded, "rounded");
        test_value(conio_renderer::box_style::heavy, "heavy");
    }

    SUBCASE("All icon_style values serialize correctly in struct") {
        auto test_value = [](conio_renderer::icon_style value, const char* expected) {
            widget_style style{.border = conio_renderer::box_style::none, .icon = value};
            auto yaml = to_yaml(style);
            CHECK(yaml["icon"].get_value<std::string>() == expected);
        };

        test_value(conio_renderer::icon_style::none, "none");
        test_value(conio_renderer::icon_style::check, "check");
        test_value(conio_renderer::icon_style::cross, "cross");
        test_value(conio_renderer::icon_style::arrow_up, "arrow_up");
        test_value(conio_renderer::icon_style::arrow_down, "arrow_down");
        test_value(conio_renderer::icon_style::arrow_left, "arrow_left");
        test_value(conio_renderer::icon_style::arrow_right, "arrow_right");
        test_value(conio_renderer::icon_style::bullet, "bullet");
        test_value(conio_renderer::icon_style::folder, "folder");
        test_value(conio_renderer::icon_style::file, "file");
    }
}

TEST_CASE("Enums in structs - Mixed with other types") {
    struct button_style {
        conio_renderer::box_style border;
        color background;
        color foreground;
        conio_renderer::icon_style icon;
        bool enabled;
    };

    SUBCASE("Serialize complex struct") {
        button_style style{
            .border = conio_renderer::box_style::single_line,
            .background = color{0, 0, 170},
            .foreground = color{255, 255, 255},
            .icon = conio_renderer::icon_style::check,
            .enabled = true
        };

        auto yaml = to_yaml(style);
        CHECK(yaml.is_mapping());
        CHECK(yaml["border"].get_value<std::string>() == "single_line");
        // Colors now serialize as objects {r, g, b} instead of hex strings
        CHECK(yaml["background"].is_mapping());
        CHECK(yaml["background"]["b"].get_value<int>() == 170);
        CHECK(yaml["foreground"].is_mapping());
        CHECK(yaml["foreground"]["r"].get_value<int>() == 255);
        CHECK(yaml["icon"].get_value<std::string>() == "check");
        CHECK(yaml["enabled"].get_value<bool>() == true);
    }

    SUBCASE("Deserialize complex struct") {
        std::string yaml = R"(
border: double_line
background: [100, 150, 200]
foreground: {r: 255, g: 255, b: 0}
icon: folder
enabled: false
)";
        auto style = from_yaml<button_style>(fkyaml::node::deserialize(yaml));
        CHECK(style.border == conio_renderer::box_style::double_line);
        CHECK(style.background.r == 100);
        CHECK(style.background.g == 150);
        CHECK(style.background.b == 200);
        CHECK(style.foreground.r == 255);
        CHECK(style.foreground.g == 255);
        CHECK(style.foreground.b == 0);
        CHECK(style.icon == conio_renderer::icon_style::folder);
        CHECK(style.enabled == false);
    }

    SUBCASE("Round-trip complex struct") {
        button_style original{
            .border = conio_renderer::box_style::rounded,
            .background = color{50, 100, 150},
            .foreground = color{200, 220, 240},
            .icon = conio_renderer::icon_style::arrow_right,
            .enabled = true
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<button_style>(yaml);

        CHECK(restored.border == original.border);
        CHECK(restored.background.r == original.background.r);
        CHECK(restored.background.g == original.background.g);
        CHECK(restored.background.b == original.background.b);
        CHECK(restored.foreground.r == original.foreground.r);
        CHECK(restored.foreground.g == original.foreground.g);
        CHECK(restored.foreground.b == original.foreground.b);
        CHECK(restored.icon == original.icon);
        CHECK(restored.enabled == original.enabled);
    }
}

TEST_CASE("Enums in structs - Error handling and defaults") {
    struct widget_style {
        conio_renderer::box_style border;
        conio_renderer::icon_style icon;
    };

    SUBCASE("Invalid enum string uses default value") {
        // reflect-cpp is forgiving - invalid fields use default values
        std::string yaml = R"(
border: invalid_border
icon: check
)";
        auto style = from_yaml<widget_style>(fkyaml::node::deserialize(yaml));
        CHECK(style.border == conio_renderer::box_style::none);  // Default (invalid field ignored)
        CHECK(style.icon == conio_renderer::icon_style::check);  // Valid value used
    }

    SUBCASE("Non-string enum value uses default") {
        // reflect-cpp handles type mismatches gracefully
        std::string yaml = R"(
border: 123
icon: check
)";
        auto style = from_yaml<widget_style>(fkyaml::node::deserialize(yaml));
        CHECK(style.border == conio_renderer::box_style::none);  // Default (type mismatch)
        CHECK(style.icon == conio_renderer::icon_style::check);  // Valid value used
    }

    SUBCASE("Missing field uses default value") {
        std::string yaml = R"(
icon: check
)";
        // Missing 'border' field - uses default constructed value
        auto style = from_yaml<widget_style>(fkyaml::node::deserialize(yaml));
        CHECK(style.border == conio_renderer::box_style::none);  // Default constructed
        CHECK(style.icon == conio_renderer::icon_style::check);  // Specified value
    }

    SUBCASE("All fields valid") {
        std::string yaml = R"(
border: double_line
icon: folder
)";
        auto style = from_yaml<widget_style>(fkyaml::node::deserialize(yaml));
        CHECK(style.border == conio_renderer::box_style::double_line);
        CHECK(style.icon == conio_renderer::icon_style::folder);
    }
}

#endif // ONYXUI_ENABLE_YAML_THEMES
