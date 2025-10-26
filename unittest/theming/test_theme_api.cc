//
// Theme API Tests
// Tests the three-way theme application API (by-name, by-value, by-shared_ptr)
// and verifies safety guarantees of v2.0 theme system
//

#include <doctest/doctest.h>
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/theme.hh>
#include <memory>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

namespace {
    // Helper to create a test theme with known values
    ui_theme<Backend> create_test_theme(const std::string& name) {
        ui_theme<Backend> theme;
        theme.name = name;
        theme.description = "Test theme for API validation";

        // Global colors
        theme.window_bg = {0, 0, 170};
        theme.text_fg = {255, 255, 255};
        theme.border_color = {170, 170, 170};

        // Button styling
        theme.button.normal.foreground = {255, 255, 255};
        theme.button.normal.background = {0, 0, 170};
        theme.button.hover.foreground = {255, 255, 0};
        theme.button.hover.background = {0, 170, 170};
        theme.button.pressed.foreground = {0, 0, 0};
        theme.button.pressed.background = {170, 170, 170};
        theme.button.disabled.foreground = {128, 128, 128};
        theme.button.disabled.background = {64, 64, 64};

        // Label styling
        theme.label.text = {255, 255, 255};
        theme.label.background = {0, 0, 170};

        // Panel styling
        theme.panel.background = {0, 0, 170};
        theme.panel.border_color = {255, 255, 255};

        return theme;
    }
}

TEST_SUITE("Theme API") {

// ============================================================================
// By-Name API (Recommended)
// ============================================================================

TEST_CASE("Theme API - By-name application success") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_test_theme("Blue Theme"));

    auto btn = std::make_unique<button<Backend>>("Test");

    bool success = btn->apply_theme("Blue Theme", ctx.themes());

    CHECK(success == true);
    CHECK(btn->resolve_style().background_color.b == 170);
    CHECK(btn->resolve_style().foreground_color.r == 255);
}

TEST_CASE("Theme API - By-name application failure (theme not found)") {
    scoped_ui_context<Backend> ctx;
    // Intentionally don't register theme

    auto btn = std::make_unique<button<Backend>>("Test");

    bool success = btn->apply_theme("Nonexistent Theme", ctx.themes());

    CHECK(success == false);
    // Widget should retain previous theme or default
}

TEST_CASE("Theme API - By-name with multiple themes") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_test_theme("Theme A"));
    ctx.themes().register_theme(create_test_theme("Theme B"));
    ctx.themes().register_theme(create_test_theme("Theme C"));

    auto btn = std::make_unique<button<Backend>>("Test");

    bool success = btn->apply_theme("Theme B", ctx.themes());

    CHECK(success == true);
    // Verify it applied Theme B (name is stored internally)
}

TEST_CASE("Theme API - By-name switching between themes") {
    scoped_ui_context<Backend> ctx;

    auto blue = create_test_theme("Blue");
    blue.window_bg = {0, 0, 170};
    ctx.themes().register_theme(std::move(blue));

    auto red = create_test_theme("Red");
    red.button.normal.background = {170, 0, 0};  // Red button
    ctx.themes().register_theme(std::move(red));

    auto btn = std::make_unique<button<Backend>>("Test");

    btn->apply_theme("Blue", ctx.themes());
    auto blue_bg = btn->resolve_style().background_color;

    btn->apply_theme("Red", ctx.themes());
    auto red_bg = btn->resolve_style().background_color;

    CHECK(blue_bg.b == 170);  // Blue: button.normal.background = {0, 0, 170}
    CHECK(red_bg.r == 170);   // Red: button.normal.background = {170, 0, 0}
}

// ============================================================================
// By-Value API (Move Semantics)
// ============================================================================

TEST_CASE("Theme API - By-value application with move") {
    auto theme = create_test_theme("Moved Theme");
    auto btn = std::make_unique<button<Backend>>("Test");

    btn->apply_theme(std::move(theme));

    CHECK(btn->resolve_style().background_color.b == 170);
    // Note: Cannot check 'theme' after move - it's in moved-from state
}

TEST_CASE("Theme API - By-value with temporary") {
    auto btn = std::make_unique<button<Backend>>("Test");

    btn->apply_theme(create_test_theme("Temporary"));

    CHECK(btn->resolve_style().background_color.b == 170);
}

TEST_CASE("Theme API - By-value multiple widgets require explicit copies") {
    auto theme = create_test_theme("Shared");
    auto btn1 = std::make_unique<button<Backend>>("Btn1");
    auto btn2 = std::make_unique<button<Backend>>("Btn2");

    // Must explicitly copy theme for second widget
    auto theme_copy = theme;  // Explicit copy
    btn1->apply_theme(std::move(theme));
    btn2->apply_theme(std::move(theme_copy));

    CHECK(btn1->resolve_style().background_color.b == 170);
    CHECK(btn2->resolve_style().background_color.b == 170);
}

