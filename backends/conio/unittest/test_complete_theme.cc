//
// Complete Theme Reflection Tests
// Tests serialization/deserialization of complete ui_theme<conio_backend>
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <../../../include/onyxui/utils/fkyaml_adapter.hh>
#include <../../../include/onyxui/theming/theme.hh>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/enum_reflection.hh>

using namespace onyxui::yaml;
using namespace onyxui;
using namespace onyxui::conio;

using theme_type = ui_theme<conio_backend>;

namespace {
    // Helper function to create a sample theme
    theme_type create_sample_theme() {
        theme_type theme;

        // Metadata
        theme.name = "Test Theme";
        theme.description = "A test theme for validation";

        // Button style
        theme.button.normal.foreground = color{255, 255, 255};
        theme.button.normal.background = color{0, 0, 170};
        theme.button.hover.foreground = color{255, 255, 0};
        theme.button.hover.background = color{0, 170, 170};
        theme.button.pressed.foreground = color{0, 0, 0};
        theme.button.pressed.background = color{170, 170, 170};
        theme.button.disabled.foreground = color{128, 128, 128};
        theme.button.disabled.background = color{64, 64, 64};
        theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};
        theme.button.normal.font = {true, false, false};
        theme.button.mnemonic_font = {true, true, false};
        theme.button.padding_horizontal = 4;
        theme.button.padding_vertical = 2;
        theme.button.text_align = horizontal_alignment::center;

        // Label style
        theme.label.text = color{255, 255, 255};
        theme.label.background = color{0, 0, 170};
        theme.label.font = {false, false, false};
        theme.label.mnemonic_font = {false, true, false};

        // Panel style
        theme.panel.background = color{0, 0, 170};
        theme.panel.border_color = color{255, 255, 255};
        theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};
        theme.panel.has_border = true;

        // Global palette
        theme.window_bg = color{0, 0, 170};
        theme.text_fg = color{255, 255, 255};
        theme.border_color = color{170, 170, 170};

        return theme;
    }
}

TEST_CASE("Complete Theme - Serialization") {
    SUBCASE("Full theme serialization") {
        auto theme = create_sample_theme();
        auto yaml = to_yaml(theme);

        CHECK(yaml.is_mapping());

        // Check metadata
        CHECK(yaml["name"].get_value<std::string>() == "Test Theme");
        CHECK(yaml["description"].get_value<std::string>() == "A test theme for validation");

        // Check button style exists and has correct structure
        CHECK(yaml["button"].is_mapping());
        CHECK(yaml["button"]["normal"].is_mapping());
        CHECK(yaml["button"]["normal"]["foreground"]["r"].get_value<int>() == 255);
        CHECK(yaml["button"]["normal"]["background"].is_mapping());
        CHECK(yaml["button"]["normal"]["background"]["b"].get_value<int>() == 170);
        CHECK(yaml["button"]["box_style"].get_value<std::string>() == "double_line");
        CHECK(yaml["button"]["text_align"].get_value<std::string>() == "center");
        CHECK(yaml["button"]["normal"]["font"]["bold"].get_value<bool>() == true);

        // Check label style
        CHECK(yaml["label"].is_mapping());
        CHECK(yaml["label"]["text"].is_mapping());
        CHECK(yaml["label"]["text"]["r"].get_value<int>() == 255);
        CHECK(yaml["label"]["font"]["bold"].get_value<bool>() == false);

        // Check panel style
        CHECK(yaml["panel"].is_mapping());
        CHECK(yaml["panel"]["background"].is_mapping());
        CHECK(yaml["panel"]["background"]["b"].get_value<int>() == 170);
        CHECK(yaml["panel"]["has_border"].get_value<bool>() == true);

        // Check global palette
        CHECK(yaml["window_bg"].is_mapping());
        CHECK(yaml["window_bg"]["b"].get_value<int>() == 170);
        CHECK(yaml["text_fg"].is_mapping());
        CHECK(yaml["text_fg"]["r"].get_value<int>() == 255);
        CHECK(yaml["border_color"].is_mapping());
        CHECK(yaml["border_color"]["r"].get_value<int>() == 170);
    }

    SUBCASE("Theme to YAML string") {
        auto theme = create_sample_theme();
        std::string yaml_str = to_yaml_string(theme);

        // Verify it contains key elements
        CHECK(yaml_str.find("name:") != std::string::npos);
        CHECK(yaml_str.find("Test Theme") != std::string::npos);
        CHECK(yaml_str.find("button:") != std::string::npos);
        CHECK(yaml_str.find("label:") != std::string::npos);
        CHECK(yaml_str.find("panel:") != std::string::npos);
        CHECK(yaml_str.find("window_bg:") != std::string::npos);
    }
}

