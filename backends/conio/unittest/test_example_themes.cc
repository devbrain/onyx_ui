//
// Example Themes Loading Tests
// Verifies that all example YAML theme files load correctly
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/theme_loader.hh>
#include <onyxui/theme.hh>
#include <onyxui/conio/conio_backend.hh>

using namespace onyxui;
using namespace onyxui::theme_loader;

// Helper function to find theme file from different working directories
static std::filesystem::path find_theme_path(const std::string& filename) {
    // Try from build/bin directory
    std::filesystem::path path = "../../themes/examples/" + filename;
    if (std::filesystem::exists(path)) {
        return path;
    }

    // Try from project root
    path = "themes/examples/" + filename;
    if (std::filesystem::exists(path)) {
        return path;
    }

    // Not found
    return "";
}

TEST_CASE("Example Themes - Norton Blue") {
    using backend_type = conio::conio_backend;

    auto theme_path = find_theme_path("norton_blue.yaml");
    if (theme_path.empty()) {
        MESSAGE("Skipping - theme files not found");
        return;
    }

    auto theme = load_from_file<backend_type>(theme_path);

    CHECK(theme.name == "Norton Blue");
    CHECK(theme.description == "Classic Norton Utilities color scheme");

    // Verify colors
    CHECK(theme.window_bg.r == 0);
    CHECK(theme.window_bg.g == 0);
    CHECK(theme.window_bg.b == 170);

    CHECK(theme.text_fg.r == 255);
    CHECK(theme.text_fg.g == 255);
    CHECK(theme.text_fg.b == 255);

    // Verify button style
    using conio_renderer = conio::conio_renderer;
    CHECK(theme.button.box_style == conio_renderer::box_style::single_line);
    CHECK(theme.button.padding_horizontal == 2);
}

TEST_CASE("Example Themes - Borland Turbo") {
    using backend_type = conio::conio_backend;

    auto theme = load_from_file<backend_type>(find_theme_path("borland_turbo.yaml"));

    CHECK(theme.name == "Borland Turbo");
    CHECK(theme.description == "Turbo Pascal/C++ IDE color scheme");

    // Verify cyan background
    CHECK(theme.window_bg.r == 0);
    CHECK(theme.window_bg.g == 170);
    CHECK(theme.window_bg.b == 170);

    // Verify double-line box style
    using conio_renderer = conio::conio_renderer;
    CHECK(theme.button.box_style == conio_renderer::box_style::double_line);
    CHECK(theme.panel.box_style == conio_renderer::box_style::double_line);
}

TEST_CASE("Example Themes - Midnight Commander") {
    using backend_type = conio::conio_backend;

    auto theme = load_from_file<backend_type>(find_theme_path("midnight_commander.yaml"));

    CHECK(theme.name == "Midnight Commander");
    CHECK(theme.description == "MC file manager color scheme");

    // Verify dark blue background
    CHECK(theme.window_bg.b == 85);

    // Verify yellow text
    CHECK(theme.text_fg.r == 255);
    CHECK(theme.text_fg.g == 255);
    CHECK(theme.text_fg.b == 85);
}

TEST_CASE("Example Themes - DOS Edit") {
    using backend_type = conio::conio_backend;

    auto theme = load_from_file<backend_type>(find_theme_path("dos_edit.yaml"));

    CHECK(theme.name == "DOS Edit");
    CHECK(theme.description == "MS-DOS Edit text editor colors");

    // Verify light gray background
    CHECK(theme.window_bg.r == 170);
    CHECK(theme.window_bg.g == 170);
    CHECK(theme.window_bg.b == 170);
}

