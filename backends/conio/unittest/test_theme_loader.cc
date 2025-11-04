//
// Theme Loader Tests
// Tests file I/O for loading and saving themes
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <../../../include/onyxui/utils/fkyaml_adapter.hh>
#include <../../../include/onyxui/theming/theme.hh>
#include <../../../include/onyxui/theming/theme_loader.hh>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/enum_reflection.hh>
#include <filesystem>
#include <fstream>

using namespace onyxui;
using namespace onyxui::conio;

using theme_type = ui_theme<conio_backend>;

// Helper to create a test theme
theme_type create_test_theme() {
    theme_type theme;
    theme.name = "Test Theme";
    theme.description = "A theme for testing file I/O";

    theme.button.normal = {
        .font = {true, false, false},
        .foreground = color{255, 255, 255},
        .background = color{0, 0, 170},
        .mnemonic_foreground = color{255, 255, 255}
    };
    theme.button.hover = {
        .font = {true, false, false},
        .foreground = color{255, 255, 0},
        .background = color{0, 170, 170},
        .mnemonic_foreground = color{255, 255, 0}
    };
    theme.button.pressed = {
        .font = {false, false, false},
        .foreground = color{0, 0, 0},
        .background = color{170, 170, 170},
        .mnemonic_foreground = color{0, 0, 0}
    };
    theme.button.disabled = {
        .font = {false, false, false},
        .foreground = color{128, 128, 128},
        .background = color{64, 64, 64},
        .mnemonic_foreground = color{128, 128, 128}
    };
    theme.button.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};
    theme.button.text_align = horizontal_alignment::center;

    theme.label.text = color{255, 255, 255};
    theme.label.background = color{0, 0, 170};

    theme.panel.background = color{0, 0, 170};
    theme.panel.border_color = color{255, 255, 255};
    theme.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};

    theme.window_bg = color{0, 0, 170};
    theme.text_fg = color{255, 255, 255};
    theme.border_color = color{170, 170, 170};

    return theme;
}

TEST_CASE("Theme Loader - Save and load from file") {
    // Create a temporary directory for test files
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "onyxui_test_themes";
    std::filesystem::create_directories(test_dir);

    SUBCASE("Save and load round-trip") {
        auto original = create_test_theme();
        auto file_path = test_dir / "test_theme.yaml";

        // Save theme
        theme_loader::save_to_file(original, file_path);

        // Verify file exists
        CHECK(std::filesystem::exists(file_path));

        // Load theme back
        auto loaded = theme_loader::load_from_file<conio_backend>(file_path);

        // Verify all fields match
        CHECK(loaded.name == original.name);
        CHECK(loaded.description == original.description);
        CHECK(loaded.button.normal.foreground.r == original.button.normal.foreground.r);
        CHECK(loaded.button.normal.background.b == original.button.normal.background.b);
        CHECK(loaded.button.box_style == original.button.box_style);
        CHECK(loaded.label.text.r == original.label.text.r);
        CHECK(loaded.panel.has_border == original.panel.has_border);
        CHECK(loaded.window_bg.b == original.window_bg.b);

        // Cleanup
        std::filesystem::remove(file_path);
    }

    SUBCASE("Save overwrites existing file") {
        auto file_path = test_dir / "overwrite_test.yaml";

        // Save first theme
        auto theme1 = create_test_theme();
        theme1.name = "First Theme";
        theme_loader::save_to_file(theme1, file_path);

        // Save second theme (overwrite)
        auto theme2 = create_test_theme();
        theme2.name = "Second Theme";
        theme_loader::save_to_file(theme2, file_path);

        // Load and verify it's the second theme
        auto loaded = theme_loader::load_from_file<conio_backend>(file_path);
        CHECK(loaded.name == "Second Theme");

        // Cleanup
        std::filesystem::remove(file_path);
    }

    SUBCASE("Save creates parent directories") {
        auto file_path = test_dir / "nested" / "dir" / "theme.yaml";

        // Save theme (should create nested/dir/)
        auto theme = create_test_theme();
        theme_loader::save_to_file(theme, file_path);

        // Verify file exists
        CHECK(std::filesystem::exists(file_path));

        // Verify directories were created
        CHECK(std::filesystem::exists(test_dir / "nested"));
        CHECK(std::filesystem::exists(test_dir / "nested" / "dir"));

        // Cleanup
        std::filesystem::remove_all(test_dir / "nested");
    }

    // Cleanup test directory
    std::filesystem::remove_all(test_dir);
}