TEST_CASE("Complete Theme - Deserialization") {
    SUBCASE("Full theme from YAML") {
        std::string yaml_str = R"(
name: "Dark Mode"
description: "A dark theme for night coding"
button:
  normal:
    foreground: [255, 255, 255]
    background: [30, 30, 30]
    font:
      bold: true
      underline: false
      reverse: false
  hover:
    foreground: [255, 255, 0]
    background: [50, 50, 50]
  pressed:
    foreground: [200, 200, 200]
    background: [20, 20, 20]
  disabled:
    foreground: [100, 100, 100]
    background: [40, 40, 40]
  box_style: rounded
  mnemonic_font:
    bold: true
    underline: true
    reverse: false
  padding_horizontal: 6
  padding_vertical: 3
  text_align: left
  corner_radius: 2
label:
  text: [200, 200, 200]
  background: [30, 30, 30]
  font:
    bold: false
    underline: false
    reverse: false
  mnemonic_font:
    bold: false
    underline: true
    reverse: false
panel:
  background: [40, 40, 40]
  border_color: [100, 100, 100]
  box_style: heavy
  has_border: true
window_bg: [20, 20, 20]
text_fg: [220, 220, 220]
border_color: [80, 80, 80]
)";

        auto theme = from_yaml_string<theme_type>(yaml_str);

        // Check metadata
        CHECK(theme.name == "Dark Mode");
        CHECK(theme.description == "A dark theme for night coding");

        // Check button style
        CHECK(theme.button.normal.foreground.r == 255);
        CHECK(theme.button.normal.background.r == 30);
        CHECK(theme.button.box_style.style == conio_renderer::border_style::rounded);
        CHECK(theme.button.text_align == horizontal_alignment::left);
        CHECK(theme.button.normal.font.bold == true);
        CHECK(theme.button.padding_horizontal == 6);

        // Check label style
        CHECK(theme.label.text.r == 200);
        CHECK(theme.label.background.r == 30);
        CHECK(theme.label.font.bold == false);

        // Check panel style
        CHECK(theme.panel.background.r == 40);
        CHECK(theme.panel.border_color.r == 100);
        CHECK(theme.panel.box_style.style == conio_renderer::border_style::heavy);
        CHECK(theme.panel.has_border == true);

        // Check global palette
        CHECK(theme.window_bg.r == 20);
        CHECK(theme.text_fg.r == 220);
        CHECK(theme.border_color.r == 80);
    }

    SUBCASE("Theme with minimal fields (defaults)") {
        std::string yaml_str = R"(
name: "Minimal Theme"
description: "Uses many defaults"
button:
  normal:
    foreground: [255, 255, 255]
    background: [0, 0, 0]
  hover:
    foreground: [255, 255, 255]
    background: [50, 50, 50]
  pressed:
    foreground: [255, 255, 255]
    background: [0, 0, 0]
  disabled:
    foreground: [128, 128, 128]
    background: [0, 0, 0]
label:
  text: [255, 255, 255]
  background: [0, 0, 0]
panel:
  background: [0, 0, 0]
  border_color: [128, 128, 128]
window_bg: [0, 0, 0]
text_fg: [255, 255, 255]
border_color: [128, 128, 128]
)";

        auto theme = from_yaml_string<theme_type>(yaml_str);

        CHECK(theme.name == "Minimal Theme");

        // Check that defaults are used
        CHECK(theme.button.padding_horizontal == 4);  // Default
        CHECK(theme.button.padding_vertical == 4);    // Default
        CHECK(theme.button.text_align == horizontal_alignment::center);  // Default
        CHECK(theme.panel.has_border == true);  // Default
    }
}

TEST_CASE("Complete Theme - Round-trip preservation") {
    SUBCASE("Complete round-trip") {
        auto original = create_sample_theme();

        // Serialize to YAML
        auto yaml = to_yaml(original);

        // Deserialize back
        auto restored = from_yaml<theme_type>(yaml);

        // Check metadata
        CHECK(restored.name == original.name);
        CHECK(restored.description == original.description);

        // Check button style
        CHECK(restored.button.normal.foreground.r == original.button.normal.foreground.r);
        CHECK(restored.button.normal.background.b == original.button.normal.background.b);
        CHECK(restored.button.box_style == original.button.box_style);
        CHECK(restored.button.normal.font.bold == original.button.normal.font.bold);
        CHECK(restored.button.text_align == original.button.text_align);
        CHECK(restored.button.padding_horizontal == original.button.padding_horizontal);

        // Check label style
        CHECK(restored.label.text.r == original.label.text.r);
        CHECK(restored.label.background.b == original.label.background.b);
        CHECK(restored.label.font.underline == original.label.font.underline);
        CHECK(restored.label.mnemonic_font.underline == original.label.mnemonic_font.underline);

        // Check panel style
        CHECK(restored.panel.background.b == original.panel.background.b);
        CHECK(restored.panel.border_color.r == original.panel.border_color.r);
        CHECK(restored.panel.box_style == original.panel.box_style);
        CHECK(restored.panel.has_border == original.panel.has_border);

        // Check global palette
        CHECK(restored.window_bg.b == original.window_bg.b);
        CHECK(restored.text_fg.r == original.text_fg.r);
        CHECK(restored.border_color.r == original.border_color.r);
    }

    SUBCASE("Double round-trip (serialize twice should be identical)") {
        auto original = create_sample_theme();

        // First round-trip
        auto yaml1 = to_yaml(original);
        auto restored1 = from_yaml<theme_type>(yaml1);

        // Second round-trip
        auto yaml2 = to_yaml(restored1);
        auto restored2 = from_yaml<theme_type>(yaml2);

        // Both should be identical to original
        CHECK(restored1.name == original.name);
        CHECK(restored2.name == original.name);
        CHECK(restored1.button.normal.foreground.r == original.button.normal.foreground.r);
        CHECK(restored2.button.normal.foreground.r == original.button.normal.foreground.r);
    }
}

