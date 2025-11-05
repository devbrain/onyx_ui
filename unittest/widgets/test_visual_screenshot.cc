#include "../../utils/ui_context_fixture.hh"
#include <onyxui/widgets/text_view.hh>
#include <iostream>
#include <fstream>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Visual screenshot") {
    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();

    // Same content as demo
    std::string demo_text =
        "Welcome to OnyxUI Text View Demo!\n"
        "\n"
        "This widget demonstrates:\n"
        "  * Multi-line text display\n"
        "  * Automatic scrolling\n"
        "\n";

    for (int i = 1; i <= 15; ++i) {
        demo_text += "[LOG " + std::to_string(i) + "] Entry at timestamp " +
                    std::to_string(1000 + i * 100) + " ms\n";
    }

    text_view_widget->set_text(demo_text);
    text_view_widget->set_has_border(true);

    // Render to canvas
    auto canvas = render_to_canvas(*text_view_widget, 120, 25);

    // Verify correct content is visible
    bool found_welcome = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("Welcome") != std::string::npos) found_welcome = true;
    }

    CHECK(found_welcome);
}
