//
// Nested Struct Serialization Tests
// Tests multi-level struct nesting with YAML serialization
// using backend-agnostic test types
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/yaml/fkyaml_adapter.hh>
#include "../test_support/test_types.hh"
#include "../test_support/test_types_yaml.hh"

using namespace onyxui::yaml;

// ===========================================================================
// Test Structures - 2 Levels of Nesting
// ===========================================================================

TEST_CASE("Nested structs - 2 levels deep") {
    struct theme_colors {
        test::test_color foreground;
        test::test_color background;
    };

    struct button_theme {
        theme_colors normal;
        theme_colors hover;
        test::test_box_style border;
    };

    SUBCASE("Serialize nested struct") {
        button_theme theme{
            .normal = {.foreground = test::test_color{255, 255, 255}, .background = test::test_color{0, 0, 170}},
            .hover = {.foreground = test::test_color{255, 255, 0}, .background = test::test_color{0, 170, 170}},
            .border = test::test_box_style::single_line
        };

        auto yaml = to_yaml(theme);
        CHECK(yaml.is_mapping());

        // Check normal colors
        CHECK(yaml["normal"].is_mapping());
        CHECK(yaml["normal"]["foreground"].is_mapping());
        CHECK(yaml["normal"]["background"].is_mapping());

        // Check hover colors
        CHECK(yaml["hover"].is_mapping());
        CHECK(yaml["hover"]["foreground"].is_mapping());
        CHECK(yaml["hover"]["background"].is_mapping());

        // Check border
        CHECK(yaml["border"].get_value<std::string>() == "single_line");
    }

    SUBCASE("Deserialize nested struct") {
        std::string yaml_str = R"(
normal:
  foreground: [255, 255, 255]
  background: [0, 0, 170]
hover:
  foreground: [255, 255, 0]
  background: [0, 170, 170]
border: single_line
)";
        auto theme = from_yaml<button_theme>(fkyaml::node::deserialize(yaml_str));

        CHECK(theme.normal.foreground.r == 255);
        CHECK(theme.normal.foreground.g == 255);
        CHECK(theme.normal.foreground.b == 255);
        CHECK(theme.normal.background.r == 0);
        CHECK(theme.normal.background.g == 0);
        CHECK(theme.normal.background.b == 170);

        CHECK(theme.hover.foreground.r == 255);
        CHECK(theme.hover.foreground.g == 255);
        CHECK(theme.hover.foreground.b == 0);
        CHECK(theme.hover.background.r == 0);
        CHECK(theme.hover.background.g == 170);
        CHECK(theme.hover.background.b == 170);

        CHECK(theme.border == test::test_box_style::single_line);
    }

    SUBCASE("Round-trip nested struct") {
        button_theme original{
            .normal = {.foreground = test::test_color{200, 200, 200}, .background = test::test_color{50, 50, 50}},
            .hover = {.foreground = test::test_color{255, 255, 255}, .background = test::test_color{100, 100, 255}},
            .border = test::test_box_style::double_line
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<button_theme>(yaml);

        CHECK(restored.normal.foreground.r == original.normal.foreground.r);
        CHECK(restored.normal.foreground.g == original.normal.foreground.g);
        CHECK(restored.normal.foreground.b == original.normal.foreground.b);
        CHECK(restored.normal.background.r == original.normal.background.r);
        CHECK(restored.normal.background.g == original.normal.background.g);
        CHECK(restored.normal.background.b == original.normal.background.b);

        CHECK(restored.hover.foreground.r == original.hover.foreground.r);
        CHECK(restored.hover.foreground.g == original.hover.foreground.g);
        CHECK(restored.hover.foreground.b == original.hover.foreground.b);
        CHECK(restored.hover.background.r == original.hover.background.r);
        CHECK(restored.hover.background.g == original.hover.background.g);
        CHECK(restored.hover.background.b == original.hover.background.b);

        CHECK(restored.border == original.border);
    }
}

// ===========================================================================
// Test Structures - 3 Levels of Nesting
// ===========================================================================

