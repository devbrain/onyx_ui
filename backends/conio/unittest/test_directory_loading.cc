//
// Directory Loading Tests
// Tests loading multiple theme files from a directory
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <../../../include/onyxui/theming/theme_loader.hh>
#include <../../../include/onyxui/theming/theme.hh>
#include <onyxui/conio/conio_backend.hh>
#include <filesystem>
#include <fstream>

using namespace onyxui;
using namespace onyxui::theme_loader;

// Helper to create a minimal valid theme YAML
std::string create_minimal_theme_yaml(const std::string& name) {
    return R"(name: ")" + name + R"("
author: "Test Author"
description: "Test theme"
window_bg: {r: 0, g: 0, b: 170}
text_fg: {r: 255, g: 255, b: 255}
)";
}

// Helper to create a test directory with theme files
std::filesystem::path create_test_themes_directory() {
    auto test_dir = std::filesystem::temp_directory_path() / "onyxui_directory_test";
    std::filesystem::create_directories(test_dir);
    return test_dir;
}

TEST_CASE("Directory Loading - Load multiple valid themes") {
    using backend_type = conio::conio_backend;

    SUBCASE("Load all themes from directory") {
        auto test_dir = create_test_themes_directory();

        // Create multiple theme files
        std::ofstream(test_dir / "theme1.yaml") << create_minimal_theme_yaml("Theme 1");
        std::ofstream(test_dir / "theme2.yaml") << create_minimal_theme_yaml("Theme 2");
        std::ofstream(test_dir / "theme3.yml") << create_minimal_theme_yaml("Theme 3");

        // Load all themes
        auto themes = load_from_directory<backend_type>(test_dir);

        // Should have loaded 3 themes
        CHECK(themes.size() == 3);

        // Verify theme names
        bool found_theme1 = false;
        bool found_theme2 = false;
        bool found_theme3 = false;

        for (const auto& theme : themes) {
            if (theme.name == "Theme 1") found_theme1 = true;
            if (theme.name == "Theme 2") found_theme2 = true;
            if (theme.name == "Theme 3") found_theme3 = true;
        }

        CHECK(found_theme1);
        CHECK(found_theme2);
        CHECK(found_theme3);

        // Cleanup
        std::filesystem::remove_all(test_dir);
    }

    SUBCASE("Empty directory returns empty vector") {
        auto test_dir = create_test_themes_directory();

        auto themes = load_from_directory<backend_type>(test_dir);

        CHECK(themes.empty());

        std::filesystem::remove_all(test_dir);
    }

    SUBCASE("Directory with non-YAML files") {
        auto test_dir = create_test_themes_directory();

        // Create theme files and non-theme files
        std::ofstream(test_dir / "theme1.yaml") << create_minimal_theme_yaml("Theme 1");
        std::ofstream(test_dir / "readme.txt") << "This is a readme file";
        std::ofstream(test_dir / "config.json") << "{}";
        std::ofstream(test_dir / "theme2.yml") << create_minimal_theme_yaml("Theme 2");

        auto themes = load_from_directory<backend_type>(test_dir);

        // Should only load 2 YAML files
        CHECK(themes.size() == 2);

        std::filesystem::remove_all(test_dir);
    }
}

TEST_CASE("Directory Loading - Skip invalid files gracefully") {
    using backend_type = conio::conio_backend;

    SUBCASE("Mixed valid and invalid YAML files") {
        auto test_dir = create_test_themes_directory();

        // Create valid themes
        std::ofstream(test_dir / "valid1.yaml") << create_minimal_theme_yaml("Valid Theme 1");
        std::ofstream(test_dir / "valid2.yaml") << create_minimal_theme_yaml("Valid Theme 2");

        // Create invalid YAML (syntax error - unclosed string)
        std::ofstream(test_dir / "invalid_syntax.yaml") << "name: \"Broken\nauthor: test";

        // Create YAML with completely wrong structure (not a mapping)
        std::ofstream(test_dir / "invalid_structure.yaml") << "- item1\n- item2\n- item3";

        // Load without errors function - should skip invalid files
        auto themes = load_from_directory<backend_type>(test_dir);

        // Should only load 2 valid themes (invalid files skipped)
        CHECK(themes.size() == 2);

        // Both valid themes should be present
        bool found_valid1 = false;
        bool found_valid2 = false;

        for (const auto& theme : themes) {
            if (theme.name == "Valid Theme 1") found_valid1 = true;
            if (theme.name == "Valid Theme 2") found_valid2 = true;
        }

        CHECK(found_valid1);
        CHECK(found_valid2);

        std::filesystem::remove_all(test_dir);
    }
}

