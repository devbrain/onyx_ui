//
// Unit tests for theme_palette - Color palette system (Phase 2)
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/theming/theme_palette.hh>

using namespace onyxui::theme_palette;

// ===========================================================================
// extract_palette_from_string() - Palette Extraction Tests
// ===========================================================================

TEST_CASE("theme_palette::extract_palette_from_string - Basic extraction") {
    const char* yaml = R"delim(
palette:
  bg: "0x0000AA"
  fg: "0xFFFFFF"
  accent: "0xFFFF00"

button:
  normal:
    foreground: "$fg"
)delim";

    palette_map palette = extract_palette_from_string(yaml);

    CHECK(palette.size() == 3);
    CHECK(palette["bg"] == "0x0000AA");
    CHECK(palette["fg"] == "0xFFFFFF");
    CHECK(palette["accent"] == "0xFFFF00");
}

TEST_CASE("theme_palette::extract_palette_from_string - Empty palette") {
    const char* yaml = R"delim(
button:
  normal:
    foreground: "0xFFFFFF"
)delim";

    palette_map palette = extract_palette_from_string(yaml);

    CHECK(palette.empty());
}

TEST_CASE("theme_palette::extract_palette_from_string - No palette section") {
    const char* yaml = R"delim(
name: "Test Theme"
description: "No palette"
)delim";

    palette_map palette = extract_palette_from_string(yaml);

    CHECK(palette.empty());
}

TEST_CASE("theme_palette::extract_palette_from_string - Multiple colors") {
    const char* yaml = R"delim(
palette:
  bg: "0x0000AA"
  fg: "0xFFFFFF"
  accent: "0xFFFF00"
  hover_bg: "0x0000FF"
  disabled: "0x555555"
  border: "0xAAAAAA"
  error: "0xFF0000"
  success: "0x00FF00"
)delim";

    palette_map palette = extract_palette_from_string(yaml);

    CHECK(palette.size() == 8);
    CHECK(palette["bg"] == "0x0000AA");
    CHECK(palette["fg"] == "0xFFFFFF");
    CHECK(palette["accent"] == "0xFFFF00");
    CHECK(palette["hover_bg"] == "0x0000FF");
    CHECK(palette["disabled"] == "0x555555");
    CHECK(palette["border"] == "0xAAAAAA");
    CHECK(palette["error"] == "0xFF0000");
    CHECK(palette["success"] == "0x00FF00");
}

TEST_CASE("theme_palette::extract_palette_from_string - RGBA colors") {
    const char* yaml = R"delim(
palette:
  semi_transparent: "0xFF000080"
  fully_opaque: "0x00FF00FF"
)delim";

    palette_map palette = extract_palette_from_string(yaml);

    CHECK(palette.size() == 2);
    CHECK(palette["semi_transparent"] == "0xFF000080");
    CHECK(palette["fully_opaque"] == "0x00FF00FF");
}

TEST_CASE("theme_palette::extract_palette_from_string - Invalid hex color") {
    const char* yaml = R"delim(
palette:
  invalid: "not a hex color"
)delim";

    CHECK_THROWS_AS(static_cast<void>(extract_palette_from_string(yaml)), std::runtime_error);
}

TEST_CASE("theme_palette::extract_palette_from_string - Wrong length hex") {
    const char* yaml = R"delim(
palette:
  wrong_length: "0xFFF"
)delim";

    CHECK_THROWS_AS(static_cast<void>(extract_palette_from_string(yaml)), std::runtime_error);
}

// ===========================================================================
// resolve_references_in_string() - Reference Resolution Tests
// ===========================================================================

TEST_CASE("theme_palette::resolve_references_in_string - Single reference") {
    palette_map palette{{"fg", "0xFFFFFF"}};

    const char* yaml = R"delim(
button:
  foreground: "$fg"
)delim";

    std::string result = resolve_references_in_string(yaml, palette);

    CHECK(result.find("$fg") == std::string::npos);
    CHECK(result.find("0xFFFFFF") != std::string::npos);
}

TEST_CASE("theme_palette::resolve_references_in_string - Multiple references") {
    palette_map palette{
        {"bg", "0x0000AA"},
        {"fg", "0xFFFFFF"}
    };

    const char* yaml = R"delim(
button:
  normal:
    foreground: "$fg"
    background: "$bg"
)delim";

    std::string result = resolve_references_in_string(yaml, palette);

    CHECK(result.find("$fg") == std::string::npos);
    CHECK(result.find("$bg") == std::string::npos);
    CHECK(result.find("0xFFFFFF") != std::string::npos);
    CHECK(result.find("0x0000AA") != std::string::npos);
}

TEST_CASE("theme_palette::resolve_references_in_string - Nested references") {
    palette_map palette{
        {"bg", "0x0000AA"},
        {"fg", "0xFFFFFF"},
        {"accent", "0xFFFF00"}
    };

    const char* yaml = R"delim(
button:
  normal:
    foreground: "$fg"
    background: "$bg"
  hover:
    foreground: "$accent"
    background: "$bg"
)delim";

    std::string result = resolve_references_in_string(yaml, palette);

    CHECK(result.find("$fg") == std::string::npos);
    CHECK(result.find("$bg") == std::string::npos);
    CHECK(result.find("$accent") == std::string::npos);
    CHECK(result.find("0xFFFFFF") != std::string::npos);
    CHECK(result.find("0x0000AA") != std::string::npos);
    CHECK(result.find("0xFFFF00") != std::string::npos);
}

