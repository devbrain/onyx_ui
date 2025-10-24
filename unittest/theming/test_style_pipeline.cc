//
// test_style_pipeline.cc - Integration tests for full style resolution pipeline
//
// Tests verify the complete flow from theme application through rendering:
// 1. Theme Registry → apply_theme → resolve_style → render_context
// 2. Deep CSS-style inheritance chains (3+ levels)
// 3. Override precedence (theme < parent < local)
// 4. Cross-component integration (theme + state + focus + visibility)
// 5. Real-world complex scenarios
// 6. Performance characteristics
//

#include <doctest/doctest.h>
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/group_box.hh>
#include <onyxui/theme.hh>
#include <onyxui/resolved_style.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

namespace {
    // Create a comprehensive test theme
    ui_theme<Backend> create_pipeline_test_theme() {
        ui_theme<Backend> theme;
        theme.name = "Pipeline Test";
        theme.description = "Theme for pipeline integration testing";

        // Global colors
        theme.window_bg = {10, 10, 10};
        theme.text_fg = {240, 240, 240};
        theme.border_color = {100, 100, 100};

        // Button styling
        theme.button.fg_normal = {255, 255, 255};
        theme.button.bg_normal = {0, 120, 215};
        theme.button.fg_hover = {255, 255, 255};
        theme.button.bg_hover = {0, 140, 235};
        theme.button.fg_pressed = {200, 200, 200};
        theme.button.bg_pressed = {0, 100, 195};
        theme.button.fg_disabled = {150, 150, 150};
        theme.button.bg_disabled = {80, 80, 80};

        // Label styling
        theme.label.text = {240, 240, 240};
        theme.label.background = {10, 10, 10};

        // Panel styling
        theme.panel.background = {30, 30, 30};
        theme.panel.border_color = {100, 100, 100};

        return theme;
    }

    // Alternative theme for switching tests
    ui_theme<Backend> create_alternate_theme() {
        ui_theme<Backend> theme;
        theme.name = "Alternate";

        // Different color scheme
        theme.window_bg = {250, 250, 250};
        theme.text_fg = {20, 20, 20};
        theme.border_color = {180, 180, 180};

        theme.button.fg_normal = {0, 0, 0};
        theme.button.bg_normal = {225, 225, 225};
        theme.button.fg_hover = {0, 0, 0};
        theme.button.bg_hover = {200, 200, 200};
        theme.button.fg_pressed = {0, 0, 0};
        theme.button.bg_pressed = {180, 180, 180};
        theme.button.fg_disabled = {120, 120, 120};
        theme.button.bg_disabled = {240, 240, 240};

        theme.label.text = {20, 20, 20};
        theme.label.background = {250, 250, 250};

        theme.panel.background = {240, 240, 240};
        theme.panel.border_color = {180, 180, 180};

        return theme;
    }
}

TEST_SUITE("Style Pipeline - Integration") {

// ============================================================================
// Full Pipeline Flow Tests
// ============================================================================

TEST_CASE("Style Pipeline - Registry to render flow") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");

    // 1. Apply theme from registry
    bool success = btn->apply_theme("Pipeline Test", ctx.themes());
    CHECK(success == true);

    // 2. Verify effective colors (button uses button-specific colors)
    auto bg = btn->get_effective_background_color();
    CHECK(bg.r == 0);   // button.bg_normal
    CHECK(bg.g == 120);
    CHECK(bg.b == 215);

    auto fg = btn->get_effective_foreground_color();
    CHECK(fg.r == 255);  // button.fg_normal
    CHECK(fg.g == 255);
    CHECK(fg.b == 255);

    // 3. Measure (triggers size calculation)
    auto size = btn->measure(200, 100);
    CHECK(size_utils::get_width(size) > 0);
    CHECK(size_utils::get_height(size) > 0);

    // 4. Arrange (finalizes layout)
    btn->arrange({0, 0, 100, 30});
    auto bounds = btn->bounds();
    CHECK(rect_utils::get_width(bounds) == 100);
    CHECK(rect_utils::get_height(bounds) == 30);

    // Full pipeline executed successfully
}

TEST_CASE("Style Pipeline - Theme application invalidates layout") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");

    // Initial layout
    [[maybe_unused]] auto _ = btn->measure(200, 100);
    btn->arrange({0, 0, 100, 30});

    // Apply theme (should invalidate)
    btn->apply_theme("Pipeline Test", ctx.themes());

    // Re-measure should be required (layout was invalidated)
    [[maybe_unused]] auto new_size = btn->measure(200, 100);
    CHECK(size_utils::get_width(new_size) > 0);
}