TEST_CASE("Theme Loader - String operations") {
    SUBCASE("to_yaml_string produces valid YAML") {
        auto theme = create_test_theme();
        std::string yaml = theme_loader::to_yaml_string(theme);

        // Check that it contains expected keys
        CHECK(yaml.find("name:") != std::string::npos);
        CHECK(yaml.find("Test Theme") != std::string::npos);
        CHECK(yaml.find("button:") != std::string::npos);
        CHECK(yaml.find("label:") != std::string::npos);
        CHECK(yaml.find("panel:") != std::string::npos);
        CHECK(yaml.find("window_bg:") != std::string::npos);
    }

    SUBCASE("load_from_string and to_yaml_string round-trip") {
        auto original = create_test_theme();

        // Convert to YAML string
        std::string yaml = theme_loader::to_yaml_string(original);

        // Load from string
        auto loaded = theme_loader::load_from_string<conio_backend>(yaml);

        // Verify fields match
        CHECK(loaded.name == original.name);
        CHECK(loaded.button.normal.foreground.r == original.button.normal.foreground.r);
        CHECK(loaded.window_bg.b == original.window_bg.b);
    }

    SUBCASE("Hybrid: to_yaml then serialize then deserialize then from_yaml") {
        using namespace onyxui::yaml;

        theme_type original;
        original.name = "Test";
        original.button.normal.foreground = color{255, 255, 255};
        original.button.normal.background = color{0, 0, 170};
        original.window_bg = color{0, 0, 170};

        // Step 1: to_yaml (WORKS in Phase 6)
        auto node1 = to_yaml(original);

        // Step 2: serialize to string
        std::string yaml_str = fkyaml::node::serialize(node1);

        // Step 3: deserialize from string
        auto node2 = fkyaml::node::deserialize(yaml_str);

        // Step 4: from_yaml (WORKS in Phase 6)
        auto restored = from_yaml<theme_type>(node2);

        CHECK(restored.button.normal.foreground.r == 255);
        CHECK(restored.window_bg.b == 170);
    }

    SUBCASE("EXACT Phase 6 copy") {
        using namespace onyxui::yaml;

        // EXACT COPY from test_complete_theme.cc line 230-237
        theme_type original;
        original.name = "Test Theme";
        original.description = "A test theme for validation";
        original.button.normal = {
            .font = {true, false, false},
            .foreground = color{255, 255, 255},
            .background = color{0, 0, 170},
            .mnemonic_foreground = color{255, 255, 255}
        };
        original.button.hover = {
            .font = {true, false, false},
            .foreground = color{255, 255, 0},
            .background = color{0, 170, 170},
            .mnemonic_foreground = color{255, 255, 0}
        };
        original.button.pressed = {
            .font = {false, false, false},
            .foreground = color{0, 0, 0},
            .background = color{170, 170, 170},
            .mnemonic_foreground = color{0, 0, 0}
        };
        original.button.disabled = {
            .font = {false, false, false},
            .foreground = color{128, 128, 128},
            .background = color{64, 64, 64},
            .mnemonic_foreground = color{128, 128, 128}
        };
        original.button.mnemonic_font = {true, true, false};
        original.button.box_style = conio_renderer::box_style{conio_renderer::border_style::double_line, true};
        original.button.padding_horizontal = 4;
        original.button.padding_vertical = 2;
        original.button.text_align = horizontal_alignment::center;
        original.label.text = color{255, 255, 255};
        original.label.background = color{0, 0, 170};
        original.label.font = {false, false, false};
        original.label.mnemonic_font = {false, true, false};
        original.panel.background = color{0, 0, 170};
        original.panel.border_color = color{255, 255, 255};
        original.panel.box_style = conio_renderer::box_style{conio_renderer::border_style::single_line, true};
        original.panel.has_border = true;
        original.window_bg = color{0, 0, 170};
        original.text_fg = color{255, 255, 255};
        original.border_color = color{170, 170, 170};

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

    SUBCASE("load_from_string with custom YAML") {
        std::string yaml = R"(
name: "Custom Theme"
description: "Loaded from string"
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
  box_style: rounded
  text_align: left
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

        auto theme = theme_loader::load_from_string<conio_backend>(yaml);

        CHECK(theme.name == "Custom Theme");
        CHECK(theme.description == "Loaded from string");
        CHECK(theme.button.normal.foreground.r == 200);
        CHECK(theme.button.box_style.style == conio_renderer::border_style::rounded);
        CHECK(theme.button.text_align == horizontal_alignment::left);
        CHECK(theme.window_bg.r == 20);
    }
}

TEST_CASE("Theme Loader - Error handling") {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "onyxui_test_themes";
    std::filesystem::create_directories(test_dir);

    SUBCASE("load_from_file throws for non-existent file") {
        auto file_path = test_dir / "does_not_exist.yaml";
        CHECK_THROWS_AS(
            theme_loader::load_from_file<conio_backend>(file_path),
            std::runtime_error
        );
    }

    SUBCASE("load_from_file error message includes filename") {
        auto file_path = test_dir / "missing.yaml";
        try {
            theme_loader::load_from_file<conio_backend>(file_path);
            FAIL("Should have thrown exception");
        } catch (const std::runtime_error& e) {
            std::string msg = e.what();
            CHECK(msg.find("missing.yaml") != std::string::npos);
        }
    }

    SUBCASE("load_from_string throws for invalid YAML") {
        std::string invalid_yaml = "this is not: valid: yaml: [[[";
        CHECK_THROWS_AS(
            theme_loader::load_from_string<conio_backend>(invalid_yaml),
            std::runtime_error
        );
    }

    SUBCASE("load_from_string with missing required fields") {
        // Missing all button color fields - deserialization will succeed but use defaults
        std::string minimal_yaml = "name: Test";
        auto theme = theme_loader::load_from_string<conio_backend>(minimal_yaml);

        // Should have loaded with default color values (0, 0, 0)
        CHECK(theme.name == "Test");
        CHECK(theme.button.normal.foreground.r == 0);  // Default value
    }

    SUBCASE("load_from_file with invalid YAML content") {
        auto file_path = test_dir / "invalid.yaml";

        // Create file with invalid YAML
        std::ofstream file(file_path);
        file << "this is: not: valid: yaml: [[[";
        file.close();

        CHECK_THROWS_AS(
            theme_loader::load_from_file<conio_backend>(file_path),
            std::runtime_error
        );

        // Cleanup
        std::filesystem::remove(file_path);
    }

    // Cleanup
    std::filesystem::remove_all(test_dir);
}

TEST_CASE("Theme Loader - is_theme_file utility") {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "onyxui_test_themes";
    std::filesystem::create_directories(test_dir);

    SUBCASE("Recognizes .yaml files") {
        auto file_path = test_dir / "theme.yaml";

        // Create empty file
        std::ofstream file(file_path);
        file << "test";
        file.close();

        CHECK(theme_loader::is_theme_file(file_path));

        std::filesystem::remove(file_path);
    }

    SUBCASE("Recognizes .yml files") {
        auto file_path = test_dir / "theme.yml";

        std::ofstream file(file_path);
        file << "test";
        file.close();

        CHECK(theme_loader::is_theme_file(file_path));

        std::filesystem::remove(file_path);
    }

    SUBCASE("Rejects non-YAML files") {
        auto file_path = test_dir / "theme.txt";

        std::ofstream file(file_path);
        file << "test";
        file.close();

        CHECK_FALSE(theme_loader::is_theme_file(file_path));

        std::filesystem::remove(file_path);
    }

    SUBCASE("Rejects non-existent files") {
        auto file_path = test_dir / "does_not_exist.yaml";
        CHECK_FALSE(theme_loader::is_theme_file(file_path));
    }

    // Cleanup
    std::filesystem::remove_all(test_dir);
}

TEST_CASE("Theme Loader - Multiple themes in succession") {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "onyxui_test_themes";
    std::filesystem::create_directories(test_dir);

    SUBCASE("Load multiple different theme files") {
        // Create first theme
        auto theme1 = create_test_theme();
        theme1.name = "Blue Theme";
        theme1.window_bg = color{0, 0, 170};
        auto file1 = test_dir / "blue.yaml";
        theme_loader::save_to_file(theme1, file1);

        // Create second theme
        auto theme2 = create_test_theme();
        theme2.name = "Dark Theme";
        theme2.window_bg = color{30, 30, 30};
        auto file2 = test_dir / "dark.yaml";
        theme_loader::save_to_file(theme2, file2);

        // Load both themes
        auto loaded1 = theme_loader::load_from_file<conio_backend>(file1);
        auto loaded2 = theme_loader::load_from_file<conio_backend>(file2);

        // Verify they're different
        CHECK(loaded1.name != loaded2.name);
        CHECK(loaded1.window_bg.r != loaded2.window_bg.r);

        // Verify correct values
        CHECK(loaded1.name == "Blue Theme");
        CHECK(loaded2.name == "Dark Theme");
        CHECK(loaded1.window_bg.b == 170);
        CHECK(loaded2.window_bg.r == 30);

        // Cleanup
        std::filesystem::remove(file1);
        std::filesystem::remove(file2);
    }

    // Cleanup
    std::filesystem::remove_all(test_dir);
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Theme Loader - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
