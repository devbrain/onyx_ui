//
// test_content_area_bounds.cc - Content area bounds regression tests
//

#include <doctest/doctest.h>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/services/ui_context.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;

namespace {
    struct scaled_test_backend : test_backend {
        struct renderer : test_backend::renderer {
            using size = test_backend::size;
            using font = test_backend::renderer::font;

            static size measure_text(std::string_view text, const font& f) {
                auto measured = test_backend::renderer::measure_text(text, f);
                measured.h = 2;  // Simulate graphical backends with taller text
                return measured;
            }
        };

        using renderer_type = renderer;

        static constexpr const char* name() { return "ScaledTest"; }
    };

    struct scaled_ui_fixture {
        scaled_ui_fixture()
            : ctx(make_terminal_metrics<scaled_test_backend>()) {
            ui_theme<scaled_test_backend> theme;
            theme.name = "ScaledTestTheme";
            theme.window_bg = typename scaled_test_backend::color_type{50, 50, 50};
            theme.text_fg = typename scaled_test_backend::color_type{255, 255, 255};
            theme.border_color = typename scaled_test_backend::color_type{100, 100, 100};
            ctx.themes().register_theme(std::move(theme));
            ctx.themes().set_current_theme("ScaledTestTheme");
        }

        scoped_ui_context<scaled_test_backend> ctx;
    };
}  // namespace

TEST_SUITE("Content Area Bounds") {
    template<typename Widget>
    class content_area_probe : public Widget {
    public:
        using Widget::Widget;
        using Widget::get_content_area;
    };

    TEST_CASE_FIXTURE(scaled_ui_fixture, "Group box content stays within bounds with title offset") {
        content_area_probe<group_box<scaled_test_backend>> gb;
        gb.set_title("Actions");
        gb.set_vbox_layout(spacing::none);
        gb.emplace_child<label>("Child");

        (void)gb.measure(20_lu, 8_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 20_lu, 8_lu});

        auto const content = gb.get_content_area();
        CHECK((content.y + content.height).value <= gb.bounds().height.value);
    }

    TEST_CASE_FIXTURE(scaled_ui_fixture, "Tab widget content stays within bounds with tab bar") {
        content_area_probe<tab_widget<scaled_test_backend>> tabs;
        tabs.add_tab(std::make_unique<panel<scaled_test_backend>>(), "One");

        (void)tabs.measure(30_lu, 10_lu);
        tabs.arrange(logical_rect{0_lu, 0_lu, 30_lu, 10_lu});

        auto const content = tabs.get_content_area();
        CHECK((content.y + content.height).value <= tabs.bounds().height.value);
    }
}