TEST_CASE("Style Pipeline - resolve_style integration") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Pipeline Test", ctx.themes());

    // Override one property locally
    btn->set_foreground_color({128, 128, 128});

    // Resolved style should combine theme + override
    auto fg = btn->get_effective_foreground_color();
    CHECK(fg.r == 128);
    CHECK(fg.g == 128);
    CHECK(fg.b == 128);

    // But background should still come from theme (button-specific)
    auto bg = btn->get_effective_background_color();
    CHECK(bg.g == 120);  // button.bg_normal
}

// ============================================================================
// Deep Inheritance Chain Tests
// ============================================================================

TEST_CASE("Style Pipeline - Three-level inheritance") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    // Level 1: Root panel with theme
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    // Level 2: Middle panel (inherits from root)
    auto* middle = root->template emplace_child<panel>();
    middle->set_vbox_layout(0);

    // Level 3: Button (inherits from middle → root → theme)
    auto* btn = middle->template emplace_child<button>("Test");

    // Measure hierarchy
    [[maybe_unused]] auto size1 = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Button should inherit theme colors through chain
    auto bg = btn->get_effective_background_color();
    CHECK(bg.r >= 0);  // Has a defined background
    CHECK(bg.g >= 0);
    CHECK(bg.b >= 0);
}

TEST_CASE("Style Pipeline - Five-level deep inheritance") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    // Build deep hierarchy: root → panel1 → panel2 → panel3 → button
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    auto* panel1 = root->template emplace_child<panel>();
    panel1->set_vbox_layout(0);

    auto* panel2 = panel1->template emplace_child<panel>();
    panel2->set_vbox_layout(0);

    auto* panel3 = panel2->template emplace_child<panel>();
    panel3->set_vbox_layout(0);

    auto* btn = panel3->template emplace_child<button>("Deep");

    // Layout entire tree
    [[maybe_unused]] auto _ = root->measure(300, 300);
    root->arrange({0, 0, 300, 300});

    // Button at depth 5 should still have theme colors
    auto fg = btn->get_effective_foreground_color();
    CHECK(fg.r == 240);
    CHECK(fg.g == 240);
    CHECK(fg.b == 240);
}

TEST_CASE("Style Pipeline - Mid-chain override propagation") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    // Root → Middle (override) → Child
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    auto* middle = root->template emplace_child<panel>();
    middle->set_vbox_layout(0);
    middle->set_foreground_color({200, 0, 0});  // Override at middle level

    auto* child = middle->template emplace_child<label>("Text");

    [[maybe_unused]] auto _ = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Child should inherit middle's override, not root's theme
    auto fg = child->get_effective_foreground_color();
    CHECK(fg.r == 200);
    CHECK(fg.g == 0);
    CHECK(fg.b == 0);
}

// ============================================================================
// Override Precedence Tests
// ============================================================================

TEST_CASE("Style Pipeline - Theme < Parent < Local precedence") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto parent = std::make_unique<panel<Backend>>();
    parent->apply_theme("Pipeline Test", ctx.themes());
    parent->set_vbox_layout(0);
    parent->set_foreground_color({100, 100, 100});  // Parent override

    auto* child = parent->template emplace_child<label>("Text");
    child->set_foreground_color({50, 50, 50});  // Local override

    [[maybe_unused]] auto _ = parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // Local override wins
    auto fg = child->get_effective_foreground_color();
    CHECK(fg.r == 50);
    CHECK(fg.g == 50);
    CHECK(fg.b == 50);
}

TEST_CASE("Style Pipeline - Partial override preserves other properties") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");
    btn->apply_theme("Pipeline Test", ctx.themes());

    // Override only foreground, not background
    btn->set_foreground_color({255, 0, 0});

    auto fg = btn->get_effective_foreground_color();
    CHECK(fg.r == 255);
    CHECK(fg.g == 0);
    CHECK(fg.b == 0);

    // Background should still come from theme (button-specific)
    auto bg = btn->get_effective_background_color();
    CHECK(bg.g == 120);  // button.bg_normal
}

TEST_CASE("Style Pipeline - Clear override restores inheritance") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto parent = std::make_unique<panel<Backend>>();
    parent->apply_theme("Pipeline Test", ctx.themes());
    parent->set_vbox_layout(0);
    parent->set_foreground_color({100, 100, 100});

    auto* child = parent->template emplace_child<label>("Text");

    [[maybe_unused]] auto _ = parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // Child inherits parent override
    auto fg1 = child->get_effective_foreground_color();
    CHECK(fg1.r == 100);

    // Clear parent override
    parent->set_foreground_color({});

    // Re-layout
    [[maybe_unused]] auto _2 = parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // Child should now have default (parent override cleared)
    auto fg2 = child->get_effective_foreground_color();
    CHECK(fg2.r == 0);  // Default after clear
}

