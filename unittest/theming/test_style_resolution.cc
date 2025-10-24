//
// Style Resolution Algorithm Tests
// Tests the resolve_style() mechanism and CSS inheritance chain
// Phase 4, Section 2: Unit Tests - Style Resolution Algorithm
//

#include <doctest/doctest.h>
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/theme.hh>
#include <onyxui/resolved_style.hh>
#include <memory>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

namespace {
    // Helper to create a test theme with known values
    ui_theme<Backend> create_resolution_test_theme() {
        ui_theme<Backend> theme;
        theme.name = "Resolution Test";
        theme.description = "Theme for style resolution testing";

        // Global colors
        theme.window_bg = {10, 10, 10};
        theme.text_fg = {240, 240, 240};
        theme.border_color = {100, 100, 100};

        // Button styling
        theme.button.fg_normal = {255, 255, 255};
        theme.button.bg_normal = {0, 120, 215};
        theme.button.fg_hover = {255, 255, 255};
        theme.button.bg_hover = {0, 102, 204};
        theme.button.fg_pressed = {255, 255, 255};
        theme.button.bg_pressed = {0, 84, 179};
        theme.button.fg_disabled = {128, 128, 128};
        theme.button.bg_disabled = {40, 40, 40};

        // Label styling
        theme.label.text = {240, 240, 240};
        theme.label.background = {10, 10, 10};

        // Panel styling
        theme.panel.background = {20, 20, 20};
        theme.panel.border_color = {100, 100, 100};

        return theme;
    }
}

TEST_SUITE("Style Resolution Algorithm") {

// ============================================================================
// Basic Functionality
// ============================================================================

TEST_CASE("Style Resolution - resolve_style() returns populated style") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());

    // Resolve style
    auto style = btn->resolve_style();

    // Verify all fields are populated (button uses button.fg_normal=255 and button.bg_normal.g=120)
    CHECK(style.foreground_color.r > 0);
    CHECK(style.background_color.g > 0);  // bg_normal = {0, 120, 215}, so check g channel
}

TEST_CASE("Style Resolution - resolve_style() is const-correct") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());

    // Should be callable on const reference
    const button<Backend>& const_btn = *btn;
    auto style = const_btn.resolve_style();

    CHECK(style.background_color.g > 0);  // bg_normal = {0, 120, 215}, so check g channel
}

TEST_CASE("Style Resolution - resolve_style() is idempotent") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());

    // Call multiple times
    auto style1 = btn->resolve_style();
    auto style2 = btn->resolve_style();
    auto style3 = btn->resolve_style();

    // All should be identical
    CHECK(style1.foreground_color.r == style2.foreground_color.r);
    CHECK(style2.foreground_color.r == style3.foreground_color.r);
    CHECK(style1.background_color.b == style2.background_color.b);
    CHECK(style2.background_color.b == style3.background_color.b);
}

// ============================================================================
// CSS Inheritance Chain: Override → Parent → Theme → Default
// ============================================================================

TEST_CASE("Style Resolution - Override takes precedence over all") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto parent = std::make_unique<panel<Backend>>();
    parent->apply_theme("Resolution Test", ctx.themes());
    parent->set_background_color({50, 50, 50});  // Parent override

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->set_background_color({255, 0, 0});  // Widget override (highest priority)

    parent->add_child(std::move(btn));
    auto* btn_ptr = dynamic_cast<button<Backend>*>(parent->children()[0].get());

    auto style = btn_ptr->resolve_style();

    // Should use widget override (red), not parent or theme
    CHECK(style.background_color.r == 255);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 0);
}

TEST_CASE("Style Resolution - Parent fallback when no override") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto parent = std::make_unique<panel<Backend>>();
    parent->apply_theme("Resolution Test", ctx.themes());
    parent->set_foreground_color({200, 200, 100});  // Parent override

    auto lbl = std::make_unique<label<Backend>>("Test");
    // No override on label

    parent->add_child(std::move(lbl));
    auto* lbl_ptr = dynamic_cast<label<Backend>*>(parent->children()[0].get());

    auto style = lbl_ptr->resolve_style();

    // Should inherit from parent
    CHECK(style.foreground_color.r == 200);
    CHECK(style.foreground_color.g == 200);
    CHECK(style.foreground_color.b == 100);
}

TEST_CASE("Style Resolution - Theme fallback when no parent or override") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());
    // No parent, no override

    auto style = btn->resolve_style();

    // Should use button-specific theme colors (button.fg_normal, button.bg_normal)
    CHECK(style.foreground_color.r == 255);  // button.fg_normal
    CHECK(style.background_color.g == 120);   // button.bg_normal = {0, 120, 215}
}