TEST_CASE("theme_palette::resolve_references_in_string - Undefined reference throws") {
    palette_map palette{{"fg", "0xFFFFFF"}};

    const char* yaml = R"delim(
button:
  foreground: "$undefined"
)delim";

    CHECK_THROWS_AS(static_cast<void>(resolve_references_in_string(yaml, palette)), std::runtime_error);
}

TEST_CASE("theme_palette::resolve_references_in_string - Mixed references and literals") {
    palette_map palette{
        {"bg", "0x0000AA"},
        {"fg", "0xFFFFFF"}
    };

    const char* yaml = R"delim(
button:
  normal:
    foreground: "$fg"
    background: "$bg"
  pressed:
    foreground: "0x000000"
    background: "0xAAAAAA"
)delim";

    std::string result = resolve_references_in_string(yaml, palette);

    // References resolved
    CHECK(result.find("$fg") == std::string::npos);
    CHECK(result.find("$bg") == std::string::npos);
    CHECK(result.find("0xFFFFFF") != std::string::npos);
    CHECK(result.find("0x0000AA") != std::string::npos);

    // Literals unchanged
    CHECK(result.find("0x000000") != std::string::npos);
    CHECK(result.find("0xAAAAAA") != std::string::npos);
}

TEST_CASE("theme_palette::resolve_references_in_string - Non-color strings unchanged") {
    palette_map palette{{"bg", "0x0000AA"}};

    const char* yaml = R"delim(
name: "Test Theme"
description: "A test theme"
border_style: "single_line"
)delim";

    std::string result = resolve_references_in_string(yaml, palette);

    // Non-reference strings unchanged
    CHECK(result.find("Test Theme") != std::string::npos);
    CHECK(result.find("A test theme") != std::string::npos);
    CHECK(result.find("single_line") != std::string::npos);
}

// ===========================================================================
// remove_palette_section() - Palette Removal Tests
// ===========================================================================

TEST_CASE("theme_palette::remove_palette_section - Removes palette") {
    const char* yaml = R"delim(
name: "Test Theme"

palette:
  bg: "0x0000AA"
  fg: "0xFFFFFF"

button:
  foreground: "0xFFFFFF"
)delim";

    std::string result = remove_palette_section(yaml);

    CHECK(result.find("palette:") == std::string::npos);
    CHECK(result.find("name: \"Test Theme\"") != std::string::npos);
    CHECK(result.find("button:") != std::string::npos);
}

TEST_CASE("theme_palette::remove_palette_section - No palette unchanged") {
    const char* yaml = R"delim(
name: "Test Theme"
button:
  foreground: "0xFFFFFF"
)delim";

    std::string result = remove_palette_section(yaml);

    CHECK(result == yaml);
}

// ===========================================================================
// apply_palette_preprocessing() - Full Pipeline Tests
// ===========================================================================

TEST_CASE("theme_palette::apply_palette_preprocessing - Complete example") {
    const char* yaml = R"delim(
name: "Test Theme"
description: "Palette test"

palette:
  bg: "0x0000AA"
  fg: "0xFFFFFF"
  accent: "0xFFFF00"

button:
  normal:
    foreground: "$fg"
    background: "$bg"
  hover:
    foreground: "$accent"
    background: "$bg"
)delim";

    std::string result = apply_palette_preprocessing(yaml);

    // Palette section removed
    CHECK(result.find("palette:") == std::string::npos);

    // References resolved
    CHECK(result.find("$fg") == std::string::npos);
    CHECK(result.find("$bg") == std::string::npos);
    CHECK(result.find("$accent") == std::string::npos);
    CHECK(result.find("0xFFFFFF") != std::string::npos);
    CHECK(result.find("0x0000AA") != std::string::npos);
    CHECK(result.find("0xFFFF00") != std::string::npos);

    // Other content preserved
    CHECK(result.find("name: \"Test Theme\"") != std::string::npos);
    CHECK(result.find("description: \"Palette test\"") != std::string::npos);
}

TEST_CASE("theme_palette::apply_palette_preprocessing - No palette passthrough") {
    const char* yaml = R"delim(
name: "Test Theme"
button:
  normal:
    foreground: "0xFFFFFF"
)delim";

    std::string result = apply_palette_preprocessing(yaml);

    // Should return unchanged when no palette
    CHECK(result == yaml);
}

TEST_CASE("theme_palette::apply_palette_preprocessing - Undefined reference error") {
    const char* yaml = R"delim(
palette:
  bg: "0x0000AA"

button:
  normal:
    foreground: "$fg"
)delim";

    CHECK_THROWS_AS(static_cast<void>(apply_palette_preprocessing(yaml)), std::runtime_error);
}

TEST_CASE("theme_palette::apply_palette_preprocessing - Invalid palette color error") {
    const char* yaml = R"delim(
palette:
  bg: "invalid"

button:
  normal:
    background: "$bg"
)delim";

    CHECK_THROWS_AS(static_cast<void>(apply_palette_preprocessing(yaml)), std::runtime_error);
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("theme_palette - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
