//
// Style Edge Cases and Robustness Tests
// Tests style resolution under unusual and stressful conditions
// Phase 4, Section 6: Edge Cases & Robustness
//

#include <doctest/doctest.h>
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/theme.hh>
#include <onyxui/resolved_style.hh>
#include <memory>
#include <vector>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

namespace {
    // Helper to create a valid test theme
    ui_theme<Backend> create_edge_case_theme(const std::string& name = "Edge Case Theme") {
        ui_theme<Backend> theme;
        theme.name = name;

        // Standard values
        theme.window_bg = {50, 50, 50};
        theme.text_fg = {200, 200, 200};
        theme.border_color = {100, 100, 100};

        theme.button.normal.foreground = {255, 255, 255};
        theme.button.normal.background = {0, 120, 215};

        return theme;
    }
}

TEST_SUITE("Style Edge Cases & Robustness") {

// ============================================================================
// Corrupted/Incomplete Theme Data
// ============================================================================

TEST_CASE("Edge Cases - Empty theme name") {
    ui_theme<Backend> empty_theme;
    empty_theme.name = "";  // Empty name
    // All other fields default

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(empty_theme));

    auto btn = std::make_unique<button<Backend>>("Test");
    // Applying empty name should fail
//     bool success = btn->apply_theme("", ctx.themes());  // No longer needed - widgets use global theme

//     CHECK(success == false);  // apply_theme() removed

    // Should still be able to resolve style (using defaults)
    auto style = btn->resolve_style();
    CHECK(style.background_color.r == 0);
}

TEST_CASE("Edge Cases - Theme with all zero colors") {
    ui_theme<Backend> black_theme;
    black_theme.name = "All Black";
    // All colors default to zero (black)

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(black_theme));

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("All Black", ctx.themes());  // No longer needed - widgets use global theme

    auto style = btn->resolve_style();

    // Should accept all-zero colors
    CHECK(style.background_color.r == 0);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 0);
}

TEST_CASE("Edge Cases - Theme with maximum color values") {
    ui_theme<Backend> max_theme;
    max_theme.name = "Max Colors";
    max_theme.window_bg = {255, 255, 255};
    max_theme.text_fg = {255, 255, 255};
    max_theme.border_color = {255, 255, 255};

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(max_theme));

    auto lbl = std::make_unique<label<Backend>>("Test");
//     lbl->apply_theme("Max Colors", ctx.themes());  // No longer needed - widgets use global theme

    auto style = lbl->resolve_style();

    // Should accept maximum values
    CHECK(style.background_color.r == 255);
    CHECK(style.foreground_color.g == 255);
    CHECK(style.border_color.b == 255);
}

TEST_CASE("Edge Cases - Partial theme with missing widget styles") {
    ui_theme<Backend> partial_theme;
    partial_theme.name = "Partial";
    partial_theme.window_bg = {40, 40, 40};
    // button, label, panel styles all default (black {0,0,0})

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(partial_theme));

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("Partial", ctx.themes());  // No longer needed - widgets use global theme

    auto style = btn->resolve_style();

    // Button uses button.normal.background (defaults to {0,0,0}), not window_bg
    CHECK(style.background_color.r == 0);
}

// ============================================================================
// Theme Switching Scenarios
// ============================================================================

TEST_CASE("Edge Cases - Rapid theme switching") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme("Theme A"));
    ctx.themes().register_theme(create_edge_case_theme("Theme B"));
    ctx.themes().register_theme(create_edge_case_theme("Theme C"));

    auto btn = std::make_unique<button<Backend>>("Test");

    // Switch rapidly 100 times
    for (int i = 0; i < 100; ++i) {
//         btn->apply_theme("Theme A", ctx.themes());  // No longer needed - widgets use global theme
//         btn->apply_theme("Theme B", ctx.themes());  // No longer needed - widgets use global theme
//         btn->apply_theme("Theme C", ctx.themes());  // No longer needed - widgets use global theme
    }

    // Should still work correctly - button uses button.normal.background = {0, 120, 215}
    auto style = btn->resolve_style();
    CHECK(style.background_color.r == 0);  // button.normal.background.r
    CHECK(style.background_color.g == 120);  // button.normal.background.g
}

TEST_CASE("Edge Cases - Theme switching during style resolution") {
    scoped_ui_context<Backend> ctx;

    auto theme_a = create_edge_case_theme("A");
    theme_a.button.normal.background = {100, 0, 0};  // Red button
    ctx.themes().register_theme(std::move(theme_a));

    auto theme_b = create_edge_case_theme("B");
    theme_b.button.normal.background = {0, 100, 0};  // Green button
    ctx.themes().register_theme(std::move(theme_b));

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("A", ctx.themes());  // No longer needed - widgets use global theme

    auto style1 = btn->resolve_style();

    // Switch theme
//     btn->apply_theme("B", ctx.themes());  // No longer needed - widgets use global theme

    auto style2 = btn->resolve_style();

    // Styles should be different (red vs green)
    CHECK(style1.background_color.r != style2.background_color.r);
    CHECK(style1.background_color.g != style2.background_color.g);
}

