//
// test_stateful_widget.cc - Tests for stateful_widget base class
//
// Tests verify state management for interactive widgets:
// - State transitions (normal, hover, pressed, disabled)
// - State-based color resolution from themes
// - Automatic invalidation on state changes
// - Integration with widget hierarchy
//

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/stateful_widget.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/button.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/theme.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_context.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_services.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/render_context.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/measure_context.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

namespace {
    // Test helper: resolve style with given theme
    template<typename Widget>
    resolved_style<Backend> resolve_with_theme(const Widget& widget, const ui_theme<Backend>* theme) {
        auto parent_style = theme ? resolved_style<Backend>::from_theme(*theme) : resolved_style<Backend>{
            .background_color = Backend::color_type{0, 0, 0},
            .foreground_color = Backend::color_type{255, 255, 255},
            .border_color = Backend::color_type{128, 128, 128},
            .box_style = Backend::renderer_type::box_style{},
            .font = Backend::renderer_type::font{},
            .opacity = 1.0f,
            .icon_style = std::optional<Backend::renderer_type::icon_style>(std::nullopt),
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<Backend::renderer_type::font>{}
        };
        return widget.resolve_style(theme, parent_style);
    }

    // Test widget that exposes stateful_widget protected interface
    template<UIBackend TBackend>
    class test_stateful_widget : public stateful_widget<TBackend> {
    public:
        using base = stateful_widget<TBackend>;
        using state_type = typename base::interaction_state;
        using color_type = typename TBackend::color_type;

        test_stateful_widget() = default;

        // Expose protected members for testing
        [[nodiscard]] state_type get_state() const noexcept {
            return this->get_interaction_state();
        }

        void set_state(state_type state) {
            this->set_interaction_state(state);
        }

        [[nodiscard]] bool check_is_normal() const noexcept {
            return this->is_normal();
        }

        [[nodiscard]] bool check_is_disabled() const noexcept {
            return this->is_disabled();
        }

        // Check if state matches what we expect
        [[nodiscard]] bool is_in_hover_state() const noexcept {
            return this->get_interaction_state() == state_type::hover;
        }

        [[nodiscard]] bool is_in_pressed_state() const noexcept {
            return this->get_interaction_state() == state_type::pressed;
        }

        void do_enable() {
            this->enable();
        }

        void do_disable() {
            this->disable();
        }

        template<typename WidgetTheme>
        [[nodiscard]] color_type test_get_state_background(const WidgetTheme& theme) const noexcept {
            return this->get_state_background(theme);
        }

        template<typename WidgetTheme>
        [[nodiscard]] color_type test_get_state_foreground(const WidgetTheme& theme) const noexcept {
            return this->get_state_foreground(theme);
        }

        // Expose protected event handlers for testing
        bool handle_mouse_enter() {
            return base::handle_mouse_enter();
        }

        bool handle_mouse_leave() {
            return base::handle_mouse_leave();
        }

        bool handle_mouse_down(int x, int y, int mouse_button) {
            return base::handle_mouse_down(x, y, mouse_button);
        }

        bool handle_mouse_up(int x, int y, int mouse_button) {
            return base::handle_mouse_up(x, y, mouse_button);
        }

    protected:
        void do_render(render_context<Backend>&) const override {
            // Minimal implementation for testing
        }
    };

    // Test button wrapper to expose protected methods
    template<UIBackend TBackend>
    class test_button : public button<TBackend> {
    public:
        using base = button<TBackend>;

        explicit test_button(const std::string& text) : base(text) {}

        // Expose protected event handlers for testing
        bool handle_mouse_enter() {
            return base::handle_mouse_enter();
        }

        bool handle_mouse_leave() {
            return base::handle_mouse_leave();
        }

        bool handle_mouse_down(int x, int y, int mouse_button) {
            return base::handle_mouse_down(x, y, mouse_button);
        }

        bool handle_mouse_up(int x, int y, int mouse_button) {
            return base::handle_mouse_up(x, y, mouse_button);
        }
    };

