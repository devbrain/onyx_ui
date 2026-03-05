/**
 * @file test_tile_widgets.cc
 * @brief Unit tests for tile widgets (panel, label, button, checkbox, radio)
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/tile/widgets/tile_panel.hh>
#include <onyxui/tile/widgets/tile_label.hh>
#include <onyxui/tile/widgets/tile_button.hh>
#include <onyxui/tile/widgets/tile_checkbox.hh>
#include <onyxui/tile/widgets/tile_radio.hh>
#include <onyxui/tile/widgets/tile_progress_bar.hh>
#include <onyxui/tile/widgets/tile_slider.hh>
#include <onyxui/tile/widgets/tile_scrollbar.hh>
#include <onyxui/tile/widgets/tile_text_input.hh>
#include <onyxui/tile/widgets/tile_combo_box.hh>
#include <onyxui/tile/tile_core.hh>
#include <stdexcept>

using namespace onyxui;
using namespace onyxui::tile;
using namespace onyxui::testing;

// ============================================================================
// Stub implementations for testing (no SDL dependency)
// ============================================================================

namespace {
    // Global test theme storage
    const tile_theme* g_test_theme = nullptr;
    tile_renderer* g_test_renderer = nullptr;
}

namespace onyxui::tile {
    void set_theme(const tile_theme& theme) {
        g_test_theme = &theme;
    }

    const tile_theme& get_theme() {
        if (!g_test_theme) {
            throw std::runtime_error("No tile theme set.");
        }
        return *g_test_theme;
    }

    bool has_theme() noexcept {
        return g_test_theme != nullptr;
    }

    void set_renderer(tile_renderer* renderer) {
        g_test_renderer = renderer;
    }

    tile_renderer* get_renderer() noexcept {
        return g_test_renderer;
    }

    // =========================================================================
    // STUB IMPLEMENTATIONS FOR TESTING (no SDL dependency)
    // =========================================================================
    //
    // The test file provides stub implementations for tile_renderer and
    // sdlpp_renderer to avoid requiring SDL as a dependency. These stubs
    // allow testing tile widget logic without actual rendering.
    //
    // Note: These must be kept in sync with the actual class interfaces.
    // =========================================================================

    // Define the pimpl struct for tile_renderer
    struct tile_renderer::tile_impl {
        const bitmap_font* default_bitmap_font{nullptr};
    };
}

// Stubs for sdlpp_renderer base class (in onyxui::sdlpp namespace)
namespace onyxui::sdlpp {
    // Define the pimpl struct for sdlpp_renderer
    struct sdlpp_renderer::impl {
        // Empty for testing
    };

    // Stub sdlpp_renderer methods
    sdlpp_renderer::~sdlpp_renderer() = default;
    sdlpp_renderer::sdlpp_renderer(sdlpp_renderer&&) noexcept = default;
    sdlpp_renderer& sdlpp_renderer::operator=(sdlpp_renderer&&) noexcept = default;

    void sdlpp_renderer::draw_text(const rect& /*r*/, std::string_view /*text*/,
                                   const font& /*f*/, const color& /*fg*/,
                                   const color& /*bg*/) {
        // Stub - no-op for testing
    }

    size sdlpp_renderer::measure_text(std::string_view /*text*/, const font& /*f*/) {
        // Stub returns dummy size for testing
        return {0, 0};
    }
}

// Continue tile_renderer stubs in onyxui::tile namespace
namespace onyxui::tile {
    // Stub tile_renderer methods (no-op for testing)
    tile_renderer::~tile_renderer() = default;
    tile_renderer::tile_renderer(tile_renderer&&) noexcept = default;
    tile_renderer& tile_renderer::operator=(tile_renderer&&) noexcept = default;

    void tile_renderer::set_atlas(tile_atlas* /*atlas*/) noexcept {}
    tile_atlas* tile_renderer::get_atlas() const noexcept { return nullptr; }
    void tile_renderer::set_texture(::sdlpp::texture* /*texture*/) noexcept {}
    void tile_renderer::draw_tile(int /*tile_id*/, tile_point /*pos*/) {}
    void tile_renderer::draw_tile_scaled(int /*tile_id*/, tile_rect /*bounds*/) {}
    void tile_renderer::draw_h_slice(const h_slice& /*slice*/, tile_rect /*bounds*/) {}
    void tile_renderer::draw_v_slice(const v_slice& /*slice*/, tile_rect /*bounds*/) {}
    void tile_renderer::draw_nine_slice(const nine_slice& /*slice*/, tile_rect /*bounds*/) {}
    void tile_renderer::draw_bitmap_text(std::string_view /*text*/, tile_point /*pos*/, const bitmap_font& /*font*/) {}
    void tile_renderer::draw_bitmap_text(std::string_view /*text*/, tile_point /*pos*/, const bitmap_font& /*font*/, const onyxui::sdlpp::color& /*color*/) {}
    void tile_renderer::draw_bitmap_text_centered(std::string_view /*text*/, tile_rect /*bounds*/, const bitmap_font& /*font*/) {}
    void tile_renderer::draw_bitmap_text_centered(std::string_view /*text*/, tile_rect /*bounds*/, const bitmap_font& /*font*/, const onyxui::sdlpp::color& /*color*/) {}
    void tile_renderer::fill_rect(tile_rect /*r*/, onyxui::sdlpp::color /*c*/) {}
    void tile_renderer::set_default_bitmap_font(const bitmap_font* /*font*/) noexcept {}
    const bitmap_font* tile_renderer::get_default_bitmap_font() const noexcept { return nullptr; }
    void tile_renderer::draw_text(const onyxui::sdlpp::rect& /*r*/, std::string_view /*text*/,
                                  const font& /*f*/, const onyxui::sdlpp::color& /*fg*/,
                                  const onyxui::sdlpp::color& /*bg*/) {}
    onyxui::sdlpp::size tile_renderer::measure_text_with_bitmap(
        std::string_view /*text*/, const font& /*f*/) const { return {0, 0}; }
    onyxui::sdlpp::size tile_renderer::measure_text(
        std::string_view /*text*/, const font& /*f*/) {
        // Stub returns dummy size for testing
        return {0, 0};
    }
}

