//
// Widget Style Reflection Tests
// Tests serialization/deserialization of button_style, label_style, and panel_style
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <../../../include/onyxui/utils/fkyaml_adapter.hh>
#include <onyxui/theme.hh>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/enum_reflection.hh>

using namespace onyxui::yaml;
using namespace onyxui;
using namespace onyxui::conio;

// Type aliases for convenience
using theme_type = ui_theme<conio_backend>;
using button_style = theme_type::button_style;
using label_style = theme_type::label_style;
using panel_style = theme_type::panel_style;

TEST_CASE("Button Style - Serialization") {
    SUBCASE("Complete button style") {
        button_style style{
            .normal = {
                .font = {true, false, false},
                .foreground = color{255, 255, 255},
                .background = color{0, 0, 170}
            },
            .hover = {
                .font = {true, false, false},
                .foreground = color{255, 255, 0},
                .background = color{0, 170, 170}
            },
            .pressed = {
                .font = {false, false, false},
                .foreground = color{0, 0, 0},
                .background = color{170, 170, 170}
            },
            .disabled = {
                .font = {false, false, false},
                .foreground = color{128, 128, 128},
                .background = color{64, 64, 64}
            },
            .mnemonic_font = {true, true, false},
            .box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true},
            .padding_horizontal = 4,
            .padding_vertical = 2,
            .text_align = horizontal_alignment::center,
            .corner_radius = 0
        };

        auto yaml = to_yaml(style);
        CHECK(yaml.is_mapping());

        // Check visual_state bundles
        CHECK(yaml["normal"].is_mapping());
        CHECK(yaml["hover"].is_mapping());
        CHECK(yaml["pressed"].is_mapping());
        CHECK(yaml["disabled"].is_mapping());

        // Check enums
        CHECK(yaml["box_style"].get_value<std::string>() == "double_line");
        CHECK(yaml["text_align"].get_value<std::string>() == "center");

        // Check fonts in visual states
        CHECK(yaml["normal"]["font"]["bold"].get_value<bool>() == true);
        CHECK(yaml["mnemonic_font"]["underline"].get_value<bool>() == true);

        // Check integers
        CHECK(yaml["padding_horizontal"].get_value<int>() == 4);
        CHECK(yaml["padding_vertical"].get_value<int>() == 2);
    }
}

TEST_CASE("Button Style - Deserialization") {
    SUBCASE("Full button style from YAML") {
        std::string yaml_str = R"(
normal:
  font:
    bold: true
    underline: false
    reverse: false
  foreground: [255, 255, 255]
  background: [0, 0, 170]
hover:
  font:
    bold: true
    underline: false
    reverse: false
  foreground: [255, 255, 0]
  background: [0, 170, 170]
pressed:
  font:
    bold: false
    underline: false
    reverse: false
  foreground: [0, 0, 0]
  background: [170, 170, 170]
disabled:
  font:
    bold: false
    underline: false
    reverse: false
  foreground: [128, 128, 128]
  background: [64, 64, 64]
box_style: double_line
mnemonic_font:
  bold: true
  underline: true
  reverse: false
padding_horizontal: 4
padding_vertical: 2
text_align: center
corner_radius: 0
)";

        auto style = from_yaml_string<button_style>(yaml_str);

        // Check visual states
        CHECK(style.normal.foreground.r == 255);
        CHECK(style.normal.background.b == 170);
        CHECK(style.hover.foreground.g == 255);

        // Check enums
        CHECK(style.box_style.style == conio_renderer::border_style::double_line);
        CHECK(style.text_align == horizontal_alignment::center);

        // Check fonts in visual states
        CHECK(style.normal.font.bold == true);
        CHECK(style.mnemonic_font.underline == true);

        // Check integers
        CHECK(style.padding_horizontal == 4);
        CHECK(style.padding_vertical == 2);
    }

    SUBCASE("Button style with defaults") {
        std::string yaml_str = R"(
normal:
  foreground: [255, 255, 255]
  background: [0, 0, 170]
hover:
  foreground: [255, 255, 0]
  background: [0, 170, 170]
pressed:
  foreground: [0, 0, 0]
  background: [170, 170, 170]
disabled:
  foreground: [128, 128, 128]
  background: [64, 64, 64]
)";

        auto style = from_yaml_string<button_style>(yaml_str);

        // Check that defaults are used for missing fields
        CHECK(style.padding_horizontal == 4);  // Default value
        CHECK(style.padding_vertical == 4);    // Default value
        CHECK(style.text_align == horizontal_alignment::center);  // Default
    }
}

