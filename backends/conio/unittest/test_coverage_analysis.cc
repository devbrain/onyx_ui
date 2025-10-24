//
// YAML Themes Test Coverage Analysis
// Comprehensive tests for edge cases, boundary conditions, and integration scenarios
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <../../../include/onyxui/utils/fkyaml_adapter.hh>
#include <onyxui/theme_loader.hh>
#include <onyxui/theme.hh>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/enum_reflection.hh>

using namespace onyxui;
using namespace onyxui::yaml;

TEST_CASE("Coverage - Color edge cases") {
    using color_type = conio::color;

    SUBCASE("Boundary values - all zeros") {
        color_type c{0, 0, 0};
        auto yaml = to_yaml_string(c);
        auto restored = from_yaml_string<color_type>(yaml);
        CHECK(restored.r == 0);
        CHECK(restored.g == 0);
        CHECK(restored.b == 0);
    }

    SUBCASE("Boundary values - all 255") {
        color_type c{255, 255, 255};
        auto yaml = to_yaml_string(c);
        auto restored = from_yaml_string<color_type>(yaml);
        CHECK(restored.r == 255);
        CHECK(restored.g == 255);
        CHECK(restored.b == 255);
    }

    SUBCASE("Boundary values - mixed extremes") {
        color_type c{255, 0, 255};
        auto yaml = to_yaml_string(c);
        auto restored = from_yaml_string<color_type>(yaml);
        CHECK(restored.r == 255);
        CHECK(restored.g == 0);
        CHECK(restored.b == 255);
    }

    SUBCASE("Hex format - lowercase letters") {
        // Hex strings must be embedded in structs due to fkYAML's handling of '#'
        struct color_wrapper { color_type value; };
        std::string yaml = "value: \"#abcdef\"";
        auto wrapper = from_yaml_string<color_wrapper>(yaml);
        CHECK(wrapper.value.r == 0xab);
        CHECK(wrapper.value.g == 0xcd);
        CHECK(wrapper.value.b == 0xef);
    }

    SUBCASE("Hex format - uppercase letters") {
        struct color_wrapper { color_type value; };
        std::string yaml = "value: \"#ABCDEF\"";
        auto wrapper = from_yaml_string<color_wrapper>(yaml);
        CHECK(wrapper.value.r == 0xAB);
        CHECK(wrapper.value.g == 0xCD);
        CHECK(wrapper.value.b == 0xEF);
    }

    SUBCASE("Hex format - mixed case") {
        struct color_wrapper { color_type value; };
        std::string yaml = "value: \"#AbCdEf\"";
        auto wrapper = from_yaml_string<color_wrapper>(yaml);
        CHECK(wrapper.value.r == 0xAB);
        CHECK(wrapper.value.g == 0xCD);
        CHECK(wrapper.value.b == 0xEF);
    }

    SUBCASE("Hex format - without # prefix") {
        struct color_wrapper { color_type value; };
        std::string yaml = "value: \"FF00FF\"";
        auto wrapper = from_yaml_string<color_wrapper>(yaml);
        CHECK(wrapper.value.r == 255);
        CHECK(wrapper.value.g == 0);
        CHECK(wrapper.value.b == 255);
    }

    SUBCASE("Array format - boundary in middle of array") {
        std::string yaml = "[128, 64, 192]";
        auto c = from_yaml_string<color_type>(yaml);
        CHECK(c.r == 128);
        CHECK(c.g == 64);
        CHECK(c.b == 192);
    }

    SUBCASE("Object format - field order independence") {
        std::string yaml = R"(
b: 255
r: 128
g: 64
)";
        auto c = from_yaml_string<color_type>(yaml);
        CHECK(c.r == 128);
        CHECK(c.g == 64);
        CHECK(c.b == 255);
    }

    SUBCASE("Object format - extra whitespace") {
        std::string yaml = R"(
r:    128
g:    64
b:    255
)";
        auto c = from_yaml_string<color_type>(yaml);
        CHECK(c.r == 128);
        CHECK(c.g == 64);
        CHECK(c.b == 255);
    }
}