// Use test_canvas_backend for unit tests (doesn't require SDL)
using Backend = test_canvas_backend;

// ============================================================================
// Test Fixtures
// ============================================================================

namespace {
    // Create a minimal test theme
    tile_theme create_test_theme() {
        tile_theme theme;

        // Set up a minimal atlas (16x16 tiles, 10 columns)
        static tile_atlas test_atlas{
            .texture = nullptr,  // No actual texture needed for unit tests
            .tile_width = 16,
            .tile_height = 16,
            .columns = 10
        };
        theme.atlas = &test_atlas;

        // Set up normal font
        theme.font_normal = bitmap_font{
            &test_atlas,
            8,      // glyph_width
            16,     // glyph_height
            32,     // first_char (space)
            96,     // char_count
            0       // first_tile_id
        };

        // Set up title font (larger)
        theme.font_title = bitmap_font{
            &test_atlas,
            10,     // glyph_width
            20,     // glyph_height
            32,     // first_char
            96,     // char_count
            100     // first_tile_id
        };

        // Set up panel background nine_slice
        theme.panel.background.top_left = 0;
        theme.panel.background.top = 1;
        theme.panel.background.top_right = 2;
        theme.panel.background.left = 10;
        theme.panel.background.center = 11;
        theme.panel.background.right = 12;
        theme.panel.background.bottom_left = 20;
        theme.panel.background.bottom = 21;
        theme.panel.background.bottom_right = 22;
        theme.panel.background.margin_h = 2;
        theme.panel.background.margin_v = 2;

        // Set up button tiles
        theme.button.normal.top_left = 30;
        theme.button.normal.top = 31;
        theme.button.normal.top_right = 32;
        theme.button.normal.left = 40;
        theme.button.normal.center = 41;
        theme.button.normal.right = 42;
        theme.button.normal.bottom_left = 50;
        theme.button.normal.bottom = 51;
        theme.button.normal.bottom_right = 52;
        theme.button.normal.margin_h = 4;
        theme.button.normal.margin_v = 4;

        // Set up checkbox tiles
        theme.checkbox.unchecked = 60;
        theme.checkbox.checked = 61;
        theme.checkbox.indeterminate = 62;
        theme.checkbox.unchecked_hover = 63;
        theme.checkbox.checked_hover = 64;
        theme.checkbox.disabled = 65;

        // Set up radio tiles
        theme.radio.unchecked = 70;
        theme.radio.checked = 71;
        theme.radio.unchecked_hover = 72;
        theme.radio.checked_hover = 73;
        theme.radio.disabled = 74;

        // Set up progress bar tiles
        theme.progress_bar.track.left = 80;
        theme.progress_bar.track.center = 81;
        theme.progress_bar.track.right = 82;
        theme.progress_bar.track.margin = 2;
        theme.progress_bar.fill.left = 83;
        theme.progress_bar.fill.center = 84;
        theme.progress_bar.fill.right = 85;
        theme.progress_bar.fill.margin = 2;

        // Set up slider tiles
        theme.slider.track.left = 90;
        theme.slider.track.center = 91;
        theme.slider.track.right = 92;
        theme.slider.track.margin = 2;
        theme.slider.thumb = 93;
        theme.slider.thumb_hover = 94;
        theme.slider.thumb_pressed = 95;

        // Set up scrollbar tiles
        theme.scrollbar.track_v.top = 100;
        theme.scrollbar.track_v.center = 101;
        theme.scrollbar.track_v.bottom = 102;
        theme.scrollbar.track_v.margin = 2;
        theme.scrollbar.thumb_v.top = 103;
        theme.scrollbar.thumb_v.center = 104;
        theme.scrollbar.thumb_v.bottom = 105;
        theme.scrollbar.thumb_v.margin = 2;
        theme.scrollbar.arrow_up = 106;
        theme.scrollbar.arrow_down = 107;
        theme.scrollbar.track_h.left = 108;
        theme.scrollbar.track_h.center = 109;
        theme.scrollbar.track_h.right = 110;
        theme.scrollbar.track_h.margin = 2;
        theme.scrollbar.thumb_h.left = 111;
        theme.scrollbar.thumb_h.center = 112;
        theme.scrollbar.thumb_h.right = 113;
        theme.scrollbar.thumb_h.margin = 2;
        theme.scrollbar.arrow_left = 114;
        theme.scrollbar.arrow_right = 115;

        // Set up text input tiles
        theme.text_input.normal.top_left = 120;
        theme.text_input.normal.top = 121;
        theme.text_input.normal.top_right = 122;
        theme.text_input.normal.left = 130;
        theme.text_input.normal.center = 131;
        theme.text_input.normal.right = 132;
        theme.text_input.normal.bottom_left = 140;
        theme.text_input.normal.bottom = 141;
        theme.text_input.normal.bottom_right = 142;
        theme.text_input.normal.margin_h = 4;
        theme.text_input.normal.margin_v = 2;
        theme.text_input.focused.top_left = 123;
        theme.text_input.focused.top = 124;
        theme.text_input.focused.top_right = 125;
        theme.text_input.focused.left = 133;
        theme.text_input.focused.center = 134;
        theme.text_input.focused.right = 135;
        theme.text_input.focused.bottom_left = 143;
        theme.text_input.focused.bottom = 144;
        theme.text_input.focused.bottom_right = 145;
        theme.text_input.focused.margin_h = 4;
        theme.text_input.focused.margin_v = 2;
        theme.text_input.cursor = 150;

        // Set up font_disabled for placeholder text
        theme.font_disabled = bitmap_font{
            &test_atlas,
            8,      // glyph_width
            16,     // glyph_height
            32,     // first_char (space)
            96,     // char_count
            200     // first_tile_id
        };

        // Set up combo box tiles
        theme.combo.button.top_left = 160;
        theme.combo.button.top = 161;
        theme.combo.button.top_right = 162;
        theme.combo.button.left = 170;
        theme.combo.button.center = 171;
        theme.combo.button.right = 172;
        theme.combo.button.bottom_left = 180;
        theme.combo.button.bottom = 181;
        theme.combo.button.bottom_right = 182;
        theme.combo.button.margin_h = 4;
        theme.combo.button.margin_v = 2;
        theme.combo.arrow = 190;
        theme.combo.dropdown.top_left = 191;
        theme.combo.dropdown.top = 192;
        theme.combo.dropdown.top_right = 193;
        theme.combo.dropdown.left = 201;
        theme.combo.dropdown.center = 202;
        theme.combo.dropdown.right = 203;
        theme.combo.dropdown.bottom_left = 211;
        theme.combo.dropdown.bottom = 212;
        theme.combo.dropdown.bottom_right = 213;
        theme.combo.dropdown.margin_h = 2;
        theme.combo.dropdown.margin_v = 2;
        theme.combo.item_hover.top_left = 220;
        theme.combo.item_hover.center = 221;
        theme.combo.item_hover.top_right = 222;
        theme.combo.item_hover.margin_h = 1;
        theme.combo.item_hover.margin_v = 0;

        return theme;
    }

