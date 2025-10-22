//
// Font Reflection Tests
// Tests conio_renderer::font serialization/deserialization
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/yaml/fkyaml_adapter.hh>
#include <onyxui/conio/conio_renderer.hh>

using namespace onyxui::yaml;
using namespace onyxui::conio;

TEST_CASE("Font - Serialization all boolean combinations") {
    SUBCASE("All false (default)") {
        conio_renderer::font f{false, false, false};
        auto yaml = to_yaml(f);
        CHECK(yaml.is_mapping());
        CHECK(yaml["bold"].get_value<bool>() == false);
        CHECK(yaml["underline"].get_value<bool>() == false);
        CHECK(yaml["reverse"].get_value<bool>() == false);
    }

    SUBCASE("Bold only") {
        conio_renderer::font f{true, false, false};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == true);
        CHECK(yaml["underline"].get_value<bool>() == false);
        CHECK(yaml["reverse"].get_value<bool>() == false);
    }

    SUBCASE("Underline only") {
        conio_renderer::font f{false, true, false};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == false);
        CHECK(yaml["underline"].get_value<bool>() == true);
        CHECK(yaml["reverse"].get_value<bool>() == false);
    }

    SUBCASE("Reverse only") {
        conio_renderer::font f{false, false, true};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == false);
        CHECK(yaml["underline"].get_value<bool>() == false);
        CHECK(yaml["reverse"].get_value<bool>() == true);
    }

    SUBCASE("Bold and underline") {
        conio_renderer::font f{true, true, false};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == true);
        CHECK(yaml["underline"].get_value<bool>() == true);
        CHECK(yaml["reverse"].get_value<bool>() == false);
    }

    SUBCASE("Bold and reverse") {
        conio_renderer::font f{true, false, true};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == true);
        CHECK(yaml["underline"].get_value<bool>() == false);
        CHECK(yaml["reverse"].get_value<bool>() == true);
    }

    SUBCASE("Underline and reverse") {
        conio_renderer::font f{false, true, true};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == false);
        CHECK(yaml["underline"].get_value<bool>() == true);
        CHECK(yaml["reverse"].get_value<bool>() == true);
    }

    SUBCASE("All true") {
        conio_renderer::font f{true, true, true};
        auto yaml = to_yaml(f);
        CHECK(yaml["bold"].get_value<bool>() == true);
        CHECK(yaml["underline"].get_value<bool>() == true);
        CHECK(yaml["reverse"].get_value<bool>() == true);
    }
}

TEST_CASE("Font - Deserialization") {
    SUBCASE("All fields specified - all false") {
        std::string yaml = R"(
bold: false
underline: false
reverse: false
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == false);
        CHECK(f.underline == false);
        CHECK(f.reverse == false);
    }

    SUBCASE("All fields specified - all true") {
        std::string yaml = R"(
bold: true
underline: true
reverse: true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == true);
        CHECK(f.reverse == true);
    }

    SUBCASE("All fields specified - mixed") {
        std::string yaml = R"(
bold: true
underline: false
reverse: true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == false);
        CHECK(f.reverse == true);
    }

    SUBCASE("Field order doesn't matter") {
        std::string yaml = R"(
reverse: true
bold: false
underline: true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == false);
        CHECK(f.underline == true);
        CHECK(f.reverse == true);
    }
}

TEST_CASE("Font - Default values when fields missing") {
    SUBCASE("Empty YAML uses all defaults") {
        std::string yaml = "{}";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == false);
        CHECK(f.underline == false);
        CHECK(f.reverse == false);
    }

    SUBCASE("Missing bold field uses default") {
        std::string yaml = R"(
underline: true
reverse: true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == false);  // Default
        CHECK(f.underline == true);
        CHECK(f.reverse == true);
    }

    SUBCASE("Missing underline field uses default") {
        std::string yaml = R"(
bold: true
reverse: true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == false);  // Default
        CHECK(f.reverse == true);
    }

    SUBCASE("Missing reverse field uses default") {
        std::string yaml = R"(
bold: true
underline: true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == true);
        CHECK(f.reverse == false);  // Default
    }

    SUBCASE("Only one field specified") {
        std::string yaml = "bold: true";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == false);  // Default
        CHECK(f.reverse == false);  // Default
    }
}