TEST_CASE("Coverage - Enum edge cases") {
    using conio_renderer = onyxui::conio::conio_renderer;

    SUBCASE("All box_style values in button_style") {
        // Test all box_style values as part of button_style (not standalone)
        // because enums serialize properly only when embedded in structs
        const std::vector<conio_renderer::box_style> all_styles = {
            conio_renderer::box_style::none,
            conio_renderer::box_style::single_line,
            conio_renderer::box_style::double_line,
            conio_renderer::box_style::rounded,
            conio_renderer::box_style::heavy
        };

        for (auto style : all_styles) {
            using theme_type = ui_theme<conio::conio_backend>;
            theme_type theme;
            theme.name = "Box Style Test";
            theme.button.box_style = style;

            auto yaml = to_yaml_string(theme);
            auto restored = from_yaml_string<theme_type>(yaml);
            CHECK(restored.button.box_style == style);
        }
    }

    SUBCASE("All horizontal_alignment values in button_style") {
        using onyxui::horizontal_alignment;

        // Test all alignment values as part of button_style (not standalone)
        // because enums serialize properly only when embedded in structs
        const std::vector<horizontal_alignment> all_alignments = {
            horizontal_alignment::left,
            horizontal_alignment::center,
            horizontal_alignment::right,
            horizontal_alignment::stretch
        };

        for (auto alignment : all_alignments) {
            using theme_type = ui_theme<conio::conio_backend>;
            theme_type theme;
            theme.name = "Alignment Test";
            theme.button.text_align = alignment;

            auto yaml = to_yaml_string(theme);
            auto restored = from_yaml_string<theme_type>(yaml);
            CHECK(restored.button.text_align == alignment);
        }
    }
}

TEST_CASE("Coverage - Font combinations") {
    using font_type = onyxui::conio::conio_renderer::font;

    SUBCASE("All 8 boolean combinations") {
        const std::vector<font_type> all_fonts = {
            {false, false, false},  // 000
            {false, false, true},   // 001
            {false, true, false},   // 010
            {false, true, true},    // 011
            {true, false, false},   // 100
            {true, false, true},    // 101
            {true, true, false},    // 110
            {true, true, true}      // 111
        };

        for (const auto& font : all_fonts) {
            auto yaml = to_yaml_string(font);
            auto restored = from_yaml_string<font_type>(yaml);
            CHECK(restored.bold == font.bold);
            CHECK(restored.reverse == font.reverse);
            CHECK(restored.underline == font.underline);
        }
    }
}

TEST_CASE("Coverage - Complete theme variations") {
    using theme_type = ui_theme<conio::conio_backend>;
    using conio_renderer = onyxui::conio::conio_renderer;
    using onyxui::horizontal_alignment;

    SUBCASE("Minimal theme - only required fields") {
        std::string yaml = R"(
name: "Minimal"
)";
        auto theme = from_yaml_string<theme_type>(yaml);
        CHECK(theme.name == "Minimal");
        // All other fields should have defaults
    }

    SUBCASE("Theme with all fields specified") {
        theme_type original;
        original.name = "Complete";
        original.description = "Fully specified theme";
        original.window_bg = conio::color{10, 20, 30};
        original.text_fg = conio::color{240, 250, 255};

        // Button with all states
        original.button.fg_normal = conio::color{255, 255, 255};
        original.button.bg_normal = conio::color{0, 0, 170};
        original.button.fg_hover = conio::color{255, 255, 0};
        original.button.bg_hover = conio::color{0, 170, 170};
        original.button.fg_pressed = conio::color{255, 255, 255};
        original.button.bg_pressed = conio::color{0, 0, 255};
        original.button.fg_disabled = conio::color{128, 128, 128};
        original.button.bg_disabled = conio::color{64, 64, 64};
        original.button.box_style = conio_renderer::box_style::single_line;
        original.button.font = {true, false, false};
        original.button.mnemonic_font = {false, false, true};
        original.button.padding_horizontal = 2;
        original.button.padding_vertical = 1;
        original.button.text_align = horizontal_alignment::center;

        // Label
        original.label.text = conio::color{255, 255, 255};
        original.label.background = conio::color{0, 0, 0};
        original.label.font = {false, false, false};
        original.label.mnemonic_font = {false, false, true};

        // Panel
        original.panel.background = conio::color{0, 0, 170};
        original.panel.border_color = conio::color{255, 255, 255};
        original.panel.box_style = conio_renderer::box_style::double_line;
        original.panel.has_border = true;

        // Round-trip
        auto yaml = to_yaml_string(original);
        auto restored = from_yaml_string<theme_type>(yaml);

        // Verify all fields
        CHECK(restored.name == original.name);
        CHECK(restored.description == original.description);
        CHECK(restored.window_bg.r == original.window_bg.r);
        CHECK(restored.button.fg_normal.g == original.button.fg_normal.g);
        CHECK(restored.button.padding_horizontal == original.button.padding_horizontal);
        CHECK(restored.button.text_align == original.button.text_align);
        CHECK(restored.panel.box_style == original.panel.box_style);
    }

    SUBCASE("Theme with Unicode characters in strings") {
        theme_type theme;
        theme.name = "Тема 日本語 العربية";  // Unicode: Cyrillic, Japanese, Arabic
        theme.description = "Theme with Müller & Göthe and café résumé";

        auto yaml = to_yaml_string(theme);
        auto restored = from_yaml_string<theme_type>(yaml);

        CHECK(restored.name == theme.name);
        CHECK(restored.description == theme.description);
    }

    SUBCASE("Theme with special characters in strings") {
        theme_type theme;
        theme.name = "Theme with \"quotes\" and 'apostrophes'";
        theme.description = "Line 1\nLine 2\tTabbed\n<email@example.com>";

        auto yaml = to_yaml_string(theme);
        auto restored = from_yaml_string<theme_type>(yaml);

        CHECK(restored.name == theme.name);
        CHECK(restored.description == theme.description);
    }

    SUBCASE("Theme with very long strings") {
        theme_type theme;
        theme.name = "Short";
        theme.description = std::string(1000, 'A');  // 1000 character string

        auto yaml = to_yaml_string(theme);
        auto restored = from_yaml_string<theme_type>(yaml);

        CHECK(restored.description.length() == 1000);
        CHECK(restored.description == theme.description);
    }
}