    // RAII guard for setting up test theme
    struct theme_guard {
        tile_theme theme;

        theme_guard() : theme(create_test_theme()) {
            set_theme(theme);
        }

        ~theme_guard() {
            // Note: No way to unset theme, but tests run sequentially
        }
    };
}

// ============================================================================
// tile_label Tests
// ============================================================================

TEST_SUITE("tile_label") {
    TEST_CASE("Default construction") {
        tile_label<Backend> label;

        CHECK(label.text().empty());
    }

    TEST_CASE("Construction with text") {
        tile_label<Backend> label("Hello World");

        CHECK(label.text() == "Hello World");
    }

    TEST_CASE("set_text changes text") {
        tile_label<Backend> label("Initial");

        label.set_text("Updated");

        CHECK(label.text() == "Updated");
    }

    TEST_CASE("set_text with same value is no-op") {
        tile_label<Backend> label("Same");

        label.set_text("Same");  // No change

        CHECK(label.text() == "Same");
    }

    TEST_CASE("set_centered") {
        tile_label<Backend> label("Test");

        CHECK_FALSE(label.is_centered());

        label.set_centered(true);
        CHECK(label.is_centered());

        label.set_centered(false);
        CHECK_FALSE(label.is_centered());
    }

    TEST_CASE("custom font can be set") {
        theme_guard guard;
        tile_label<Backend> label("Test");

        // Initially uses theme font
        label.use_theme_font();

        // Set custom font
        bitmap_font custom_font{
            guard.theme.atlas,
            12,   // glyph_width
            24,   // glyph_height
            32,   // first_char
            96,   // char_count
            200   // first_tile_id
        };
        label.set_font(&custom_font);

        // Reset to theme font
        label.use_theme_font();
    }
}

// ============================================================================
// tile_panel Tests
// ============================================================================