TEST_CASE("Label Style - Serialization") {
    SUBCASE("Complete label style") {
        label_style style{
            .text = color{255, 255, 255},
            .background = color{0, 0, 170},
            .font = {false, false, false},
            .mnemonic_font = {false, true, false}
        };

        auto yaml = to_yaml(style);
        CHECK(yaml.is_mapping());

        // Color now as object: CHECK(yaml["text"].is_mapping());
        // Color now as object: CHECK(yaml["background"].is_mapping());
        CHECK(yaml["font"]["bold"].get_value<bool>() == false);
        CHECK(yaml["mnemonic_font"]["underline"].get_value<bool>() == true);
    }
}

TEST_CASE("Label Style - Deserialization") {
    SUBCASE("Full label style from YAML") {
        std::string yaml_str = R"(
text: [255, 255, 0]
background: [0, 0, 170]
font:
  bold: false
  underline: false
  reverse: false
mnemonic_font:
  bold: false
  underline: true
  reverse: false
)";

        auto style = from_yaml_string<label_style>(yaml_str);

        CHECK(style.text.r == 255);
        CHECK(style.text.g == 255);
        CHECK(style.text.b == 0);
        CHECK(style.background.b == 170);
        CHECK(style.font.bold == false);
        CHECK(style.mnemonic_font.underline == true);
    }

    SUBCASE("Label style with minimal fields") {
        std::string yaml_str = R"(
text: [255, 255, 255]
background: [0, 0, 0]
)";

        auto style = from_yaml_string<label_style>(yaml_str);

        CHECK(style.text.r == 255);
        CHECK(style.background.r == 0);
        // Fonts should use defaults
        CHECK(style.font.bold == false);
        CHECK(style.font.underline == false);
    }
}

TEST_CASE("Panel Style - Serialization") {
    SUBCASE("Complete panel style") {
        panel_style style{
            .background = color{0, 0, 170},
            .border_color = color{255, 255, 255},
            .box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true},
            .has_border = true
        };

        auto yaml = to_yaml(style);
        CHECK(yaml.is_mapping());

        // Color now as object: CHECK(yaml["background"].is_mapping());
        // Color now as object: CHECK(yaml["border_color"].is_mapping());
        CHECK(yaml["box_style"].get_value<std::string>() == "single_line");
        CHECK(yaml["has_border"].get_value<bool>() == true);
    }

    SUBCASE("Panel without border") {
        panel_style style{
            .background = color{50, 50, 50},
            .border_color = color{0, 0, 0},
            .box_style = conio_renderer::box_style{conio_renderer::border_style::none, true},
            .has_border = false
        };

        auto yaml = to_yaml(style);
        CHECK(yaml["has_border"].get_value<bool>() == false);
        CHECK(yaml["box_style"].get_value<std::string>() == "none");
    }
}

TEST_CASE("Panel Style - Deserialization") {
    SUBCASE("Full panel style from YAML") {
        std::string yaml_str = R"(
background: [0, 0, 170]
border_color: [255, 255, 255]
box_style: single_line
has_border: true
)";

        auto style = from_yaml_string<panel_style>(yaml_str);

        CHECK(style.background.b == 170);
        CHECK(style.border_color.r == 255);
        CHECK(style.box_style.style == conio_renderer::border_style::single_line);
        CHECK(style.has_border == true);
    }

    SUBCASE("Panel style with defaults") {
        std::string yaml_str = R"(
background: [64, 64, 64]
border_color: [128, 128, 128]
)";

        auto style = from_yaml_string<panel_style>(yaml_str);

        CHECK(style.background.r == 64);
        CHECK(style.has_border == true);  // Default value
    }
}