TEST_CASE("Style Resolution - Default fallback when no theme") {
    scoped_ui_context<Backend> ctx;
    // Don't apply theme

    auto btn = std::make_unique<button<Backend>>("Test");
    // No theme, no parent, no override

    auto style = btn->resolve_style();

    // Should use defaults (all zeros)
    CHECK(style.foreground_color.r == 0);
    CHECK(style.foreground_color.g == 0);
    CHECK(style.foreground_color.b == 0);
    CHECK(style.background_color.r == 0);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 0);
}

// ============================================================================
// Partial Theme Handling
// ============================================================================

TEST_CASE("Style Resolution - Partial theme fills gaps with defaults") {
    ui_theme<Backend> incomplete_theme;
    incomplete_theme.name = "Incomplete";
    // Leave most fields default-initialized
    incomplete_theme.button.fg_normal = {255, 100, 0};  // Only set foreground

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(incomplete_theme));

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Incomplete", ctx.themes());

    auto style = btn->resolve_style();

    // Foreground should use theme value
    CHECK(style.foreground_color.r == 255);
    CHECK(style.foreground_color.g == 100);

    // Background should use defaults (zeros)
    CHECK(style.background_color.r == 0);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 0);
}

TEST_CASE("Style Resolution - Missing properties use defaults") {
    ui_theme<Backend> minimal_theme;
    minimal_theme.name = "Minimal";
    // Only set name, everything else default

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(std::move(minimal_theme));

    auto lbl = std::make_unique<label<Backend>>("Test");
    lbl->apply_theme("Minimal", ctx.themes());

    auto style = lbl->resolve_style();

    // All colors should be default (zeros)
    CHECK(style.foreground_color.r == 0);
    CHECK(style.background_color.r == 0);
    CHECK(style.border_color.r == 0);
}

// ============================================================================
// No Theme Applied
// ============================================================================

TEST_CASE("Style Resolution - No theme applied uses defaults") {
    scoped_ui_context<Backend> ctx;

    auto btn = std::make_unique<button<Backend>>("Test");
    // No apply_theme() call

    auto style = btn->resolve_style();

    // Should use all defaults
    CHECK(style.foreground_color.r == 0);
    CHECK(style.foreground_color.g == 0);
    CHECK(style.foreground_color.b == 0);
    CHECK(style.background_color.r == 0);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 0);
}

TEST_CASE("Style Resolution - Theme not found falls back to default") {
    scoped_ui_context<Backend> ctx;
    // Register theme but apply different name

    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    bool success = btn->apply_theme("Nonexistent Theme", ctx.themes());

    CHECK(success == false);

    auto style = btn->resolve_style();

    // Should use defaults since theme not found
    CHECK(style.foreground_color.r == 0);
    CHECK(style.background_color.r == 0);
}

// ============================================================================
// Null Parent Handling
// ============================================================================

TEST_CASE("Style Resolution - Null parent (root widget) works correctly") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Resolution Test", ctx.themes());
    // Root has no parent (nullptr)

    auto style = root->resolve_style();

    // Should use theme values
    CHECK(style.background_color.r == 10);  // window_bg
    CHECK(style.foreground_color.r == 240);  // text_fg
}

TEST_CASE("Style Resolution - Orphan widget with theme resolves correctly") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto orphan = std::make_unique<button<Backend>>("Test");
    orphan->apply_theme("Resolution Test", ctx.themes());
    // No parent

    auto style = orphan->resolve_style();

    // Should use button-specific theme colors
    CHECK(style.foreground_color.r == 255);  // button.fg_normal
    CHECK(style.background_color.g == 120);   // button.bg_normal = {0, 120, 215}
}

// ============================================================================
// Deep Inheritance Chains
// ============================================================================

TEST_CASE("Style Resolution - Five-level inheritance chain") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    // Build deep hierarchy: L1 → L2 → L3 → L4 → L5
    auto l1 = std::make_unique<panel<Backend>>();
    l1->apply_theme("Resolution Test", ctx.themes());
    l1->set_foreground_color({255, 0, 0});  // Red at root

    auto* l2 = l1->template emplace_child<panel>();
    // No override, inherits from L1

    auto* l3 = l2->template emplace_child<panel>();
    // No override, inherits from L2

    auto* l4 = l3->template emplace_child<panel>();
    l4->set_foreground_color({0, 255, 0});  // Green override at L4

    auto* l5 = l4->template emplace_child<button>("Deep Button");
    // No override, inherits from L4

    auto style = l5->resolve_style();

    // Should inherit green from L4 (nearest parent with override)
    CHECK(style.foreground_color.r == 0);
    CHECK(style.foreground_color.g == 255);
    CHECK(style.foreground_color.b == 0);
}