TEST_CASE("Directory Loading - Error reporting with load_from_directory_with_errors") {
    using backend_type = conio::conio_backend;

    SUBCASE("Get error details for failed loads") {
        auto test_dir = create_test_themes_directory();

        // Create valid and invalid themes
        std::ofstream(test_dir / "valid.yaml") << create_minimal_theme_yaml("Valid");
        std::ofstream(test_dir / "invalid.yaml") << "name: \"Broken\n";

        // Load with error reporting
        auto results = load_from_directory_with_errors<backend_type>(test_dir);

        // Should have 2 results
        CHECK(results.size() == 2);

        // Count successes and failures
        int success_count = 0;
        int failure_count = 0;

        for (const auto& result : results) {
            if (result.success) {
                success_count++;
                CHECK(result.error_message.empty());
                CHECK(result.theme.name == "Valid");
            } else {
                failure_count++;
                CHECK(!result.error_message.empty());
            }
        }

        CHECK(success_count == 1);
        CHECK(failure_count == 1);

        std::filesystem::remove_all(test_dir);
    }
}

TEST_CASE("Directory Loading - Recursive directory traversal") {
    using backend_type = conio::conio_backend;

    SUBCASE("Recursive loading finds themes in subdirectories") {
        auto test_dir = create_test_themes_directory();
        auto subdir1 = test_dir / "dark_themes";
        auto subdir2 = test_dir / "light_themes";

        std::filesystem::create_directories(subdir1);
        std::filesystem::create_directories(subdir2);

        // Create themes in root
        std::ofstream(test_dir / "default.yaml") << create_minimal_theme_yaml("Default");

        // Create themes in subdirectories
        std::ofstream(subdir1 / "dark1.yaml") << create_minimal_theme_yaml("Dark 1");
        std::ofstream(subdir1 / "dark2.yaml") << create_minimal_theme_yaml("Dark 2");
        std::ofstream(subdir2 / "light1.yaml") << create_minimal_theme_yaml("Light 1");

        // Non-recursive should only find root theme
        auto non_recursive = load_from_directory<backend_type>(test_dir, false);
        CHECK(non_recursive.size() == 1);

        // Recursive should find all 4 themes
        auto recursive = load_from_directory<backend_type>(test_dir, true);
        CHECK(recursive.size() == 4);

        std::filesystem::remove_all(test_dir);
    }
}

TEST_CASE("Directory Loading - Error handling") {
    using backend_type = conio::conio_backend;

    SUBCASE("Non-existent directory throws exception") {
        std::filesystem::path non_existent = "/tmp/this_directory_does_not_exist_12345";

        CHECK_THROWS_AS(
            load_from_directory<backend_type>(non_existent),
            std::runtime_error
        );
    }

    SUBCASE("Path is file, not directory") {
        auto test_dir = create_test_themes_directory();
        auto file_path = test_dir / "file.txt";
        std::ofstream(file_path) << "test";

        CHECK_THROWS_AS(
            load_from_directory<backend_type>(file_path),
            std::runtime_error
        );

        std::filesystem::remove_all(test_dir);
    }
}

TEST_CASE("Directory Loading - File extension handling") {
    using backend_type = conio::conio_backend;

    SUBCASE("Both .yaml and .yml extensions work") {
        auto test_dir = create_test_themes_directory();

        std::ofstream(test_dir / "theme.yaml") << create_minimal_theme_yaml("YAML");
        std::ofstream(test_dir / "theme.yml") << create_minimal_theme_yaml("YML");

        auto themes = load_from_directory<backend_type>(test_dir);

        CHECK(themes.size() == 2);

        std::filesystem::remove_all(test_dir);
    }

    SUBCASE("Files without extension are ignored") {
        auto test_dir = create_test_themes_directory();

        std::ofstream(test_dir / "theme") << create_minimal_theme_yaml("No Extension");
        std::ofstream(test_dir / "theme.yaml") << create_minimal_theme_yaml("With Extension");

        auto themes = load_from_directory<backend_type>(test_dir);

        // Should only load the .yaml file
        CHECK(themes.size() == 1);
        CHECK(themes[0].name == "With Extension");

        std::filesystem::remove_all(test_dir);
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Directory Loading - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