TEST_CASE("Complete Theme - Multiple themes") {
    SUBCASE("Load multiple different themes") {
        std::string theme1_yaml = R"(
name: "Blue Theme"
description: "Classic blue"
button:
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
label:
  text: [255, 255, 255]
  background: [0, 0, 170]
panel:
  background: [0, 0, 170]
  border_color: [255, 255, 255]
window_bg: [0, 0, 170]
text_fg: [255, 255, 255]
border_color: [170, 170, 170]
)";

        std::string theme2_yaml = R"(
name: "Dark Theme"
description: "Dark mode"
button:
  normal:
    foreground: [200, 200, 200]
    background: [30, 30, 30]
  hover:
    foreground: [255, 255, 255]
    background: [50, 50, 50]
  pressed:
    foreground: [180, 180, 180]
    background: [20, 20, 20]
  disabled:
    foreground: [100, 100, 100]
    background: [40, 40, 40]
label:
  text: [200, 200, 200]
  background: [30, 30, 30]
panel:
  background: [30, 30, 30]
  border_color: [100, 100, 100]
window_bg: [20, 20, 20]
text_fg: [220, 220, 220]
border_color: [80, 80, 80]
)";

        auto theme1 = from_yaml_string<theme_type>(theme1_yaml);
        auto theme2 = from_yaml_string<theme_type>(theme2_yaml);

        // Themes should be different
        CHECK(theme1.name != theme2.name);
        CHECK(theme1.description != theme2.description);
        CHECK(theme1.window_bg.r != theme2.window_bg.r);
        CHECK(theme1.button.normal.background.b != theme2.button.normal.background.b);

        // Each theme should have its own values
        CHECK(theme1.name == "Blue Theme");
        CHECK(theme2.name == "Dark Theme");
        CHECK(theme1.window_bg.b == 170);
        CHECK(theme2.window_bg.r == 20);
    }
}

TEST_CASE("Complete Theme - Error handling") {
    SUBCASE("Invalid YAML structure") {
        const std::string invalid_yaml = "this is not: valid: yaml: [";
        CHECK_THROWS(from_yaml_string<theme_type>(invalid_yaml));
    }

    SUBCASE("Missing required color fields in button") {
        const std::string yaml_str = R"(
name: "Incomplete"
description: "Missing button colors"
button:
  normal:
    foreground: [255, 255, 255]
  # Missing other required colors
label:
  text: [255, 255, 255]
  background: [0, 0, 0]
panel:
  background: [0, 0, 0]
  border_color: [128, 128, 128]
window_bg: [0, 0, 0]
text_fg: [255, 255, 255]
border_color: [128, 128, 128]
)";

        // Should use defaults for missing fields (reflect-cpp behavior)
        auto theme = from_yaml_string<theme_type>(yaml_str);
        CHECK(theme.name == "Incomplete");
        // Missing color fields will have default-constructed values (0, 0, 0)
    }
}

TEST_CASE("Complete Theme - Field validation") {
    SUBCASE("All widget styles present") {
        auto theme = create_sample_theme();
        auto yaml = to_yaml(theme);

        // Verify all major sections exist
        CHECK(yaml["button"].is_mapping());
        CHECK(yaml["label"].is_mapping());
        CHECK(yaml["panel"].is_mapping());
        CHECK(yaml["window_bg"].is_mapping());
        CHECK(yaml["text_fg"].is_mapping());
        CHECK(yaml["border_color"].is_mapping());
    }

    SUBCASE("Button has all required fields") {
        auto theme = create_sample_theme();
        auto yaml = to_yaml(theme);
        auto button = yaml["button"];

        // All 4 visual states with their color properties
        CHECK(button["normal"].is_mapping());
        CHECK(button["normal"]["foreground"].is_mapping());
        CHECK(button["normal"]["background"].is_mapping());
        CHECK(button["hover"].is_mapping());
        CHECK(button["hover"]["foreground"].is_mapping());
        CHECK(button["hover"]["background"].is_mapping());
        CHECK(button["pressed"].is_mapping());
        CHECK(button["pressed"]["foreground"].is_mapping());
        CHECK(button["pressed"]["background"].is_mapping());
        CHECK(button["disabled"].is_mapping());
        CHECK(button["disabled"]["foreground"].is_mapping());
        CHECK(button["disabled"]["background"].is_mapping());

        // Styles and preferences
        CHECK(button["box_style"].is_string());
        CHECK(button["normal"]["font"].is_mapping());
        CHECK(button["mnemonic_font"].is_mapping());
        CHECK(button["padding_horizontal"].is_integer());
        CHECK(button["padding_vertical"].is_integer());
        CHECK(button["text_align"].is_string());
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Complete Theme - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