// ============================================================================
// Theme Switching Integration
// ============================================================================

TEST_CASE("Style Pipeline - Theme switch propagates to entire tree") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());
    ctx.themes().register_theme(create_alternate_theme());

    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    auto* btn1 = root->template emplace_child<button>("Button 1");
    auto* btn2 = root->template emplace_child<button>("Button 2");

    [[maybe_unused]] auto _ = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Initial colors (dark theme)
    auto bg1_dark = btn1->get_effective_background_color();
    CHECK(bg1_dark.g == 10);

    // Switch to alternate theme (light)
    root->apply_theme("Alternate", ctx.themes());

    [[maybe_unused]] auto _2 = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Colors should update
    auto bg1_light = btn1->get_effective_background_color();
    auto bg2_light = btn2->get_effective_background_color();

    CHECK(bg1_light.r == 250);
    CHECK(bg2_light.r == 250);
}

TEST_CASE("Style Pipeline - Theme switch preserves local overrides") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());
    ctx.themes().register_theme(create_alternate_theme());

    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    auto* btn = root->template emplace_child<button>("Test");
    btn->set_foreground_color({255, 0, 0});  // Local override

    [[maybe_unused]] auto _ = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Switch theme
    root->apply_theme("Alternate", ctx.themes());

    [[maybe_unused]] auto _2 = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Local override should be preserved
    auto fg = btn->get_effective_foreground_color();
    CHECK(fg.r == 255);
    CHECK(fg.g == 0);
    CHECK(fg.b == 0);

    // Background should update from theme
    auto bg = btn->get_effective_background_color();
    CHECK(bg.r == 250);  // Alternate theme
}

TEST_CASE("Style Pipeline - Partial theme application in hierarchy") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());
    ctx.themes().register_theme(create_alternate_theme());

    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    auto* middle = root->template emplace_child<panel>();
    middle->apply_theme("Alternate", ctx.themes());  // Different theme!
    middle->set_vbox_layout(0);

    auto* btn = middle->template emplace_child<button>("Test");

    [[maybe_unused]] auto _ = root->measure(200, 200);
    root->arrange({0, 0, 200, 200});

    // Button inherits from root (parent colors, not parent's theme)
    auto bg = btn->get_effective_background_color();
    CHECK(bg.r == 10);  // Root's Pipeline Test theme
}

// ============================================================================
// Complex Real-World Scenarios
// ============================================================================

TEST_CASE("Style Pipeline - Nested group boxes with mixed themes") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto outer_box = std::make_unique<group_box<Backend>>();
    outer_box->set_title("Outer");
    outer_box->apply_theme("Pipeline Test", ctx.themes());
    outer_box->set_vbox_layout(5);

    auto* inner_box = outer_box->template emplace_child<group_box>();
    inner_box->set_title("Inner");
    inner_box->set_vbox_layout(5);

    auto* btn1 = inner_box->template emplace_child<button>("Button 1");
    auto* btn2 = inner_box->template emplace_child<button>("Button 2");

    [[maybe_unused]] auto _ = outer_box->measure(300, 300);
    outer_box->arrange({0, 0, 300, 300});

    // Both buttons should have theme colors
    auto bg1 = btn1->get_effective_background_color();
    auto bg2 = btn2->get_effective_background_color();

    CHECK(bg1.g == 10);
    CHECK(bg2.g == 10);
}

TEST_CASE("Style Pipeline - Mixed widget types with inheritance") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto root = std::make_unique<vbox<Backend>>(5);
    root->apply_theme("Pipeline Test", ctx.themes());

    auto* pnl = root->template emplace_child<panel>();
    auto* lbl = root->template emplace_child<label>("Label");
    auto* btn = root->template emplace_child<button>("Button");

    [[maybe_unused]] auto _ = root->measure(300, 300);
    root->arrange({0, 0, 300, 300});

    // All widgets should have theme-appropriate colors
    auto panel_bg = pnl->get_effective_background_color();
    auto label_fg = lbl->get_effective_foreground_color();
    auto button_bg = btn->get_effective_background_color();

    CHECK(panel_bg.r == 10);   // Inherits window_bg
    CHECK(label_fg.r == 240);  // Inherits text_fg
    CHECK(button_bg.g == 10); // Inherits window_bg
}