TEST_CASE("Widget Styles - Round-trip preservation") {
    SUBCASE("Button style round-trip") {
        button_style original{
            .normal = {
                .font = {true, false, false},
                .foreground = color{200, 200, 200},
                .background = color{50, 50, 50}
            },
            .hover = {
                .font = {true, false, false},
                .foreground = color{255, 255, 255},
                .background = color{100, 100, 255}
            },
            .pressed = {
                .font = {false, false, false},
                .foreground = color{128, 128, 128},
                .background = color{30, 30, 30}
            },
            .disabled = {
                .font = {false, false, false},
                .foreground = color{100, 100, 100},
                .background = color{40, 40, 40}
            },
            .mnemonic_font = {true, true, false},
            .box_style = conio_renderer::box_style{conio_renderer::border_style::rounded, true},
            .padding_horizontal = 6,
            .padding_vertical = 3,
            .text_align = horizontal_alignment::left
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<button_style>(yaml);

        CHECK(restored.normal.foreground.r == original.normal.foreground.r);
        CHECK(restored.hover.background.b == original.hover.background.b);
        CHECK(restored.box_style == original.box_style);
        CHECK(restored.normal.font.bold == original.normal.font.bold);
        CHECK(restored.padding_horizontal == original.padding_horizontal);
        CHECK(restored.text_align == original.text_align);
    }

    SUBCASE("Label style round-trip") {
        label_style original{
            .text = color{255, 255, 0},
            .background = color{0, 0, 128},
            .font = {false, true, false},
            .mnemonic_font = {true, true, false}
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<label_style>(yaml);

        CHECK(restored.text.g == original.text.g);
        CHECK(restored.background.b == original.background.b);
        CHECK(restored.font.underline == original.font.underline);
        CHECK(restored.mnemonic_font.bold == original.mnemonic_font.bold);
    }

    SUBCASE("Panel style round-trip") {
        panel_style original{
            .background = color{30, 30, 30},
            .border_color = color{200, 200, 200},
            .box_style = conio_renderer::box_style{conio_renderer::border_style::heavy, true},
            .has_border = false
        };

        auto yaml = to_yaml(original);
        auto restored = from_yaml<panel_style>(yaml);

        CHECK(restored.background.r == original.background.r);
        CHECK(restored.border_color.r == original.border_color.r);
        CHECK(restored.box_style == original.box_style);
        CHECK(restored.has_border == original.has_border);
    }
}

TEST_CASE("Widget Styles - Text alignment variations") {
    SUBCASE("Left alignment") {
        button_style style{
            .normal = {
                .foreground = color{255, 255, 255},
                .background = color{0, 0, 0}
            },
            .hover = {
                .foreground = color{255, 255, 255},
                .background = color{0, 0, 0}
            },
            .pressed = {
                .foreground = color{255, 255, 255},
                .background = color{0, 0, 0}
            },
            .disabled = {
                .foreground = color{255, 255, 255},
                .background = color{0, 0, 0}
            },
            .text_align = horizontal_alignment::left
        };

        auto yaml = to_yaml(style);
        CHECK(yaml["text_align"].get_value<std::string>() == "left");
    }

    SUBCASE("Right alignment") {
        std::string yaml_str = R"(
normal:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
hover:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
pressed:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
disabled:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
text_align: right
)";

        auto style = from_yaml_string<button_style>(yaml_str);
        CHECK(style.text_align == horizontal_alignment::right);
    }

    SUBCASE("Stretch alignment") {
        std::string yaml_str = R"(
normal:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
hover:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
pressed:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
disabled:
  foreground: [255, 255, 255]
  background: [0, 0, 0]
text_align: stretch
)";

        auto style = from_yaml_string<button_style>(yaml_str);
        CHECK(style.text_align == horizontal_alignment::stretch);
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Widget Styles - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