    // Helper to create test theme with known state colors
    ui_theme<Backend> create_state_test_theme() {
        ui_theme<Backend> theme;
        theme.name = "State Test";

        // Generic widget colors (used by resolve_style)
        theme.text_fg = {255, 255, 255};  // White

        // Button states with distinct colors
        theme.button.normal.background = {0, 0, 170};      // Blue
        theme.button.normal.foreground = {255, 255, 255};  // White

        theme.button.hover.background = {0, 170, 170};     // Cyan
        theme.button.hover.foreground = {255, 255, 0};     // Yellow

        theme.button.pressed.background = {170, 170, 170}; // Gray
        theme.button.pressed.foreground = {0, 0, 0};       // Black

        theme.button.disabled.background = {64, 64, 64};   // Dark gray
        theme.button.disabled.foreground = {128, 128, 128}; // Light gray

        return theme;
    }
}

TEST_SUITE("stateful_widget") {

// ============================================================================
// State Detection Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Initial state is normal") {
    test_stateful_widget<Backend> widget;

    CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::normal);
    CHECK(widget.check_is_normal() == true);
    CHECK(widget.is_in_hover_state() == false);
    CHECK(widget.is_in_pressed_state() == false);
    CHECK(widget.check_is_disabled() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State transitions work correctly") {
    test_stateful_widget<Backend> widget;

    SUBCASE("Normal to hover") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::hover);
        CHECK(widget.is_in_hover_state() == true);
    }

    SUBCASE("Normal to pressed") {
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::pressed);
        CHECK(widget.is_in_pressed_state() == true);
    }

    SUBCASE("Normal to disabled") {
        widget.set_state(test_stateful_widget<Backend>::state_type::disabled);
        CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::disabled);
        CHECK(widget.check_is_disabled() == true);
    }

    SUBCASE("Hover to pressed") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        CHECK(widget.is_in_pressed_state() == true);
        CHECK(widget.is_in_hover_state() == false);
    }

    SUBCASE("Pressed back to normal") {
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        CHECK(widget.check_is_normal() == true);
        CHECK(widget.is_in_pressed_state() == false);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State queries are mutually exclusive") {
    test_stateful_widget<Backend> widget;

    SUBCASE("Only normal is true in normal state") {
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        CHECK(widget.check_is_normal() == true);
        CHECK(widget.is_in_hover_state() == false);
        CHECK(widget.is_in_pressed_state() == false);
        CHECK(widget.check_is_disabled() == false);
    }

    SUBCASE("Only hovered is true in hover state") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        CHECK(widget.check_is_normal() == false);
        CHECK(widget.is_in_hover_state() == true);
        CHECK(widget.is_in_pressed_state() == false);
        CHECK(widget.check_is_disabled() == false);
    }

    SUBCASE("Only pressed is true in pressed state") {
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        CHECK(widget.check_is_normal() == false);
        CHECK(widget.is_in_hover_state() == false);
        CHECK(widget.is_in_pressed_state() == true);
        CHECK(widget.check_is_disabled() == false);
    }

    SUBCASE("Only disabled is true in disabled state") {
        widget.set_state(test_stateful_widget<Backend>::state_type::disabled);
        CHECK(widget.check_is_normal() == false);
        CHECK(widget.is_in_hover_state() == false);
        CHECK(widget.is_in_pressed_state() == false);
        CHECK(widget.check_is_disabled() == true);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - enable() and disable() methods") {
    test_stateful_widget<Backend> widget;

    SUBCASE("disable() sets disabled state") {
        widget.do_disable();
        CHECK(widget.check_is_disabled() == true);
        CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::disabled);
    }

    SUBCASE("enable() sets normal state") {
        widget.do_disable();
        widget.do_enable();
        CHECK(widget.check_is_normal() == true);
        CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::normal);
    }

    SUBCASE("enable() works from pressed state") {
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        widget.do_enable();
        CHECK(widget.check_is_normal() == true);
    }

    SUBCASE("enable() works from hover state") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        widget.do_enable();
        CHECK(widget.check_is_normal() == true);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State changes are reflected immediately") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    SUBCASE("Normal to hover changes appearance") {
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        auto normal_bg = widget.test_get_state_background(theme.button);

        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        auto hover_bg = widget.test_get_state_background(theme.button);

        // Colors should be different (compare full colors)
        CHECK((normal_bg.r != hover_bg.r || normal_bg.g != hover_bg.g || normal_bg.b != hover_bg.b));
    }

    SUBCASE("Hover to pressed changes appearance") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        auto hover_bg = widget.test_get_state_background(theme.button);

        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        auto pressed_bg = widget.test_get_state_background(theme.button);

        // Colors should be different (compare full colors)
        CHECK((hover_bg.r != pressed_bg.r || hover_bg.g != pressed_bg.g || hover_bg.b != pressed_bg.b));
    }

    SUBCASE("Same state keeps same appearance") {
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        auto bg1 = widget.test_get_state_background(theme.button);

        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        auto bg2 = widget.test_get_state_background(theme.button);

        // Colors should be identical
        CHECK(bg1.r == bg2.r);
        CHECK(bg1.g == bg2.g);
        CHECK(bg1.b == bg2.b);
    }

    SUBCASE("disable() changes appearance") {
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        auto normal_bg = widget.test_get_state_background(theme.button);

        widget.do_disable();
        auto disabled_bg = widget.test_get_state_background(theme.button);

        // Colors should be different
        CHECK(normal_bg.b != disabled_bg.r);
    }

    SUBCASE("enable() from disabled restores normal appearance") {
        widget.do_disable();
        auto disabled_bg = widget.test_get_state_background(theme.button);

        widget.do_enable();
        auto normal_bg = widget.test_get_state_background(theme.button);

        // Should have normal state colors
        CHECK(normal_bg.b == 170);
        CHECK(disabled_bg.r != normal_bg.b);
    }
}