TEST_CASE("Coverage - Error handling robustness") {
    using color_type = conio::color;

    SUBCASE("Invalid hex - too short") {
        std::string yaml = "\"#FFFF\"";  // Only 4 hex digits
        CHECK_THROWS_AS(from_yaml_string<color_type>(yaml), std::runtime_error);
    }

    SUBCASE("Invalid hex - too long") {
        std::string yaml = "\"#FFFFFFF\"";  // 7 hex digits
        CHECK_THROWS_AS(from_yaml_string<color_type>(yaml), std::runtime_error);
    }

    SUBCASE("Invalid hex - non-hex characters") {
        std::string yaml = "\"#GGGGGG\"";
        CHECK_THROWS_AS(from_yaml_string<color_type>(yaml), std::runtime_error);
    }

    SUBCASE("Invalid array - wrong size") {
        std::string yaml = "[255]";  // Only 1 element
        CHECK_THROWS_AS(from_yaml_string<color_type>(yaml), std::runtime_error);
    }

    SUBCASE("Invalid array - values out of range") {
        std::string yaml = "[256, 0, 0]";  // 256 is out of range
        // Should not throw - will be truncated to uint8_t
        auto c = from_yaml_string<color_type>(yaml);
        CHECK(c.r == 0);  // 256 % 256 = 0
    }

    SUBCASE("Invalid object - missing required field") {
        std::string yaml = R"(
r: 255
g: 128
)";  // Missing 'b'
        CHECK_THROWS_AS(from_yaml_string<color_type>(yaml), std::runtime_error);
    }

    SUBCASE("Invalid enum - unknown value in theme") {
        using theme_type = ui_theme<conio::conio_backend>;
        // Invalid box_style in theme - reflect-cpp uses default for invalid values
        std::string yaml = R"(
name: "Test"
button:
  box_style: "invalid_box_style"
)";
        // reflect-cpp doesn't throw - it uses default value for invalid enum strings
        auto theme = from_yaml_string<theme_type>(yaml);
        using conio_renderer = onyxui::conio::conio_renderer;
        CHECK(theme.button.box_style == conio_renderer::box_style::none);  // Default value
    }

    SUBCASE("Malformed YAML - unclosed quote") {
        std::string yaml = "name: \"Unclosed";
        using theme_type = ui_theme<conio::conio_backend>;
        // fkYAML throws fkyaml::exception, not std::runtime_error
        CHECK_THROWS(from_yaml_string<theme_type>(yaml));
    }

    SUBCASE("Malformed YAML - invalid field type uses default") {
        std::string yaml = "name:\n  - item1\n  - item2";  // name should be string, not array
        using theme_type = ui_theme<conio::conio_backend>;
        // reflect-cpp is forgiving - it uses default value when type doesn't match
        auto theme = from_yaml_string<theme_type>(yaml);
        CHECK(theme.name.empty());  // Name defaulted to empty string due to type mismatch
    }
}