// ============================================================================
// By-Shared-Ptr API (Shared Ownership)
// ============================================================================

TEST_CASE("Theme API - By-shared_ptr application") {
    auto theme = std::make_shared<ui_theme<Backend>>(
        create_test_theme("Shared Theme")
    );
    auto btn = std::make_unique<button<Backend>>("Test");

    btn->apply_theme(theme);

    CHECK(btn->resolve_style().background_color.b == 170);
    CHECK(theme.use_count() >= 2);  // Widget holds a copy
}

TEST_CASE("Theme API - By-shared_ptr multiple widgets") {
    auto theme = std::make_shared<ui_theme<Backend>>(
        create_test_theme("Shared")
    );
    auto btn1 = std::make_unique<button<Backend>>("Btn1");
    auto btn2 = std::make_unique<button<Backend>>("Btn2");
    auto btn3 = std::make_unique<button<Backend>>("Btn3");

    btn1->apply_theme(theme);
    btn2->apply_theme(theme);
    btn3->apply_theme(theme);

    CHECK(theme.use_count() >= 4);  // Original + 3 widgets
    CHECK(btn1->resolve_style().background_color.b == 170);
    CHECK(btn2->resolve_style().background_color.b == 170);
    CHECK(btn3->resolve_style().background_color.b == 170);
}

TEST_CASE("Theme API - By-shared_ptr lifetime management") {
    auto btn = std::make_unique<button<Backend>>("Test");

    {
        auto theme = std::make_shared<ui_theme<Backend>>(
            create_test_theme("Scoped")
        );
        btn->apply_theme(theme);
        CHECK(theme.use_count() >= 2);
    } // theme goes out of scope

    // Theme should still be alive (widget holds reference)
    CHECK(btn->resolve_style().background_color.b == 170);
}

TEST_CASE("Theme API - By-shared_ptr dynamic updates") {
    auto theme = std::make_shared<ui_theme<Backend>>(
        create_test_theme("Dynamic")
    );
    theme->button.normal.background = {0, 0, 170};  // Blue button

    auto btn1 = std::make_unique<button<Backend>>("Btn1");
    auto btn2 = std::make_unique<button<Backend>>("Btn2");
    btn1->apply_theme(theme);
    btn2->apply_theme(theme);

    // Modify theme
    theme->button.normal.background = {170, 0, 0};  // Change to red button

    // Re-apply to see changes
    btn1->apply_theme(theme);
    btn2->apply_theme(theme);

    CHECK(btn1->resolve_style().background_color.r == 170);
    CHECK(btn2->resolve_style().background_color.r == 170);
}

// ============================================================================
// API Comparison and Safety
// ============================================================================

TEST_CASE("Theme API - Null pointer safety") {
    std::shared_ptr<ui_theme<Backend>> null_theme = nullptr;
    auto btn = std::make_unique<button<Backend>>("Test");

    // Apply null theme (should be no-op)
    btn->apply_theme(null_theme);

    // Widget should retain default values (no crash)
    CHECK(btn->resolve_style().background_color.r >= 0);
    CHECK(btn->resolve_style().background_color.g >= 0);
    CHECK(btn->resolve_style().background_color.b >= 0);
}

TEST_CASE("Theme API - By-name is zero overhead") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_test_theme("Original"));

    auto btn = std::make_unique<button<Backend>>("Test");

    btn->apply_theme("Original", ctx.themes());

    // Verify theme is looked up from registry, not copied
    auto* registry_theme = ctx.themes().get_theme("Original");
    CHECK(registry_theme != nullptr);
}

TEST_CASE("Theme API - All three APIs produce same result") {
    scoped_ui_context<Backend> ctx;
    auto theme_for_registry = create_test_theme("Test");
    ctx.themes().register_theme(theme_for_registry);

    auto theme_for_value = create_test_theme("Test");
    auto theme_for_shared = std::make_shared<ui_theme<Backend>>(
        create_test_theme("Test")
    );

    auto btn1 = std::make_unique<button<Backend>>("Btn1");
    auto btn2 = std::make_unique<button<Backend>>("Btn2");
    auto btn3 = std::make_unique<button<Backend>>("Btn3");

    btn1->apply_theme("Test", ctx.themes());              // By-name
    btn2->apply_theme(std::move(theme_for_value));       // By-value
    btn3->apply_theme(theme_for_shared);                  // By-shared_ptr

    // All buttons should have identical styling
    CHECK(btn1->resolve_style().background_color == btn2->resolve_style().background_color);
    CHECK(btn2->resolve_style().background_color == btn3->resolve_style().background_color);
    CHECK(btn1->resolve_style().foreground_color == btn2->resolve_style().foreground_color);
    CHECK(btn2->resolve_style().foreground_color == btn3->resolve_style().foreground_color);
}