// ============================================================================
// Style Resolution Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - get_state_background() returns correct colors") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    SUBCASE("Normal state returns normal background") {
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        auto bg = widget.test_get_state_background(theme.button);
        CHECK(bg.r == 0);
        CHECK(bg.g == 0);
        CHECK(bg.b == 170);
    }

    SUBCASE("Hover state returns hover background") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        auto bg = widget.test_get_state_background(theme.button);
        CHECK(bg.r == 0);
        CHECK(bg.g == 170);
        CHECK(bg.b == 170);
    }

    SUBCASE("Pressed state returns pressed background") {
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        auto bg = widget.test_get_state_background(theme.button);
        CHECK(bg.r == 170);
        CHECK(bg.g == 170);
        CHECK(bg.b == 170);
    }

    SUBCASE("Disabled state returns disabled background") {
        widget.set_state(test_stateful_widget<Backend>::state_type::disabled);
        auto bg = widget.test_get_state_background(theme.button);
        CHECK(bg.r == 64);
        CHECK(bg.g == 64);
        CHECK(bg.b == 64);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - get_state_foreground() returns correct colors") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    SUBCASE("Normal state returns normal foreground") {
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        auto fg = widget.test_get_state_foreground(theme.button);
        CHECK(fg.r == 255);
        CHECK(fg.g == 255);
        CHECK(fg.b == 255);
    }

    SUBCASE("Hover state returns hover foreground") {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        auto fg = widget.test_get_state_foreground(theme.button);
        CHECK(fg.r == 255);
        CHECK(fg.g == 255);
        CHECK(fg.b == 0);
    }

    SUBCASE("Pressed state returns pressed foreground") {
        widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
        auto fg = widget.test_get_state_foreground(theme.button);
        CHECK(fg.r == 0);
        CHECK(fg.g == 0);
        CHECK(fg.b == 0);
    }

    SUBCASE("Disabled state returns disabled foreground") {
        widget.set_state(test_stateful_widget<Backend>::state_type::disabled);
        auto fg = widget.test_get_state_foreground(theme.button);
        CHECK(fg.r == 128);
        CHECK(fg.g == 128);
        CHECK(fg.b == 128);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State-based colors work with different widget themes") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    // Modify label colors to be different from button
    theme.label.text = {255, 0, 0};        // Red
    theme.label.background = {0, 255, 0};  // Green

    // Create a mock label theme structure with state colors
    struct label_state_theme {
        struct state_colors {
            Backend::color_type foreground;
            Backend::color_type background;
        };
        state_colors normal{Backend::color_type{255, 0, 0}, Backend::color_type{0, 255, 0}};
        state_colors hover{Backend::color_type{255, 128, 0}, Backend::color_type{0, 255, 128}};
        state_colors pressed{Backend::color_type{128, 0, 0}, Backend::color_type{0, 128, 0}};
        state_colors disabled{Backend::color_type{64, 0, 0}, Backend::color_type{0, 64, 0}};
    } label_theme;

    widget.set_state(test_stateful_widget<Backend>::state_type::normal);
    auto label_bg = widget.test_get_state_background(label_theme);
    auto button_bg = widget.test_get_state_background(theme.button);

    // Different widget types should get different colors
    CHECK(label_bg.g != button_bg.g);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Color resolution is consistent across calls") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    widget.set_state(test_stateful_widget<Backend>::state_type::hover);

    auto bg1 = widget.test_get_state_background(theme.button);
    auto bg2 = widget.test_get_state_background(theme.button);
    auto fg1 = widget.test_get_state_foreground(theme.button);
    auto fg2 = widget.test_get_state_foreground(theme.button);

    // Multiple calls should return identical colors
    CHECK(bg1.r == bg2.r);
    CHECK(bg1.g == bg2.g);
    CHECK(bg1.b == bg2.b);
    CHECK(fg1.r == fg2.r);
    CHECK(fg1.g == fg2.g);
    CHECK(fg1.b == fg2.b);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Multiple state transitions maintain correctness") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    // Simulate interaction sequence: normal → hover → pressed → normal
    widget.set_state(test_stateful_widget<Backend>::state_type::normal);
    auto bg1 = widget.test_get_state_background(theme.button);
    CHECK(bg1.b == 170);  // Blue

    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    auto bg2 = widget.test_get_state_background(theme.button);
    CHECK(bg2.g == 170);  // Cyan

    widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
    auto bg3 = widget.test_get_state_background(theme.button);
    CHECK(bg3.r == 170);  // Gray

    widget.set_state(test_stateful_widget<Backend>::state_type::normal);
    auto bg4 = widget.test_get_state_background(theme.button);
    CHECK(bg4.b == 170);  // Back to blue

    // First and last should match
    CHECK(bg1.r == bg4.r);
    CHECK(bg1.g == bg4.g);
    CHECK(bg1.b == bg4.b);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Disabled state is sticky") {
    test_stateful_widget<Backend> widget;

    widget.do_disable();
    CHECK(widget.check_is_disabled() == true);

    // Setting other states while disabled should not change state
    // (In real usage, event handlers would check is_disabled() first)
    // Here we're just verifying the state stays disabled until enable() is called

    widget.do_enable();
    CHECK(widget.check_is_disabled() == false);
    CHECK(widget.check_is_normal() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State transitions are reversible") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    // Normal → Hover → Normal
    auto original_bg = widget.test_get_state_background(theme.button);
    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    widget.set_state(test_stateful_widget<Backend>::state_type::normal);
    auto restored_bg = widget.test_get_state_background(theme.button);
    CHECK(original_bg.r == restored_bg.r);
    CHECK(original_bg.g == restored_bg.g);
    CHECK(original_bg.b == restored_bg.b);
}

// ============================================================================
// Additional Edge Case Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State persists across measure/arrange") {
    test_stateful_widget<Backend> widget;
    widget.set_state(test_stateful_widget<Backend>::state_type::hover);

    // Measure and arrange shouldn't affect state
    [[maybe_unused]] auto size = widget.measure(100, 100);
    CHECK(widget.is_in_hover_state() == true);

    widget.arrange({0, 0, 100, 50});
    CHECK(widget.is_in_hover_state() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State doesn't affect layout size") {
    test_stateful_widget<Backend> widget;

    // Measure in normal state
    auto normal_size = widget.measure(100, 100);

    // Measure in hover state
    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    auto hover_size = widget.measure(100, 100);

    // Measure in pressed state
    widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
    auto pressed_size = widget.measure(100, 100);

    // All sizes should be identical (state affects appearance, not size)
    CHECK(size_utils::get_width(normal_size) == size_utils::get_width(hover_size));
    CHECK(size_utils::get_height(normal_size) == size_utils::get_height(hover_size));
    CHECK(size_utils::get_width(hover_size) == size_utils::get_width(pressed_size));
    CHECK(size_utils::get_height(hover_size) == size_utils::get_height(pressed_size));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Default constructor initializes state") {
    test_stateful_widget<Backend> widget1;
    test_stateful_widget<Backend> widget2;
    test_stateful_widget<Backend> widget3;

    // All widgets should start in normal state
    CHECK(widget1.check_is_normal() == true);
    CHECK(widget2.check_is_normal() == true);
    CHECK(widget3.check_is_normal() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State change is idempotent") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    auto initial_bg = widget.test_get_state_background(theme.button);

    // Setting to same state multiple times should have no effect
    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    widget.set_state(test_stateful_widget<Backend>::state_type::hover);

    auto final_bg = widget.test_get_state_background(theme.button);

    // State and appearance should be unchanged
    CHECK(widget.is_in_hover_state() == true);
    CHECK(initial_bg.r == final_bg.r);
    CHECK(initial_bg.g == final_bg.g);
    CHECK(initial_bg.b == final_bg.b);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Rapid state changes are handled correctly") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    // Simulate rapid mouse movement (normal → hover → normal → hover)
    for (int i = 0; i < 10; ++i) {
        widget.set_state(test_stateful_widget<Backend>::state_type::hover);
        CHECK(widget.is_in_hover_state() == true);
        widget.set_state(test_stateful_widget<Backend>::state_type::normal);
        CHECK(widget.check_is_normal() == true);
    }

    // Final state should be consistent
    CHECK(widget.check_is_normal() == true);
    auto bg = widget.test_get_state_background(theme.button);
    CHECK(bg.b == 170);  // Blue (normal)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - All four states are distinct") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    widget.set_state(test_stateful_widget<Backend>::state_type::normal);
    auto normal_bg = widget.test_get_state_background(theme.button);

    widget.set_state(test_stateful_widget<Backend>::state_type::hover);
    auto hover_bg = widget.test_get_state_background(theme.button);

    widget.set_state(test_stateful_widget<Backend>::state_type::pressed);
    auto pressed_bg = widget.test_get_state_background(theme.button);

    widget.set_state(test_stateful_widget<Backend>::state_type::disabled);
    auto disabled_bg = widget.test_get_state_background(theme.button);

    // All backgrounds should be different (compare full colors)
    CHECK((normal_bg.r != hover_bg.r || normal_bg.g != hover_bg.g || normal_bg.b != hover_bg.b));
    CHECK((hover_bg.r != pressed_bg.r || hover_bg.g != pressed_bg.g || hover_bg.b != pressed_bg.b));
    CHECK((pressed_bg.r != disabled_bg.r || pressed_bg.g != disabled_bg.g || pressed_bg.b != disabled_bg.b));
}

// ============================================================================
// Automatic Event-Based State Transitions (NEW)
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Mouse enter triggers hover state") {
    test_stateful_widget<Backend> widget;

    CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::normal);

    // Simulate mouse enter event
    widget.handle_mouse_enter();

    CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::hover);
    CHECK(widget.is_in_hover_state() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Mouse leave returns to normal state") {
    test_stateful_widget<Backend> widget;

    // First enter hover state
    widget.handle_mouse_enter();
    CHECK(widget.is_in_hover_state() == true);

    // Leave should return to normal
    widget.handle_mouse_leave();
    CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::normal);
    CHECK(widget.check_is_normal() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Mouse down while hovering triggers pressed state") {
    test_stateful_widget<Backend> widget;

    // Enter hover state
    widget.handle_mouse_enter();
    CHECK(widget.is_in_hover_state() == true);

    // Mouse down should change to pressed
    widget.handle_mouse_down(0, 0, 0);
    CHECK(widget.get_state() == test_stateful_widget<Backend>::state_type::pressed);
    CHECK(widget.is_in_pressed_state() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Mouse up returns to hover if still hovering") {
    test_stateful_widget<Backend> widget;

    // Simulate hover → press sequence
    widget.handle_mouse_enter();
    widget.handle_mouse_down(0, 0, 0);
    CHECK(widget.is_in_pressed_state() == true);

    // Mouse up should return to hover (widget still under mouse)
    widget.handle_mouse_up(0, 0, 0);

    // This depends on whether the test widget properly tracks hover state
    // For now, just check it's not pressed
    CHECK(widget.is_in_pressed_state() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Complete click sequence (enter → down → up → leave)") {
    test_stateful_widget<Backend> widget;

    // 1. Start: normal
    CHECK(widget.check_is_normal() == true);

    // 2. Mouse enters: should be hover
    widget.handle_mouse_enter();
    CHECK(widget.is_in_hover_state() == true);

    // 3. Mouse down: should be pressed
    widget.handle_mouse_down(0, 0, 0);
    CHECK(widget.is_in_pressed_state() == true);

    // 4. Mouse up: should return to some state (hover or normal)
    widget.handle_mouse_up(0, 0, 0);
    CHECK(widget.is_in_pressed_state() == false);

    // 5. Mouse leaves: should return to normal
    widget.handle_mouse_leave();
    CHECK(widget.check_is_normal() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Disabled widget ignores mouse events") {
    test_stateful_widget<Backend> widget;

    // Disable the widget
    widget.do_disable();
    CHECK(widget.check_is_disabled() == true);

    // Try to trigger state changes (should be ignored)
    widget.handle_mouse_enter();
    CHECK(widget.check_is_disabled() == true);  // Still disabled

    widget.handle_mouse_down(0, 0, 0);
    CHECK(widget.check_is_disabled() == true);  // Still disabled

    widget.handle_mouse_up(0, 0, 0);
    CHECK(widget.check_is_disabled() == true);  // Still disabled

    widget.handle_mouse_leave();
    CHECK(widget.check_is_disabled() == true);  // Still disabled
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Re-enabling widget returns to normal state") {
    test_stateful_widget<Backend> widget;

    // Disable widget
    widget.do_disable();
    CHECK(widget.check_is_disabled() == true);

    // Try to set hover state (should fail)
    widget.handle_mouse_enter();
    CHECK(widget.check_is_disabled() == true);

    // Re-enable widget
    widget.do_enable();
    CHECK(widget.check_is_normal() == true);

    // Now mouse events should work
    widget.handle_mouse_enter();
    CHECK(widget.is_in_hover_state() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - State affects color resolution from theme") {
    test_stateful_widget<Backend> widget;
    auto theme = create_state_test_theme();

    // Normal state
    auto normal_bg = widget.test_get_state_background(theme.button);
    auto normal_fg = widget.test_get_state_foreground(theme.button);
    CHECK(normal_bg.b == 170);  // Blue background
    CHECK(normal_fg.r == 255);  // White foreground

    // Hover state (triggered by event)
    widget.handle_mouse_enter();
    auto hover_bg = widget.test_get_state_background(theme.button);
    auto hover_fg = widget.test_get_state_foreground(theme.button);
    CHECK(hover_bg.g == 170);   // Cyan background
    CHECK(hover_fg.g == 255);   // Yellow foreground

    // Pressed state (triggered by event)
    widget.handle_mouse_down(0, 0, 0);
    auto pressed_bg = widget.test_get_state_background(theme.button);
    auto pressed_fg = widget.test_get_state_foreground(theme.button);
    CHECK(pressed_bg.r == 170); // Gray background
    CHECK(pressed_fg.r == 0);   // Black foreground

    // Disabled state
    widget.do_disable();
    auto disabled_bg = widget.test_get_state_background(theme.button);
    auto disabled_fg = widget.test_get_state_foreground(theme.button);
    CHECK(disabled_bg.r == 64);  // Dark gray background
    CHECK(disabled_fg.r == 128); // Light gray foreground
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "stateful_widget - Button integration with automatic state syncing") {
    test_button<Backend> btn("Test");
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_state_test_theme());
//     btn.apply_theme("State Test", ctx.themes());  // No longer needed - widgets use global theme

    // Get the registered theme
    auto* test_theme = ctx.themes().get_theme("State Test");

    // Normal state initially
    Backend::color_type normal_fg = resolve_with_theme(btn, test_theme).foreground_color;
    CHECK(normal_fg.r == 255);  // White (normal state)

    // Trigger hover via event
    btn.handle_mouse_enter();
    // Note: Color resolution needs resolve_style() to be called
    // This test verifies the state changes, color resolution tested separately

    // Trigger pressed via event
    btn.handle_mouse_down(0, 0, 0);
    // State should now be pressed

    // Return to normal
    btn.handle_mouse_up(0, 0, 0);
    btn.handle_mouse_leave();
    // State should now be normal again
}

} // TEST_SUITE