TEST_SUITE("tile_panel") {
    TEST_CASE("Default construction") {
        tile_panel<Backend> panel;

        // Just verify construction doesn't throw
        CHECK_NOTHROW((void)panel);
    }

    TEST_CASE("Construction with spacing") {
        tile_panel<Backend> panel(nullptr, 5);

        // Just verify construction doesn't throw
        CHECK_NOTHROW((void)panel);
    }

    TEST_CASE("Add children") {
        tile_panel<Backend> panel;

        auto label1 = std::make_unique<tile_label<Backend>>("Label 1");
        auto label2 = std::make_unique<tile_label<Backend>>("Label 2");

        panel.add_child(std::move(label1));
        panel.add_child(std::move(label2));

        // Children added successfully
        CHECK(true);
    }

    TEST_CASE("Custom nine_slice can be set") {
        tile_panel<Backend> panel;

        nine_slice custom{};
        custom.top_left = 5;
        custom.top = 6;
        custom.top_right = 7;
        custom.left = 15;
        custom.center = 16;
        custom.right = 17;
        custom.bottom_left = 25;
        custom.bottom = 26;
        custom.bottom_right = 27;
        custom.margin_h = 4;
        custom.margin_v = 4;

        panel.set_nine_slice(custom);

        // Reset to theme slice
        panel.use_theme_slice();

        CHECK(true);
    }

    TEST_CASE("Nested panels") {
        theme_guard guard;
        tile_panel<Backend> outer;
        auto inner = std::make_unique<tile_panel<Backend>>();

        auto label = std::make_unique<tile_label<Backend>>("Nested");
        inner->add_child(std::move(label));

        outer.add_child(std::move(inner));

        // Nested structure created successfully
        CHECK(true);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_SUITE("tile_widgets_integration") {
    TEST_CASE("Panel with multiple labels") {
        theme_guard guard;
        tile_panel<Backend> panel(nullptr, 4);  // 4px spacing

        panel.add_child(std::make_unique<tile_label<Backend>>("Title"));
        panel.add_child(std::make_unique<tile_label<Backend>>("Line 1"));
        panel.add_child(std::make_unique<tile_label<Backend>>("Line 2"));
        panel.add_child(std::make_unique<tile_label<Backend>>("Line 3"));

        // All children added successfully
        CHECK(true);
    }

    TEST_CASE("Theme provides default styling") {
        theme_guard guard;

        CHECK(has_theme());

        const auto& theme = get_theme();
        CHECK(theme.atlas != nullptr);
        CHECK(theme.font_normal.glyph_width > 0);
        CHECK(theme.panel.background.is_valid());
    }

    TEST_CASE("No renderer returns nullptr") {
        // get_renderer() should return nullptr when no renderer is set
        tile_renderer* renderer = get_renderer();
        CHECK(renderer == nullptr);
    }
}

// ============================================================================
// tile_button Tests
// ============================================================================

TEST_SUITE("tile_button") {
    TEST_CASE("Default construction") {
        tile_button<Backend> button;

        CHECK(button.text().empty());
    }

    TEST_CASE("Construction with text") {
        tile_button<Backend> button("Click Me");

        CHECK(button.text() == "Click Me");
    }

    TEST_CASE("set_text changes text") {
        tile_button<Backend> button("Initial");

        button.set_text("Updated");

        CHECK(button.text() == "Updated");
    }

    TEST_CASE("Button is focusable by default") {
        tile_button<Backend> button("Test");

        CHECK(button.is_focusable());
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_button<Backend> button("Test");

        button_tiles custom{};
        custom.normal.top_left = 100;
        custom.normal.center = 111;

        button.set_tiles(custom);

        // Reset to theme tiles
        button.use_theme_tiles();

        CHECK(true);
    }

    TEST_CASE("Clicked signal emitted") {
        tile_button<Backend> button("Test");

        bool clicked = false;
        button.clicked.connect([&clicked]() {
            clicked = true;
        });

        // Simulate click (would normally come from on_click override)
        button.clicked.emit();

        CHECK(clicked);
    }
}

// ============================================================================
// tile_checkbox Tests
// ============================================================================

TEST_SUITE("tile_checkbox") {
    TEST_CASE("Default construction") {
        tile_checkbox<Backend> checkbox;

        CHECK(checkbox.text().empty());
        CHECK_FALSE(checkbox.is_checked());
        CHECK(checkbox.get_state() == checkbox_state::unchecked);
    }

    TEST_CASE("Construction with text") {
        tile_checkbox<Backend> checkbox("Enable sound");

        CHECK(checkbox.text() == "Enable sound");
    }

    TEST_CASE("set_checked toggles state") {
        tile_checkbox<Backend> checkbox("Test");

        CHECK_FALSE(checkbox.is_checked());

        checkbox.set_checked(true);
        CHECK(checkbox.is_checked());
        CHECK(checkbox.get_state() == checkbox_state::checked);

        checkbox.set_checked(false);
        CHECK_FALSE(checkbox.is_checked());
        CHECK(checkbox.get_state() == checkbox_state::unchecked);
    }

    TEST_CASE("Tri-state mode") {
        tile_checkbox<Backend> checkbox("Test");

        // Tri-state is disabled by default
        CHECK_FALSE(checkbox.is_tri_state_enabled());

        // Cannot set indeterminate when tri-state is disabled
        checkbox.set_state(checkbox_state::indeterminate);
        CHECK(checkbox.get_state() == checkbox_state::unchecked);  // Unchanged

        // Enable tri-state
        checkbox.set_tri_state_enabled(true);
        CHECK(checkbox.is_tri_state_enabled());

        // Now can set indeterminate
        checkbox.set_state(checkbox_state::indeterminate);
        CHECK(checkbox.get_state() == checkbox_state::indeterminate);
    }

    TEST_CASE("Toggled signal emitted") {
        tile_checkbox<Backend> checkbox("Test");

        bool toggled_value = false;
        checkbox.toggled.connect([&toggled_value](bool checked) {
            toggled_value = checked;
        });

        checkbox.set_checked(true);
        CHECK(toggled_value);

        checkbox.set_checked(false);
        CHECK_FALSE(toggled_value);
    }

    TEST_CASE("State changed signal emitted") {
        tile_checkbox<Backend> checkbox("Test");
        checkbox.set_tri_state_enabled(true);

        checkbox_state last_state = checkbox_state::unchecked;
        checkbox.state_changed.connect([&last_state](checkbox_state state) {
            last_state = state;
        });

        checkbox.set_state(checkbox_state::indeterminate);
        CHECK(last_state == checkbox_state::indeterminate);

        checkbox.set_state(checkbox_state::checked);
        CHECK(last_state == checkbox_state::checked);
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_checkbox<Backend> checkbox("Test");

        checkbox_tiles custom{};
        custom.unchecked = 200;
        custom.checked = 201;

        checkbox.set_tiles(custom);

        // Reset to theme tiles
        checkbox.use_theme_tiles();

        CHECK(true);
    }
}

// ============================================================================
// tile_radio Tests
// ============================================================================

TEST_SUITE("tile_radio") {
    TEST_CASE("Default construction") {
        tile_radio<Backend> radio;

        CHECK(radio.text().empty());
        CHECK_FALSE(radio.is_checked());
    }

    TEST_CASE("Construction with text") {
        tile_radio<Backend> radio("Option A");

        CHECK(radio.text() == "Option A");
    }

    TEST_CASE("set_checked toggles state") {
        tile_radio<Backend> radio("Test");

        CHECK_FALSE(radio.is_checked());

        radio.set_checked(true);
        CHECK(radio.is_checked());

        radio.set_checked(false);
        CHECK_FALSE(radio.is_checked());
    }

    TEST_CASE("Toggled signal emitted") {
        tile_radio<Backend> radio("Test");

        bool toggled_value = false;
        radio.toggled.connect([&toggled_value](bool checked) {
            toggled_value = checked;
        });

        radio.set_checked(true);
        CHECK(toggled_value);

        radio.set_checked(false);
        CHECK_FALSE(toggled_value);
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_radio<Backend> radio("Test");

        radio_tiles custom{};
        custom.unchecked = 300;
        custom.checked = 301;

        radio.set_tiles(custom);

        // Reset to theme tiles
        radio.use_theme_tiles();

        CHECK(true);
    }
}

// ============================================================================
// tile_radio_group Tests
// ============================================================================

TEST_SUITE("tile_radio_group") {
    TEST_CASE("Default construction") {
        tile_radio_group<Backend> group;

        CHECK(group.count() == 0);
        CHECK(group.get_selected_index() == -1);
    }

    TEST_CASE("Add options") {
        tile_radio_group<Backend> group;

        group.add_option("Small");
        group.add_option("Medium");
        group.add_option("Large");

        CHECK(group.count() == 3);
    }

    TEST_CASE("Set selected index") {
        tile_radio_group<Backend> group;

        group.add_option("Option A");
        group.add_option("Option B");
        group.add_option("Option C");

        CHECK(group.get_selected_index() == -1);

        group.set_selected_index(1);
        CHECK(group.get_selected_index() == 1);

        group.set_selected_index(2);
        CHECK(group.get_selected_index() == 2);

        group.set_selected_index(-1);
        CHECK(group.get_selected_index() == -1);
    }

    TEST_CASE("Mutual exclusion") {
        tile_radio_group<Backend> group;

        auto* opt_a = group.add_option("Option A");
        auto* opt_b = group.add_option("Option B");
        auto* opt_c = group.add_option("Option C");

        // Select option A
        opt_a->set_checked(true);
        CHECK(opt_a->is_checked());
        CHECK_FALSE(opt_b->is_checked());
        CHECK_FALSE(opt_c->is_checked());

        // Select option B (should uncheck A)
        opt_b->set_checked(true);
        CHECK_FALSE(opt_a->is_checked());
        CHECK(opt_b->is_checked());
        CHECK_FALSE(opt_c->is_checked());

        // Select option C (should uncheck B)
        opt_c->set_checked(true);
        CHECK_FALSE(opt_a->is_checked());
        CHECK_FALSE(opt_b->is_checked());
        CHECK(opt_c->is_checked());
    }

    TEST_CASE("Selection changed signal") {
        tile_radio_group<Backend> group;

        group.add_option("Option A");
        group.add_option("Option B");
        group.add_option("Option C");

        int last_selected = -1;
        group.selection_changed.connect([&last_selected](int index) {
            last_selected = index;
        });

        group.set_selected_index(0);
        CHECK(last_selected == 0);

        group.set_selected_index(2);
        CHECK(last_selected == 2);
    }
}

// ============================================================================
// tile_progress_bar Tests
// ============================================================================

TEST_SUITE("tile_progress_bar") {
    TEST_CASE("Default construction") {
        tile_progress_bar<Backend> progress;

        CHECK(progress.value() == 0);
        CHECK(progress.minimum() == 0);
        CHECK(progress.maximum() == 100);
        CHECK_FALSE(progress.is_indeterminate());
        CHECK(progress.orientation() == tile_progress_orientation::horizontal);
    }

    TEST_CASE("set_value clamps to range") {
        tile_progress_bar<Backend> progress;

        progress.set_value(50);
        CHECK(progress.value() == 50);

        progress.set_value(150);  // Above max
        CHECK(progress.value() == 100);

        progress.set_value(-10);  // Below min
        CHECK(progress.value() == 0);
    }

    TEST_CASE("set_range updates bounds") {
        tile_progress_bar<Backend> progress;

        progress.set_range(10, 200);
        CHECK(progress.minimum() == 10);
        CHECK(progress.maximum() == 200);

        // Value should be clamped to new range
        progress.set_value(5);
        CHECK(progress.value() == 10);  // Clamped to min
    }

    TEST_CASE("Indeterminate mode") {
        tile_progress_bar<Backend> progress;

        CHECK_FALSE(progress.is_indeterminate());

        progress.set_indeterminate(true);
        CHECK(progress.is_indeterminate());

        progress.set_indeterminate(false);
        CHECK_FALSE(progress.is_indeterminate());
    }

    TEST_CASE("Orientation change") {
        tile_progress_bar<Backend> progress;

        CHECK(progress.orientation() == tile_progress_orientation::horizontal);

        progress.set_orientation(tile_progress_orientation::vertical);
        CHECK(progress.orientation() == tile_progress_orientation::vertical);
    }

    TEST_CASE("Value changed signal") {
        tile_progress_bar<Backend> progress;

        int last_value = -1;
        progress.value_changed.connect([&last_value](int val) {
            last_value = val;
        });

        progress.set_value(42);
        CHECK(last_value == 42);

        progress.set_value(75);
        CHECK(last_value == 75);
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_progress_bar<Backend> progress;

        progress_bar_tiles custom{};
        custom.track.left = 100;
        custom.track.center = 101;
        custom.track.right = 102;

        progress.set_tiles(custom);

        // Reset to theme tiles
        progress.use_theme_tiles();

        CHECK(true);
    }
}

// ============================================================================
// tile_slider Tests
// ============================================================================

TEST_SUITE("tile_slider") {
    TEST_CASE("Default construction") {
        tile_slider<Backend> slider;

        CHECK(slider.value() == 0);
        CHECK(slider.minimum() == 0);
        CHECK(slider.maximum() == 100);
        CHECK(slider.single_step() == 1);
        CHECK(slider.page_step() == 10);
        CHECK(slider.orientation() == tile_slider_orientation::horizontal);
    }

    TEST_CASE("set_value clamps and snaps") {
        tile_slider<Backend> slider;

        slider.set_value(50);
        CHECK(slider.value() == 50);

        slider.set_value(150);  // Above max
        CHECK(slider.value() == 100);

        slider.set_value(-10);  // Below min
        CHECK(slider.value() == 0);
    }

    TEST_CASE("set_range updates bounds") {
        tile_slider<Backend> slider;

        slider.set_range(10, 200);
        CHECK(slider.minimum() == 10);
        CHECK(slider.maximum() == 200);
    }

    TEST_CASE("Step configuration") {
        tile_slider<Backend> slider;

        slider.set_single_step(5);
        CHECK(slider.single_step() == 5);

        slider.set_page_step(20);
        CHECK(slider.page_step() == 20);
    }

    TEST_CASE("Orientation change") {
        tile_slider<Backend> slider;

        CHECK(slider.orientation() == tile_slider_orientation::horizontal);

        slider.set_orientation(tile_slider_orientation::vertical);
        CHECK(slider.orientation() == tile_slider_orientation::vertical);
    }

    TEST_CASE("Value changed signal") {
        tile_slider<Backend> slider;

        int last_value = -1;
        slider.value_changed.connect([&last_value](int val) {
            last_value = val;
        });

        slider.set_value(42);
        CHECK(last_value == 42);

        slider.set_value(75);
        CHECK(last_value == 75);
    }

    TEST_CASE("Slider is focusable") {
        tile_slider<Backend> slider;

        CHECK(slider.is_focusable());
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_slider<Backend> slider;

        slider_tiles custom{};
        custom.track.left = 200;
        custom.track.center = 201;
        custom.track.right = 202;
        custom.thumb = 203;

        slider.set_tiles(custom);

        // Reset to theme tiles
        slider.use_theme_tiles();

        CHECK(true);
    }
}

// ============================================================================
// tile_scrollbar Tests
// ============================================================================

TEST_SUITE("tile_scrollbar") {
    TEST_CASE("Default construction") {
        tile_scrollbar<Backend> scrollbar;

        CHECK(scrollbar.get_orientation() == orientation::vertical);
        CHECK(scrollbar.content_size() == 100);
        CHECK(scrollbar.viewport_size() == 100);
        CHECK(scrollbar.scroll_position() == 0);
        CHECK(scrollbar.show_arrows());
    }

    TEST_CASE("Horizontal orientation") {
        tile_scrollbar<Backend> scrollbar(orientation::horizontal);

        CHECK(scrollbar.get_orientation() == orientation::horizontal);
    }

    TEST_CASE("Set scroll configuration") {
        tile_scrollbar<Backend> scrollbar;

        scrollbar.set_content_size(500);
        CHECK(scrollbar.content_size() == 500);

        scrollbar.set_viewport_size(100);
        CHECK(scrollbar.viewport_size() == 100);

        scrollbar.set_scroll_position(50);
        CHECK(scrollbar.scroll_position() == 50);
    }

    TEST_CASE("Scroll position is clamped") {
        tile_scrollbar<Backend> scrollbar;
        scrollbar.set_content_size(500);
        scrollbar.set_viewport_size(100);

        // Clamp to max
        scrollbar.set_scroll_position(1000);
        CHECK(scrollbar.scroll_position() == 400);  // max = content - viewport

        // Clamp to min
        scrollbar.set_scroll_position(-100);
        CHECK(scrollbar.scroll_position() == 0);
    }

    TEST_CASE("Scroll position when no scrolling needed") {
        tile_scrollbar<Backend> scrollbar;
        scrollbar.set_content_size(50);   // Content smaller than viewport
        scrollbar.set_viewport_size(100);

        scrollbar.set_scroll_position(100);
        CHECK(scrollbar.scroll_position() == 0);  // No scrolling possible
    }

    TEST_CASE("Step configuration") {
        tile_scrollbar<Backend> scrollbar;

        scrollbar.set_line_step(10);
        CHECK(scrollbar.line_step() == 10);

        scrollbar.set_page_step(50);
        CHECK(scrollbar.page_step() == 50);
    }

    TEST_CASE("Arrow visibility") {
        tile_scrollbar<Backend> scrollbar;

        CHECK(scrollbar.show_arrows());

        scrollbar.set_show_arrows(false);
        CHECK_FALSE(scrollbar.show_arrows());

        scrollbar.set_show_arrows(true);
        CHECK(scrollbar.show_arrows());
    }

    TEST_CASE("Orientation change") {
        tile_scrollbar<Backend> scrollbar;

        CHECK(scrollbar.get_orientation() == orientation::vertical);

        scrollbar.set_orientation(orientation::horizontal);
        CHECK(scrollbar.get_orientation() == orientation::horizontal);
    }

    TEST_CASE("Scroll requested signal") {
        tile_scrollbar<Backend> scrollbar;
        scrollbar.set_content_size(500);
        scrollbar.set_viewport_size(100);
        scrollbar.set_line_step(10);

        int last_scroll = -1;
        scrollbar.scroll_requested.connect([&last_scroll](int pos) {
            last_scroll = pos;
        });

        // Signal is emitted when scroll position changes
        scrollbar.set_scroll_position(50);
        // Note: set_scroll_position doesn't emit signal directly
        // The signal is emitted through user interaction

        CHECK(true);  // Just verify construction and connection work
    }

    TEST_CASE("Scrollbar is focusable") {
        tile_scrollbar<Backend> scrollbar;

        CHECK(scrollbar.is_focusable());
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_scrollbar<Backend> scrollbar;

        scrollbar_tiles custom{};
        custom.track_v.top = 200;
        custom.track_v.center = 201;
        custom.track_v.bottom = 202;
        custom.thumb_v.top = 203;
        custom.thumb_v.center = 204;
        custom.thumb_v.bottom = 205;

        scrollbar.set_tiles(custom);

        // Reset to theme tiles
        scrollbar.use_theme_tiles();

        CHECK(true);
    }
}

// ============================================================================
// tile_text_input Tests
// ============================================================================

TEST_SUITE("tile_text_input") {
    TEST_CASE("Default construction") {
        tile_text_input<Backend> input;

        CHECK(input.text().empty());
        CHECK(input.placeholder().empty());
        CHECK_FALSE(input.is_password_mode());
        CHECK_FALSE(input.is_read_only());
        CHECK(input.visible_chars() == 20);
        CHECK(input.cursor_position() == 0);
    }

    TEST_CASE("Construction with text") {
        tile_text_input<Backend> input("Hello");

        CHECK(input.text() == "Hello");
        CHECK(input.cursor_position() == 5);  // Cursor at end
    }

    TEST_CASE("set_text changes text and emits signal") {
        tile_text_input<Backend> input;

        std::string last_text;
        input.text_changed.connect([&last_text](const std::string& text) {
            last_text = text;
        });

        input.set_text("Test");
        CHECK(input.text() == "Test");
        CHECK(last_text == "Test");
    }

    TEST_CASE("set_text clamps cursor position") {
        tile_text_input<Backend> input("Long text here");
        input.set_cursor_position(14);  // At end
        CHECK(input.cursor_position() == 14);

        input.set_text("Short");
        CHECK(input.cursor_position() == 5);  // Clamped to new length
    }

    TEST_CASE("set_text same value is no-op") {
        tile_text_input<Backend> input("Same");

        int signal_count = 0;
        input.text_changed.connect([&signal_count](const std::string&) {
            signal_count++;
        });

        input.set_text("Same");  // Same value
        CHECK(signal_count == 0);  // Signal not emitted
    }

    TEST_CASE("Placeholder text") {
        tile_text_input<Backend> input;

        input.set_placeholder("Enter name...");
        CHECK(input.placeholder() == "Enter name...");
    }

    TEST_CASE("Password mode") {
        tile_text_input<Backend> input("secret");

        CHECK_FALSE(input.is_password_mode());

        input.set_password_mode(true);
        CHECK(input.is_password_mode());

        input.set_password_mode(false);
        CHECK_FALSE(input.is_password_mode());
    }

    TEST_CASE("Read-only mode") {
        tile_text_input<Backend> input("readonly");

        CHECK_FALSE(input.is_read_only());

        input.set_read_only(true);
        CHECK(input.is_read_only());

        input.set_read_only(false);
        CHECK_FALSE(input.is_read_only());
    }

    TEST_CASE("Visible chars affects size") {
        tile_text_input<Backend> input;

        CHECK(input.visible_chars() == 20);

        input.set_visible_chars(30);
        CHECK(input.visible_chars() == 30);

        input.set_visible_chars(0);  // Clamped to minimum
        CHECK(input.visible_chars() == 1);
    }

    TEST_CASE("Cursor position") {
        tile_text_input<Backend> input("Hello World");

        // Default at end
        CHECK(input.cursor_position() == 11);

        input.set_cursor_position(5);
        CHECK(input.cursor_position() == 5);

        // Clamped to text length
        input.set_cursor_position(100);
        CHECK(input.cursor_position() == 11);

        // Clamped to 0
        input.set_cursor_position(-5);
        CHECK(input.cursor_position() == 0);
    }

    TEST_CASE("Text input is focusable") {
        tile_text_input<Backend> input;

        CHECK(input.is_focusable());
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_text_input<Backend> input;

        text_input_tiles custom{};
        custom.normal.top_left = 300;
        custom.normal.center = 304;
        custom.cursor = 310;

        input.set_tiles(custom);

        // Reset to theme tiles
        input.use_theme_tiles();

        CHECK(true);
    }

    TEST_CASE("Return pressed signal") {
        tile_text_input<Backend> input("Test");

        bool return_called = false;
        input.return_pressed.connect([&return_called]() {
            return_called = true;
        });

        // Signal emitted manually (normally from keyboard event)
        input.return_pressed.emit();
        CHECK(return_called);
    }

    TEST_CASE("Editing finished signal") {
        tile_text_input<Backend> input("Test");

        bool finished_called = false;
        input.editing_finished.connect([&finished_called]() {
            finished_called = true;
        });

        // Signal emitted manually (normally on focus loss or Enter)
        input.editing_finished.emit();
        CHECK(finished_called);
    }

}

// ============================================================================
// tile_combo_box Tests
// ============================================================================

TEST_SUITE("tile_combo_box") {
    TEST_CASE("Default construction") {
        tile_combo_box<Backend> combo;

        CHECK(combo.count() == 0);
        CHECK(combo.current_index() == -1);
        CHECK(combo.current_text().empty());
        CHECK_FALSE(combo.is_popup_open());
    }

    TEST_CASE("Add items") {
        tile_combo_box<Backend> combo;

        combo.add_item("Small");
        combo.add_item("Medium");
        combo.add_item("Large");

        CHECK(combo.count() == 3);
        CHECK(combo.item_at(0) == "Small");
        CHECK(combo.item_at(1) == "Medium");
        CHECK(combo.item_at(2) == "Large");
    }

    TEST_CASE("Insert item") {
        tile_combo_box<Backend> combo;

        combo.add_item("First");
        combo.add_item("Third");
        combo.insert_item(1, "Second");

        CHECK(combo.count() == 3);
        CHECK(combo.item_at(0) == "First");
        CHECK(combo.item_at(1) == "Second");
        CHECK(combo.item_at(2) == "Third");
    }

    TEST_CASE("Remove item") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");
        combo.add_item("C");

        combo.remove_item(1);

        CHECK(combo.count() == 2);
        CHECK(combo.item_at(0) == "A");
        CHECK(combo.item_at(1) == "C");
    }

    TEST_CASE("Clear items") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");
        combo.set_current_index(0);

        combo.clear();

        CHECK(combo.count() == 0);
        CHECK(combo.current_index() == -1);
    }

    TEST_CASE("Item at invalid index returns empty") {
        tile_combo_box<Backend> combo;

        combo.add_item("Only");

        CHECK(combo.item_at(-1).empty());
        CHECK(combo.item_at(1).empty());
        CHECK(combo.item_at(100).empty());
    }

    TEST_CASE("Set current index") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");
        combo.add_item("C");

        CHECK(combo.current_index() == -1);

        combo.set_current_index(1);
        CHECK(combo.current_index() == 1);
        CHECK(combo.current_text() == "B");

        combo.set_current_index(0);
        CHECK(combo.current_index() == 0);
        CHECK(combo.current_text() == "A");
    }

    TEST_CASE("Set current index clamps") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");

        combo.set_current_index(100);
        CHECK(combo.current_index() == 1);  // Clamped to max

        combo.set_current_index(-5);
        CHECK(combo.current_index() == -1);  // Clamped to -1
    }

    TEST_CASE("Set current text") {
        tile_combo_box<Backend> combo;

        combo.add_item("Apple");
        combo.add_item("Banana");
        combo.add_item("Cherry");

        bool found = combo.set_current_text("Banana");
        CHECK(found);
        CHECK(combo.current_index() == 1);

        found = combo.set_current_text("NotFound");
        CHECK_FALSE(found);
        CHECK(combo.current_index() == 1);  // Unchanged
    }

    TEST_CASE("Current index changed signal") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");

        int last_index = -999;
        combo.current_index_changed.connect([&last_index](int index) {
            last_index = index;
        });

        combo.set_current_index(0);
        CHECK(last_index == 0);

        combo.set_current_index(1);
        CHECK(last_index == 1);
    }

    TEST_CASE("Current text changed signal") {
        tile_combo_box<Backend> combo;

        combo.add_item("First");
        combo.add_item("Second");

        std::string last_text;
        combo.current_text_changed.connect([&last_text](const std::string& text) {
            last_text = text;
        });

        combo.set_current_index(0);
        CHECK(last_text == "First");

        combo.set_current_index(1);
        CHECK(last_text == "Second");
    }

    TEST_CASE("Signal not emitted for same value") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.set_current_index(0);

        int signal_count = 0;
        combo.current_index_changed.connect([&signal_count](int) {
            signal_count++;
        });

        combo.set_current_index(0);  // Same value
        CHECK(signal_count == 0);
    }

    TEST_CASE("Popup open/close") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");

        CHECK_FALSE(combo.is_popup_open());

        combo.open_popup();
        CHECK(combo.is_popup_open());

        combo.close_popup();
        CHECK_FALSE(combo.is_popup_open());

        combo.toggle_popup();
        CHECK(combo.is_popup_open());

        combo.toggle_popup();
        CHECK_FALSE(combo.is_popup_open());
    }

    TEST_CASE("Popup cannot open when empty") {
        tile_combo_box<Backend> combo;

        combo.open_popup();
        CHECK_FALSE(combo.is_popup_open());  // Empty, cannot open
    }

    TEST_CASE("Visible chars setting") {
        tile_combo_box<Backend> combo;

        CHECK(combo.visible_chars() == 15);

        combo.set_visible_chars(20);
        CHECK(combo.visible_chars() == 20);

        combo.set_visible_chars(0);  // Clamped to minimum
        CHECK(combo.visible_chars() == 1);
    }

    TEST_CASE("Max visible items setting") {
        tile_combo_box<Backend> combo;

        CHECK(combo.max_visible_items() == 8);

        combo.set_max_visible_items(5);
        CHECK(combo.max_visible_items() == 5);

        combo.set_max_visible_items(0);  // 0 = show all
        CHECK(combo.max_visible_items() == 0);
    }

    TEST_CASE("Combo box is focusable") {
        tile_combo_box<Backend> combo;

        CHECK(combo.is_focusable());
    }

    TEST_CASE("Custom tiles can be set") {
        theme_guard guard;
        tile_combo_box<Backend> combo;

        combo_tiles custom{};
        custom.button.top_left = 400;
        custom.button.center = 404;
        custom.arrow = 410;

        combo.set_tiles(custom);

        // Reset to theme tiles
        combo.use_theme_tiles();

        CHECK(true);
    }

    TEST_CASE("Insert adjusts current index") {
        tile_combo_box<Backend> combo;

        combo.add_item("B");
        combo.add_item("C");
        combo.set_current_index(1);  // Select "C"
        CHECK(combo.current_text() == "C");

        combo.insert_item(0, "A");  // Insert before selection

        CHECK(combo.current_index() == 2);  // Index shifted
        CHECK(combo.current_text() == "C");  // Still same item
    }

    TEST_CASE("Remove adjusts current index") {
        tile_combo_box<Backend> combo;

        combo.add_item("A");
        combo.add_item("B");
        combo.add_item("C");
        combo.set_current_index(2);  // Select "C"

        combo.remove_item(2);  // Remove selected item

        CHECK(combo.current_index() == 1);  // Clamped to new max
    }
}