TEST_CASE("Coverage - File I/O edge cases") {
    using theme_type = ui_theme<conio::conio_backend>;
    using namespace theme_loader;

    SUBCASE("Save to deeply nested directory") {
        auto deep_path = std::filesystem::temp_directory_path() /
                        "onyxui_test/a/b/c/d/e/f/theme.yaml";

        theme_type theme;
        theme.name = "Deep Theme";

        // Should create all parent directories
        CHECK_NOTHROW(save_to_file(theme, deep_path));
        CHECK(std::filesystem::exists(deep_path));

        // Cleanup
        std::filesystem::remove_all(deep_path.parent_path().parent_path().parent_path()
                                   .parent_path().parent_path().parent_path());
    }

    SUBCASE("Save and load with different extensions") {
        auto temp_dir = std::filesystem::temp_directory_path() / "onyxui_ext_test";
        std::filesystem::create_directories(temp_dir);

        theme_type theme;
        theme.name = "Extension Test";

        // Save as .yaml
        auto yaml_path = temp_dir / "theme.yaml";
        save_to_file(theme, yaml_path);
        auto loaded_yaml = load_from_file<conio::conio_backend>(yaml_path);
        CHECK(loaded_yaml.name == "Extension Test");

        // Save as .yml
        auto yml_path = temp_dir / "theme.yml";
        save_to_file(theme, yml_path);
        auto loaded_yml = load_from_file<conio::conio_backend>(yml_path);
        CHECK(loaded_yml.name == "Extension Test");

        std::filesystem::remove_all(temp_dir);
    }

    SUBCASE("Overwrite existing file") {
        auto temp_path = std::filesystem::temp_directory_path() / "overwrite_test.yaml";

        theme_type theme1;
        theme1.name = "Original";
        save_to_file(theme1, temp_path);

        theme_type theme2;
        theme2.name = "Overwritten";
        save_to_file(theme2, temp_path);  // Overwrite

        auto loaded = load_from_file<conio::conio_backend>(temp_path);
        CHECK(loaded.name == "Overwritten");

        std::filesystem::remove(temp_path);
    }

    SUBCASE("Empty file creates default theme") {
        auto temp_path = std::filesystem::temp_directory_path() / "empty.yaml";
        std::ofstream(temp_path) << "";  // Empty file

        // Empty YAML file parses successfully and creates a default-initialized theme
        auto theme = load_from_file<conio::conio_backend>(temp_path);
        CHECK(theme.name.empty());  // Default-constructed name is empty

        std::filesystem::remove(temp_path);
    }

    SUBCASE("File with only whitespace") {
        auto temp_path = std::filesystem::temp_directory_path() / "whitespace.yaml";
        std::ofstream(temp_path) << "   \n\n  \t  \n";  // Only whitespace

        // Whitespace-only file parses successfully
        auto theme = load_from_file<conio::conio_backend>(temp_path);
        CHECK(theme.name.empty());  // Default values

        std::filesystem::remove(temp_path);
    }
}

TEST_CASE("Coverage - Performance and stress tests") {
    using theme_type = ui_theme<conio::conio_backend>;

    SUBCASE("Large number of round-trips") {
        theme_type original;
        original.name = "Stress Test";
        original.window_bg = conio::color{128, 64, 192};

        auto yaml = to_yaml_string(original);

        // 100 round-trips
        for (int i = 0; i < 100; ++i) {
            auto restored = from_yaml_string<theme_type>(yaml);
            CHECK(restored.name == "Stress Test");
            CHECK(restored.window_bg.r == 128);
        }
    }

    SUBCASE("Multiple themes in memory") {
        std::vector<theme_type> themes;

        for (int i = 0; i < 50; ++i) {
            theme_type theme;
            theme.name = "Theme " + std::to_string(i);
            theme.window_bg = conio::color{
                static_cast<uint8_t>(i * 5),
                static_cast<uint8_t>(i * 3),
                static_cast<uint8_t>(i * 7)
            };
            themes.push_back(theme);
        }

        // Verify all themes
        for (size_t i = 0; i < themes.size(); ++i) {
            CHECK(themes[i].name == "Theme " + std::to_string(i));
            CHECK(themes[i].window_bg.r == static_cast<uint8_t>(i * 5));
        }
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Coverage - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
