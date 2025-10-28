#include <doctest/doctest.h>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include <iostream>

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

TEST_CASE("Debug box_style resolution") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().set_current_theme("Canvas Test Theme");
    
    auto* theme = ui_services<Backend>::themes()->get_current_theme();
    REQUIRE(theme != nullptr);
    
    // Check theme has correct box_style
    INFO("Theme panel.box_style.draw_border = ", theme->panel.box_style.draw_border);
    REQUIRE(theme->panel.box_style.draw_border == true);
    
    // Create simple panel
    panel<Backend> p;
    p.set_has_border(true);
    
    // Resolve style manually
    auto default_parent_style = resolved_style<Backend>::from_theme(*theme);
    auto resolved = p.resolve_style(theme, default_parent_style);
    
    INFO("Resolved box_style.draw_border = ", resolved.box_style.value.draw_border);
    CHECK(resolved.box_style.value.draw_border == true);
}