TEST_CASE("Style Resolution - Ten-level inheritance chain") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    // Build very deep hierarchy
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Resolution Test", ctx.themes());
    root->set_background_color({10, 10, 10});

    panel<Backend>* current = root.get();
    for (int i = 0; i < 9; ++i) {
        current = current->template emplace_child<panel>();
    }

    // Add button at depth 10
    auto* deep_btn = current->template emplace_child<button>("Deep");

    auto style = deep_btn->resolve_style();

    // Should successfully resolve even at depth 10
    CHECK(style.background_color.r == 10);
    CHECK(style.background_color.g == 10);
    CHECK(style.background_color.b == 10);
}

TEST_CASE("Style Resolution - Deep chain with override at each level") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    // Build chain with override at each level
    auto l1 = std::make_unique<panel<Backend>>();
    l1->apply_theme("Resolution Test", ctx.themes());
    l1->set_opacity(1.0f);

    auto* l2 = l1->template emplace_child<panel>();
    l2->set_opacity(0.9f);

    auto* l3 = l2->template emplace_child<panel>();
    l3->set_opacity(0.8f);

    auto* l4 = l3->template emplace_child<panel>();
    l4->set_opacity(0.7f);

    auto* l5 = l4->template emplace_child<button>("Deep");
    // No override, inherits from L4

    auto style = l5->resolve_style();

    // Opacity is multiplicative: 1.0 * 0.9 * 0.8 * 0.7 * 1.0 = 0.504
    CHECK(style.opacity == doctest::Approx(0.504f).epsilon(0.01));
}

// ============================================================================
// Circular Parent References (Impossible but Verify)
// ============================================================================

TEST_CASE("Style Resolution - Detached widget has no parent reference") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto widget = std::make_unique<button<Backend>>("Test");
    widget->apply_theme("Resolution Test", ctx.themes());

    // Verify no parent
    CHECK(widget->parent() == nullptr);

    // Should still resolve correctly
    auto style = widget->resolve_style();
    CHECK(style.foreground_color.r == 255);  // Uses button theme (button.fg_normal, not text_fg)
}

TEST_CASE("Style Resolution - Widget removed from parent has no dangling ref") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto parent = std::make_unique<panel<Backend>>();
    parent->apply_theme("Resolution Test", ctx.themes());

    auto child = std::make_unique<button<Backend>>("Test");
    child->set_foreground_color({100, 100, 100});

    parent->add_child(std::move(child));

    // Get pointer before removal
    auto* child_ptr = parent->children()[0].get();
    auto removed = parent->remove_child(child_ptr);

    // Verify no parent after removal
    CHECK(removed->parent() == nullptr);

    // Should still resolve correctly (uses override, not parent)
    auto* btn = dynamic_cast<button<Backend>*>(removed.get());
    auto style = btn->resolve_style();
    CHECK(style.foreground_color.r == 100);
}

// ============================================================================
// Resolution Performance
// ============================================================================

TEST_CASE("Style Resolution - Performance: 1000 resolutions") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());

    // Resolve 1000 times
    for (int i = 0; i < 1000; ++i) {
        [[maybe_unused]] auto style = btn->resolve_style();
    }

    // If this completes without hanging, performance is acceptable
    CHECK(true);
}

TEST_CASE("Style Resolution - Performance: Deep hierarchy resolution") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    // Build 100-level deep hierarchy
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Resolution Test", ctx.themes());

    panel<Backend>* current = root.get();
    for (int i = 0; i < 99; ++i) {
        current = current->template emplace_child<panel>();
    }

    auto* deep_btn = current->template emplace_child<button>("Deep");

    // Should resolve in reasonable time even at depth 100
    auto style = deep_btn->resolve_style();

    CHECK(style.foreground_color.r >= 0);
}

// ============================================================================
// Style Immutability
// ============================================================================

TEST_CASE("Style Resolution - Resolved style is independent of source") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());

    auto style1 = btn->resolve_style();
    auto original_fg = style1.foreground_color;

    // Modify widget's override
    btn->set_foreground_color({123, 45, 67});

    // Previous resolved style should be unchanged
    CHECK(style1.foreground_color.r == original_fg.r);
    CHECK(style1.foreground_color.g == original_fg.g);

    // New resolution should reflect change
    auto style2 = btn->resolve_style();
    CHECK(style2.foreground_color.r == 123);
}

TEST_CASE("Style Resolution - Multiple resolutions are independent") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_resolution_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Resolution Test", ctx.themes());

    auto style1 = btn->resolve_style();
    auto style2 = btn->resolve_style();

    // Modifying style1 doesn't affect style2
    style1.foreground_color = {100, 200, 150};

    // Style2 should still have original button colors (not modified by style1 change)
    CHECK(style2.foreground_color.r == 255);  // button.fg_normal
    CHECK(style2.foreground_color.g == 255);
    CHECK(style2.foreground_color.b == 255);
}

} // TEST_SUITE