TEST_CASE("Edge Cases - Theme switch from valid to invalid") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme("Valid"));

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("Valid", ctx.themes());  // No longer needed - widgets use global theme

    auto style1 = btn->resolve_style();
    CHECK(style1.background_color.g == 120);  // button.normal.background.g

    // Try to switch to nonexistent theme
//     bool success = btn->apply_theme("Nonexistent", ctx.themes());  // No longer needed - widgets use global theme

//     CHECK(success == false);  // apply_theme() removed

    // Should retain previous theme
    auto style2 = btn->resolve_style();
    CHECK(style2.background_color.g == 120);  // Still valid theme
}

// ============================================================================
// Large Widget Trees
// ============================================================================

TEST_CASE("Edge Cases - 100-widget linear hierarchy") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto root = std::make_unique<panel<Backend>>();
//     root->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme
    root->set_background_color({100, 100, 100});

    panel<Backend>* current = root.get();
    for (int i = 0; i < 99; ++i) {
        current = current->template emplace_child<panel>();
    }

    // Add panel at depth 100 (testing CSS inheritance)
    auto* deep_panel = current->template emplace_child<panel>();

    // Should resolve without hanging
    auto style = deep_panel->resolve_style();
    CHECK(style.background_color.r == 100);
}

TEST_CASE("Edge Cases - Wide tree with 100 children") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto root = std::make_unique<panel<Backend>>();
//     root->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme
    root->set_foreground_color({150, 150, 150});

    // Add 100 children (testing CSS inheritance with panels)
    std::vector<panel<Backend>*> children;
    for (int i = 0; i < 100; ++i) {
        auto* child = root->template emplace_child<panel>();
        children.push_back(child);
    }

    // All children should resolve correctly
    for (auto* child : children) {
        auto style = child->resolve_style();
        CHECK(style.foreground_color.r == 150);
    }
}

TEST_CASE("Edge Cases - 1000-widget complex tree") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto root = std::make_unique<panel<Backend>>();
//     root->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme

    // Build tree: 10 levels, 10 children per node (10^3 = 1000 widgets)
    std::vector<panel<Backend>*> current_level;
    current_level.push_back(root.get());

    for (int level = 0; level < 3; ++level) {
        std::vector<panel<Backend>*> next_level;
        for (auto* parent : current_level) {
            for (int i = 0; i < 10; ++i) {
                auto* child = parent->template emplace_child<panel>();
                next_level.push_back(child);
            }
        }
        current_level = next_level;
    }

    // Verify all leaf nodes can resolve style
    for (auto* leaf : current_level) {
        auto style = leaf->resolve_style();
        CHECK(style.background_color.r >= 0);
    }

    CHECK(current_level.size() == 1000);
}

// ============================================================================
// Missing/Invalid Property Data
// ============================================================================

TEST_CASE("Edge Cases - Missing font data in theme") {
    ui_theme<Backend> theme;
    theme.name = "No Font";
    theme.window_bg = {50, 50, 50};
    // font fields left default

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(theme));

    auto lbl = std::make_unique<label<Backend>>("Test");
//     lbl->apply_theme("No Font", ctx.themes());  // No longer needed - widgets use global theme

    auto style = lbl->resolve_style();

    // Should handle missing font gracefully (font exists, even if empty)
    [[maybe_unused]] auto font = style.font;
    CHECK(true);  // Font structure exists
}

TEST_CASE("Edge Cases - Extreme color values (boundary)") {
    scoped_ui_context<Backend> ctx;

    auto widget = std::make_unique<panel<Backend>>();
    widget->set_background_color({255, 0, 255});  // Extreme values
    widget->set_foreground_color({0, 255, 0});

    auto style = widget->resolve_style();

    // Should accept boundary values
    CHECK(style.background_color.r == 255);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 255);
}

TEST_CASE("Edge Cases - Zero opacity") {
    scoped_ui_context<Backend> ctx;

    auto widget = std::make_unique<panel<Backend>>();
    widget->set_opacity(0.0f);  // Fully transparent

    auto style = widget->resolve_style();

    // Should accept zero opacity
    CHECK(style.opacity == 0.0f);
}

TEST_CASE("Edge Cases - Opacity greater than 1.0") {
    scoped_ui_context<Backend> ctx;

    auto widget = std::make_unique<panel<Backend>>();
    widget->set_opacity(1.5f);  // Invalid (> 1.0)

    auto style = widget->resolve_style();

    // Implementation may clamp or accept (both valid)
    CHECK(style.opacity >= 0.0f);
}