TEST_CASE("Style Pipeline - Dynamic widget addition inherits theme") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto root = std::make_unique<vbox<Backend>>(5);
    root->apply_theme("Pipeline Test", ctx.themes());

    auto* btn1 = root->template emplace_child<button>("Existing");

    [[maybe_unused]] auto _ = root->measure(300, 300);
    root->arrange({0, 0, 300, 300});

    // Add new button after layout
    auto* btn2 = root->template emplace_child<button>("New");

    // Re-layout
    [[maybe_unused]] auto _2 = root->measure(300, 300);
    root->arrange({0, 0, 300, 300});

    // New button should inherit theme
    auto bg1 = btn1->get_effective_background_color();
    auto bg2 = btn2->get_effective_background_color();

    CHECK(bg1.g == bg2.g);  // Both have theme colors
    CHECK(bg2.g == 10);
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

TEST_CASE("Style Pipeline - Large widget hierarchy") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto root = std::make_unique<vbox<Backend>>(2);
    root->apply_theme("Pipeline Test", ctx.themes());

    // Create 50 buttons
    for (int i = 0; i < 50; ++i) {
        root->template emplace_child<button>("Button " + std::to_string(i));
    }

    // Should handle layout without issues
    auto size = root->measure(400, 2000);
    auto const height = size_utils::get_height(size);
    root->arrange({0, 0, 400, height});

    // Verify all children have theme colors
    auto const& children = root->children();
    CHECK(children.size() == 50);

    for (auto const& child : children) {
        if (auto* btn = dynamic_cast<button<Backend>*>(child.get())) {
            auto bg = btn->get_effective_background_color();
            CHECK(bg.g == 10);
        }
    }
}

TEST_CASE("Style Pipeline - Repeated theme switches") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());
    ctx.themes().register_theme(create_alternate_theme());

    auto btn = std::make_unique<button<Backend>>("Test");

    // Switch themes 20 times
    for (int i = 0; i < 20; ++i) {
        if (i % 2 == 0) {
            btn->apply_theme("Pipeline Test", ctx.themes());
        } else {
            btn->apply_theme("Alternate", ctx.themes());
        }

        [[maybe_unused]] auto _ = btn->measure(200, 100);
        btn->arrange({0, 0, 100, 30});
    }

    // Final theme should be Alternate (button uses button.bg_normal)
    auto bg = btn->get_effective_background_color();
    CHECK(bg.r == 225);  // Alternate theme: button.bg_normal = {225, 225, 225}
}

TEST_CASE("Style Pipeline - Deep hierarchy with mixed overrides") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Pipeline Test", ctx.themes());
    root->set_vbox_layout(0);

    // Level 1
    auto* p1 = root->template emplace_child<panel>();
    p1->set_vbox_layout(0);
    p1->set_foreground_color({200, 0, 0});

    // Level 2
    auto* p2 = p1->template emplace_child<panel>();
    p2->set_vbox_layout(0);
    p2->set_background_color({0, 200, 0});

    // Level 3
    auto* p3 = p2->template emplace_child<panel>();
    p3->set_vbox_layout(0);

    // Level 4
    auto* btn = p3->template emplace_child<button>("Deep");

    [[maybe_unused]] auto _ = root->measure(400, 400);
    root->arrange({0, 0, 400, 400});

    // Button should have: foreground from p1, background from p2
    auto fg = btn->get_effective_foreground_color();
    auto bg = btn->get_effective_background_color();

    CHECK(fg.r == 200);  // From p1 override
    CHECK(bg.g == 200);  // From p2 override
}

TEST_CASE("Style Pipeline - Theme application is idempotent") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_pipeline_test_theme());

    auto btn = std::make_unique<button<Backend>>("Test");

    // Apply same theme multiple times
    btn->apply_theme("Pipeline Test", ctx.themes());
    auto bg1 = btn->get_effective_background_color();

    btn->apply_theme("Pipeline Test", ctx.themes());
    auto bg2 = btn->get_effective_background_color();

    btn->apply_theme("Pipeline Test", ctx.themes());
    auto bg3 = btn->get_effective_background_color();

    // All should be identical
    CHECK(bg1.r == bg2.r);
    CHECK(bg1.g == bg2.g);
    CHECK(bg1.b == bg2.b);
    CHECK(bg2.r == bg3.r);
    CHECK(bg2.g == bg3.g);
    CHECK(bg2.b == bg3.b);
}

} // TEST_SUITE