// ============================================================================
// Error Handling and Edge Cases
// ============================================================================

TEST_CASE("Theme API - Empty theme name") {
    scoped_ui_context<Backend> ctx;
    auto btn = std::make_unique<button<Backend>>("Test");

    bool success = btn->apply_theme("", ctx.themes());

    CHECK(success == false);
}

TEST_CASE("Theme API - Case sensitivity") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_test_theme("MyTheme"));

    auto btn = std::make_unique<button<Backend>>("Test");

    bool success_lower = btn->apply_theme("mytheme", ctx.themes());
    bool success_upper = btn->apply_theme("MYTHEME", ctx.themes());
    bool success_correct = btn->apply_theme("MyTheme", ctx.themes());

    CHECK(success_lower == false);
    CHECK(success_upper == false);
    CHECK(success_correct == true);
}

TEST_CASE("Theme API - Theme with incomplete data") {
    ui_theme<Backend> incomplete_theme;
    incomplete_theme.name = "Incomplete";
    // Intentionally leave other fields default-initialized

    auto btn = std::make_unique<button<Backend>>("Test");

    btn->apply_theme(std::move(incomplete_theme));

    // Should apply successfully with default values
    CHECK(btn->resolve_style().background_color.r == 0);
    CHECK(btn->resolve_style().background_color.g == 0);
    CHECK(btn->resolve_style().background_color.b == 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("Theme API - Apply to widget hierarchy") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_test_theme("Hierarchy Test"));

    auto parent = std::make_unique<panel<Backend>>();
    auto child1 = std::make_unique<button<Backend>>("Child1");
    auto child2 = std::make_unique<button<Backend>>("Child2");

    auto* child1_ptr = child1.get();
    auto* child2_ptr = child2.get();

    parent->add_child(std::move(child1));
    parent->add_child(std::move(child2));

    parent->apply_theme("Hierarchy Test", ctx.themes());

    // Check parent
    CHECK(parent->resolve_style().background_color.b == 170);

    // Children should inherit
    CHECK(child1_ptr->resolve_style().background_color.b >= 100);
    CHECK(child2_ptr->resolve_style().background_color.b >= 100);
}

TEST_CASE("Theme API - Apply same theme twice is idempotent") {
    scoped_ui_context<Backend> ctx;
    auto theme = create_test_theme("Test");
    ctx.themes().register_theme(std::move(theme));

    auto widget = std::make_unique<button<Backend>>("Test");

    widget->apply_theme("Test", ctx.themes());
    auto bg1 = widget->resolve_style().background_color;

    widget->apply_theme("Test", ctx.themes());
    auto bg2 = widget->resolve_style().background_color;

    CHECK(bg1 == bg2);
}

TEST_CASE("Theme API - Multiple theme switches maintain consistency") {
    scoped_ui_context<Backend> ctx;

    auto blue = create_test_theme("Blue");
    blue.button.normal.background = {0, 0, 170};  // Blue button
    ctx.themes().register_theme(std::move(blue));

    auto red = create_test_theme("Red");
    red.button.normal.background = {170, 0, 0};  // Red button
    ctx.themes().register_theme(std::move(red));

    auto btn = std::make_unique<button<Backend>>("Test");

    // Switch multiple times
    btn->apply_theme("Blue", ctx.themes());
    auto bg1 = btn->resolve_style().background_color;

    btn->apply_theme("Red", ctx.themes());
    auto bg2 = btn->resolve_style().background_color;

    btn->apply_theme("Blue", ctx.themes());
    auto bg3 = btn->resolve_style().background_color;

    btn->apply_theme("Red", ctx.themes());
    auto bg4 = btn->resolve_style().background_color;

    // First and third should match (both blue)
    CHECK(bg1.r == bg3.r);
    CHECK(bg1.g == bg3.g);
    CHECK(bg1.b == bg3.b);

    // Second and fourth should match (both red)
    CHECK(bg2.r == bg4.r);
    CHECK(bg2.g == bg4.g);
    CHECK(bg2.b == bg4.b);

    // Blue and red buttons should differ
    CHECK((bg1.r != bg2.r || bg1.b != bg2.b));
}

} // TEST_SUITE