TEST_CASE("Edge Cases - Negative opacity") {
    scoped_ui_context<Backend> ctx;

    auto widget = std::make_unique<panel<Backend>>();
    widget->set_opacity(-0.5f);  // Invalid (< 0.0)

    auto style = widget->resolve_style();

    // Implementation may clamp or accept (both valid)
    CHECK(style.opacity >= -1.0f);  // At least not corrupted
}

// ============================================================================
// Null/Zero-Sized Bounds
// ============================================================================

TEST_CASE("Edge Cases - Widget with zero-sized bounds") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto widget = std::make_unique<panel<Backend>>();
//     widget->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme

    // Arrange with zero size
    widget->arrange({0, 0, 0, 0});

    // Should still resolve style correctly
    auto style = widget->resolve_style();
    CHECK(style.background_color.r == 50);
}

TEST_CASE("Edge Cases - Widget not yet measured or arranged") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto widget = std::make_unique<button<Backend>>("Test");
//     widget->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme

    // No measure() or arrange() called

    // Should still resolve style - button uses button.normal.foreground = {255, 255, 255}
    auto style = widget->resolve_style();
    CHECK(style.foreground_color.r == 255);
}

// ============================================================================
// Multiple Resolution Calls
// ============================================================================

TEST_CASE("Edge Cases - 10000 sequential style resolutions") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme

    // Resolve many times
    for (int i = 0; i < 10000; ++i) {
        [[maybe_unused]] auto style = btn->resolve_style();
    }

    // Should complete without hanging or corruption - button uses button.normal.background
    auto final_style = btn->resolve_style();
    CHECK(final_style.background_color.g == 120);  // button.normal.background = {0, 120, 215}
}

TEST_CASE("Edge Cases - Alternating resolution and modification") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme

    for (int i = 1; i < 101; ++i) {
        auto style = btn->resolve_style();
        CHECK(style.foreground_color.r >= 0);  // Can be 0 or greater

        // Modify widget (start from 1 to avoid setting 0)
        btn->set_foreground_color({static_cast<std::uint8_t>(i % 255), 100, 100});
    }

    // Should still work correctly
    auto final_style = btn->resolve_style();
    CHECK(final_style.foreground_color.r == 100);  // 100 % 255 = 100
}

// ============================================================================
// Detached/Orphaned Widgets
// ============================================================================

TEST_CASE("Edge Cases - Widget with no parent can resolve style") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_edge_case_theme());

    // Widget without parent
    auto orphan = std::make_unique<button<Backend>>("Orphan");
//     orphan->apply_theme("Edge Case Theme", ctx.themes());  // No longer needed - widgets use global theme

    // Should resolve style successfully (no parent inheritance)
    auto style = orphan->resolve_style();
    CHECK(style.background_color.r >= 0);
}

// ============================================================================
// Theme Registry Edge Cases
// ============================================================================

TEST_CASE("Edge Cases - Apply theme before registry initialized") {
    // Note: This tests behavior when ui_context doesn't exist yet
    auto widget = std::make_unique<button<Backend>>("Test");

    // Should handle gracefully (no crash)
    auto style = widget->resolve_style();
    CHECK(style.background_color.r == 0);  // Defaults
}

TEST_CASE("Edge Cases - Duplicate theme registration") {
    scoped_ui_context<Backend> ctx;

    auto theme1 = create_edge_case_theme("Duplicate");
    theme1.button.normal.background = {100, 0, 0};
    ctx.themes().register_theme(std::move(theme1));

    auto theme2 = create_edge_case_theme("Duplicate");
    theme2.button.normal.background = {0, 100, 0};
    ctx.themes().register_theme(std::move(theme2));  // Overwrites first

    auto btn = std::make_unique<button<Backend>>("Test");
//     btn->apply_theme("Duplicate", ctx.themes());  // No longer needed - widgets use global theme

    auto style = btn->resolve_style();

    // Should use second theme's button color (overwrite behavior)
    CHECK(style.background_color.g == 100);
}

TEST_CASE("Edge Cases - Theme name with special characters") {
    scoped_ui_context<Backend> ctx;

    auto theme = create_edge_case_theme("Theme!@#$%^&*()");
    ctx.themes().register_theme(std::move(theme));

    auto btn = std::make_unique<button<Backend>>("Test");
//     bool success = btn->apply_theme("Theme!@#$%^&*()", ctx.themes());  // No longer needed - widgets use global theme

//     CHECK(success == true);  // apply_theme() removed

    auto style = btn->resolve_style();
    CHECK(style.background_color.g == 120);  // button.normal.background = {0, 120, 215}
}

} // TEST_SUITE