TEST_CASE("Example Themes - High Contrast") {
    using backend_type = conio::conio_backend;

    auto theme = load_from_file<backend_type>(find_theme_path("high_contrast.yaml"));

    CHECK(theme.name == "High Contrast");
    CHECK(theme.description == "High contrast theme for accessibility");

    // Verify pure black background
    CHECK(theme.window_bg.r == 0);
    CHECK(theme.window_bg.g == 0);
    CHECK(theme.window_bg.b == 0);

    // Verify pure white text
    CHECK(theme.text_fg.r == 255);
    CHECK(theme.text_fg.g == 255);
    CHECK(theme.text_fg.b == 255);

    // Verify heavy borders for accessibility
    using conio_renderer = conio::conio_renderer;
    CHECK(theme.button.box_style == conio_renderer::box_style::heavy);
    CHECK(theme.panel.box_style == conio_renderer::box_style::heavy);

    // Verify bold font for readability
    CHECK(theme.button.font.bold == true);
    CHECK(theme.label.font.bold == true);
}

TEST_CASE("Example Themes - Dark Professional") {
    using backend_type = conio::conio_backend;

    auto theme = load_from_file<backend_type>(find_theme_path("dark_professional.yaml"));

    CHECK(theme.name == "Dark Professional");
    CHECK(theme.description == "Modern dark theme for professional use");

    // Verify dark gray (not pure black)
    CHECK(theme.window_bg.r == 30);
    CHECK(theme.window_bg.g == 30);
    CHECK(theme.window_bg.b == 30);

    // Verify modern rounded borders
    using conio_renderer = conio::conio_renderer;
    CHECK(theme.button.box_style == conio_renderer::box_style::rounded);
    CHECK(theme.panel.box_style == conio_renderer::box_style::rounded);

    // Verify modern blue hover color
    CHECK(theme.button.bg_hover.b == 215);
}

TEST_CASE("Example Themes - Light Modern") {
    using backend_type = conio::conio_backend;

    auto theme = load_from_file<backend_type>(find_theme_path("light_modern.yaml"));

    CHECK(theme.name == "Light Modern");
    CHECK(theme.description == "Modern light theme with soft colors");

    // Verify very light background
    CHECK(theme.window_bg.r == 250);
    CHECK(theme.window_bg.g == 250);
    CHECK(theme.window_bg.b == 250);

    // Verify dark text
    CHECK(theme.text_fg.r == 30);
    CHECK(theme.text_fg.g == 30);
    CHECK(theme.text_fg.b == 30);

    // Verify rounded borders
    using conio_renderer = conio::conio_renderer;
    CHECK(theme.button.box_style == conio_renderer::box_style::rounded);
}

TEST_CASE("Example Themes - Load all from directory") {
    using backend_type = conio::conio_backend;

    // Find the themes directory
    std::filesystem::path themes_dir = "../../themes/examples";
    if (!std::filesystem::exists(themes_dir)) {
        themes_dir = "themes/examples";
    }

    if (!std::filesystem::exists(themes_dir)) {
        MESSAGE("Skipping - themes directory not found");
        return;
    }

    // Load all themes from examples directory
    auto themes = load_from_directory<backend_type>(themes_dir);

    // Should have loaded all 10 example themes (7 original + 3 modern: Solarized, Monokai, Gruvbox)
    CHECK(themes.size() == 10);

    // Verify all themes have valid names
    for (const auto& theme : themes) {
        CHECK(!theme.name.empty());
        CHECK(!theme.description.empty());
    }

    // Verify specific themes are present
    bool found_norton = false;
    bool found_borland = false;
    bool found_mc = false;
    bool found_dos_edit = false;
    bool found_high_contrast = false;
    bool found_dark_pro = false;
    bool found_light = false;

    for (const auto& theme : themes) {
        if (theme.name == "Norton Blue") found_norton = true;
        if (theme.name == "Borland Turbo") found_borland = true;
        if (theme.name == "Midnight Commander") found_mc = true;
        if (theme.name == "DOS Edit") found_dos_edit = true;
        if (theme.name == "High Contrast") found_high_contrast = true;
        if (theme.name == "Dark Professional") found_dark_pro = true;
        if (theme.name == "Light Modern") found_light = true;
    }

    CHECK(found_norton);
    CHECK(found_borland);
    CHECK(found_mc);
    CHECK(found_dos_edit);
    CHECK(found_high_contrast);
    CHECK(found_dark_pro);
    CHECK(found_light);
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("Example Themes - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