TEST_CASE("Font - Round-trip preservation") {
    SUBCASE("Round-trip all false") {
        conio_renderer::font original{false, false, false};
        auto yaml = to_yaml(original);
        auto restored = from_yaml<conio_renderer::font>(yaml);
        CHECK(restored.bold == original.bold);
        CHECK(restored.underline == original.underline);
        CHECK(restored.reverse == original.reverse);
    }

    SUBCASE("Round-trip all true") {
        conio_renderer::font original{true, true, true};
        auto yaml = to_yaml(original);
        auto restored = from_yaml<conio_renderer::font>(yaml);
        CHECK(restored.bold == original.bold);
        CHECK(restored.underline == original.underline);
        CHECK(restored.reverse == original.reverse);
    }

    SUBCASE("Round-trip mixed values") {
        conio_renderer::font original{true, false, true};
        auto yaml = to_yaml(original);
        auto restored = from_yaml<conio_renderer::font>(yaml);
        CHECK(restored.bold == original.bold);
        CHECK(restored.underline == original.underline);
        CHECK(restored.reverse == original.reverse);
    }
}

TEST_CASE("Font - Integration with other types") {
    SUBCASE("Font in struct with color") {
        struct text_style {
            conio_renderer::font font;
            color text_color;
        };

        text_style style{
            .font = {true, true, false},
            .text_color = {255, 255, 0}
        };

        auto yaml = to_yaml(style);
        CHECK(yaml.is_mapping());
        CHECK(yaml["font"]["bold"].get_value<bool>() == true);
        CHECK(yaml["font"]["underline"].get_value<bool>() == true);
        // Color now as object: CHECK(yaml["text_color"].is_mapping());
    }

    SUBCASE("Deserialize font from struct") {
        struct text_style {
            conio_renderer::font font;
            color text_color;
        };

        std::string yaml = R"(
font:
  bold: true
  underline: false
  reverse: true
text_color: [100, 150, 200]
)";

        auto style = from_yaml_string<text_style>(yaml);
        CHECK(style.font.bold == true);
        CHECK(style.font.underline == false);
        CHECK(style.font.reverse == true);
        CHECK(style.text_color.r == 100);
        CHECK(style.text_color.g == 150);
        CHECK(style.text_color.b == 200);
    }

    SUBCASE("Font with enum and color") {
        struct widget_style {
            conio_renderer::font font;
            conio_renderer::box_style border;
            color background;
        };

        widget_style style{
            .font = {true, false, false},
            .border = conio_renderer::box_style::double_line,
            .background = {50, 100, 150}
        };

        auto yaml = to_yaml(style);
        CHECK(yaml["font"]["bold"].get_value<bool>() == true);
        CHECK(yaml["border"].get_value<std::string>() == "double_line");
        // Color now as object: CHECK(yaml["background"].is_mapping());
}
}

TEST_CASE("Font - YAML format variations") {
    SUBCASE("Inline format") {
        std::string yaml = "{ bold: true, underline: false, reverse: true }";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == false);
        CHECK(f.reverse == true);
    }

    SUBCASE("Compact format") {
        std::string yaml = "bold: true\nunderline: false\nreverse: true";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == false);
        CHECK(f.reverse == true);
    }

    SUBCASE("Extra whitespace handled") {
        std::string yaml = R"(
bold:    true
underline:   false
reverse:     true
)";
        auto f = from_yaml_string<conio_renderer::font>(yaml);
        CHECK(f.bold == true);
        CHECK(f.underline == false);
        CHECK(f.reverse == true);
    }
}

TEST_CASE("Font - to_yaml_string produces valid YAML") {
    SUBCASE("Serialize and verify YAML is valid") {
        conio_renderer::font f{true, false, true};
        std::string yaml_str = to_yaml_string(f);

        // Should contain the field names
        CHECK(yaml_str.find("bold") != std::string::npos);
        CHECK(yaml_str.find("underline") != std::string::npos);
        CHECK(yaml_str.find("reverse") != std::string::npos);

        // Should be deserializable
        auto restored = from_yaml_string<conio_renderer::font>(yaml_str);
        CHECK(restored.bold == true);
        CHECK(restored.underline == false);
        CHECK(restored.reverse == true);
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Font - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