TEST_CASE("Nested structs - 3 levels deep") {
    struct font_style {
        bool bold;
        bool underline;
    };

    struct text_style {
        test::test_color text_color;
        font_style font;
    };

    struct widget_theme {
        text_style title;
        text_style content;
        test::test_box_style border;
    };

    SUBCASE("Serialize 3-level nested struct") {
        widget_theme theme{
            .title = {
                .text_color = test::test_color{255, 255, 0},
                .font = {.bold = true, .underline = false}
            },
            .content = {
                .text_color = test::test_color{255, 255, 255},
                .font = {.bold = false, .underline = false}
            },
            .border = test::test_box_style::rounded
        };

        auto yaml = to_yaml(theme);
        CHECK(yaml.is_mapping());
        CHECK(yaml["title"]["text_color"].is_mapping());
        CHECK(yaml["title"]["font"]["bold"].get_value<bool>() == true);
        CHECK(yaml["title"]["font"]["underline"].get_value<bool>() == false);
        CHECK(yaml["content"]["text_color"].is_mapping());
        CHECK(yaml["content"]["font"]["bold"].get_value<bool>() == false);
        CHECK(yaml["border"].get_value<std::string>() == "rounded");
    }

    SUBCASE("Deserialize 3-level nested struct") {
        std::string yaml_str = R"(
title:
  text_color: {r: 255, g: 255, b: 0}
  font:
    bold: true
    underline: false
content:
  text_color: {r: 255, g: 255, b: 255}
  font:
    bold: false
    underline: false
border: rounded
)";
        auto theme = from_yaml<widget_theme>(fkyaml::node::deserialize(yaml_str));

        CHECK(theme.title.text_color.r == 255);
        CHECK(theme.title.text_color.g == 255);
        CHECK(theme.title.text_color.b == 0);
        CHECK(theme.title.font.bold == true);
        CHECK(theme.title.font.underline == false);

        CHECK(theme.content.text_color.r == 255);
        CHECK(theme.content.text_color.g == 255);
        CHECK(theme.content.text_color.b == 255);
        CHECK(theme.content.font.bold == false);
        CHECK(theme.content.font.underline == false);

        CHECK(theme.border == test::test_box_style::rounded);
    }

    SUBCASE("Round-trip 3-level nested struct") {
        widget_theme original{
            .title = {
                .text_color = test::test_color{128, 128, 255},
                .font = {.bold = true, .underline = true}
            },
            .content = {
                .text_color = test::test_color{200, 200, 200},
                .font = {.bold = false, .underline = true}
            },
            .border = test::test_box_style::heavy
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<widget_theme>(yaml);

        CHECK(restored.title.text_color.r == original.title.text_color.r);
        CHECK(restored.title.font.bold == original.title.font.bold);
        CHECK(restored.title.font.underline == original.title.font.underline);
        CHECK(restored.content.text_color.g == original.content.text_color.g);
        CHECK(restored.content.font.bold == original.content.font.bold);
        CHECK(restored.border == original.border);
    }
}

// ===========================================================================
// Edge Cases - Empty and Default Values
// ===========================================================================

TEST_CASE("Nested structs - Default values and missing fields") {
    struct inner {
        int value = 42;
        std::string name = "default";
    };

    struct outer {
        inner data;
        bool flag = true;
    };

    SUBCASE("Missing nested struct uses defaults") {
        std::string yaml_str = R"(
flag: false
)";
        auto obj = from_yaml<outer>(fkyaml::node::deserialize(yaml_str));
        CHECK(obj.data.value == 42);  // Default value
        CHECK(obj.data.name == "default");  // Default value
        CHECK(obj.flag == false);  // Specified value
    }

    SUBCASE("Partially specified nested struct") {
        std::string yaml_str = R"(
data:
  value: 100
flag: false
)";
        auto obj = from_yaml<outer>(fkyaml::node::deserialize(yaml_str));
        CHECK(obj.data.value == 100);  // Specified
        CHECK(obj.data.name == "default");  // Default (missing field)
        CHECK(obj.flag == false);  // Specified
    }

    SUBCASE("Fully specified nested struct") {
        std::string yaml_str = R"(
data:
  value: 200
  name: "custom"
flag: true
)";
        auto obj = from_yaml<outer>(fkyaml::node::deserialize(yaml_str));
        CHECK(obj.data.value == 200);
        CHECK(obj.data.name == "custom");
        CHECK(obj.flag == true);
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Nested structs - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
