/**
 * @file strategy_ui_demo.cc
 * @brief Strategy game UI demo implementing design.txt layout
 *
 * Layout from design.txt:
 * ┌────────────────────────────────────────────────────────────────────────────────┐
 * │ Menu                                                                           │
 * ┌──────────────────────────────────────────────────────────────┐┌────────────────┐
 * │                                                              ││                │
 * │          Strategic Map                                       ││  Minimap       │
 * │                                                              ││                │
 * └──────────────────────────────────────────────────────────────┘└────────────────┘
 * ┌────────────────────────────────────────────────────────────────────────────────┐
 * │                 Toolbox                                                        │
 * └────────────────────────────────────────────────────────────────────────────────┘
 *
 * Key features:
 * - Small gaps between map, minimap and toolbox
 * - Map takes flexible width, minimap has fixed width
 * - Toolbox at bottom spans full width
 * - Self-contained procedural atlas generator using sdlpp::surface
 */

#include <onyxui/tile/tile_backend.hh>
#include <onyxui/tile/tile_theme.hh>
#include <onyxui/tile/tile_factory.hh>
#include <onyxui/tile/widgets/tile_widgets.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>
#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/core/element.hh>
#include <onyxui/layout/layout_strategy.hh>

#include <sdlpp/video/surface.hh>
#include <sdlpp/video/texture.hh>
#include <sdlpp/video/renderer.hh>
#include <onyxui/theming/theme.hh>
#include <onyxui/services/ui_services.hh>

#include <iostream>
#include <memory>
#include <array>
#include <cstdint>

using namespace onyxui;
using namespace onyxui::tile;

// ============================================================================
// Constants
// ============================================================================

namespace {
    // UI sizes (in pixels, 1:1 scaling for tile backend)
    constexpr int GAP_SIZE = 2;           // Small gap between panels
    constexpr int MENU_HEIGHT = 16;       // Menu bar height (fits 8px bitmap font + padding)
    constexpr int TOOLBOX_HEIGHT = 64;    // Toolbox height
    constexpr int MINIMAP_WIDTH = 160;    // Minimap panel width

    // Atlas constants
    constexpr int TILE_SIZE = 8;
    constexpr int ATLAS_COLUMNS = 16;
    constexpr int ATLAS_ROWS = 16;
    constexpr int ATLAS_WIDTH = TILE_SIZE * ATLAS_COLUMNS;   // 128
    constexpr int ATLAS_HEIGHT = TILE_SIZE * ATLAS_ROWS;     // 128

    // Nine-slice tile indices
    constexpr int PANEL_LIGHT_START = 0;
    constexpr int PANEL_DARK_START = 16;
    constexpr int BUTTON_NORMAL_START = 32;

    // Font tile indices (row 6+)
    constexpr int FONT_START = 96;
    constexpr int FONT_FIRST_CHAR = 32;
    constexpr int FONT_CHAR_COUNT = 96;
}

// ============================================================================
// Simple Procedural Atlas Generator
// ============================================================================

namespace {

// Simple 5x7 bitmap font data (ASCII 32-127)
// Each character is 5 bits wide, 7 rows tall
constexpr uint8_t FONT_DATA[96][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x04,0x04,0x04,0x04,0x00,0x04,0x00}, // !
    {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00}, // "
    {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00}, // #
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04}, // $
    {0x18,0x19,0x02,0x04,0x08,0x13,0x03}, // %
    {0x08,0x14,0x14,0x08,0x15,0x12,0x0D}, // &
    {0x04,0x04,0x00,0x00,0x00,0x00,0x00}, // '
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02}, // (
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08}, // )
    {0x00,0x04,0x15,0x0E,0x15,0x04,0x00}, // *
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}, // +
    {0x00,0x00,0x00,0x00,0x04,0x04,0x08}, // ,
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x04,0x00}, // .
    {0x01,0x02,0x02,0x04,0x08,0x08,0x10}, // /
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 2
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
    {0x00,0x04,0x00,0x00,0x04,0x00,0x00}, // :
    {0x00,0x04,0x00,0x00,0x04,0x04,0x08}, // ;
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02}, // <
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}, // =
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08}, // >
    {0x0E,0x11,0x01,0x06,0x04,0x00,0x04}, // ?
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E}, // @
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}, // V
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}, // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}, // [
    {0x10,0x08,0x08,0x04,0x02,0x02,0x01}, // backslash
    {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}, // ]
    {0x04,0x0A,0x11,0x00,0x00,0x00,0x00}, // ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F}, // _
    {0x08,0x04,0x00,0x00,0x00,0x00,0x00}, // `
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}, // a
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E}, // b
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E}, // c
    {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F}, // d
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}, // e
    {0x06,0x08,0x1C,0x08,0x08,0x08,0x08}, // f
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E}, // g
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x11}, // h
    {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}, // i
    {0x02,0x00,0x06,0x02,0x02,0x12,0x0C}, // j
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12}, // k
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}, // l
    {0x00,0x00,0x1A,0x15,0x15,0x11,0x11}, // m
    {0x00,0x00,0x1E,0x11,0x11,0x11,0x11}, // n
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}, // o
    {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}, // p
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01}, // q
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10}, // r
    {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E}, // s
    {0x08,0x08,0x1C,0x08,0x08,0x09,0x06}, // t
    {0x00,0x00,0x11,0x11,0x11,0x13,0x0D}, // u
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04}, // v
    {0x00,0x00,0x11,0x11,0x15,0x15,0x0A}, // w
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}, // x
    {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E}, // y
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F}, // z
    {0x02,0x04,0x04,0x08,0x04,0x04,0x02}, // {
    {0x04,0x04,0x04,0x04,0x04,0x04,0x04}, // |
    {0x08,0x04,0x04,0x02,0x04,0x04,0x08}, // }
    {0x00,0x00,0x08,0x15,0x02,0x00,0x00}, // ~
    {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}, // DEL (block)
};

/**
 * @brief Draw a nine-slice set to the atlas surface
 */
void draw_nine_slice(
    ::sdlpp::surface& surf,
    int start_tile,
    const ::sdlpp::color& bg,
    const ::sdlpp::color& border,
    const ::sdlpp::color& highlight,
    const ::sdlpp::color& shadow)
{
    // Nine-slice layout: TL, T, TR, L, C, R, BL, B, BR
    for (int slice = 0; slice < 9; ++slice) {
        int tile_x = ((start_tile + slice) % ATLAS_COLUMNS) * TILE_SIZE;
        int tile_y = ((start_tile + slice) / ATLAS_COLUMNS) * TILE_SIZE;

        // Fill with background
        ::sdlpp::rect<int> tile_rect{tile_x, tile_y, TILE_SIZE, TILE_SIZE};
        surf.fill_rect(tile_rect, bg);

        // Draw borders based on slice position
        bool top_edge = (slice < 3);
        bool bottom_edge = (slice >= 6);
        bool left_edge = (slice % 3 == 0);
        bool right_edge = (slice % 3 == 2);

        // Draw highlight (top and left edges)
        if (top_edge) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                surf.put_pixel(tile_x + x, tile_y, highlight);
            }
        }
        if (left_edge) {
            for (int y = 0; y < TILE_SIZE; ++y) {
                surf.put_pixel(tile_x, tile_y + y, highlight);
            }
        }

        // Draw shadow (bottom and right edges)
        if (bottom_edge) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                surf.put_pixel(tile_x + x, tile_y + TILE_SIZE - 1, shadow);
            }
        }
        if (right_edge) {
            for (int y = 0; y < TILE_SIZE; ++y) {
                surf.put_pixel(tile_x + TILE_SIZE - 1, tile_y + y, shadow);
            }
        }

        // Draw border pixels at corners
        if (top_edge || left_edge || bottom_edge || right_edge) {
            if (top_edge && left_edge) {
                surf.put_pixel(tile_x, tile_y, border);
            }
            if (top_edge && right_edge) {
                surf.put_pixel(tile_x + TILE_SIZE - 1, tile_y, border);
            }
            if (bottom_edge && left_edge) {
                surf.put_pixel(tile_x, tile_y + TILE_SIZE - 1, border);
            }
            if (bottom_edge && right_edge) {
                surf.put_pixel(tile_x + TILE_SIZE - 1, tile_y + TILE_SIZE - 1, border);
            }
        }
    }
}

/**
 * @brief Draw font glyphs to the atlas surface
 */
void draw_font_glyphs(::sdlpp::surface& surf, const ::sdlpp::color& text_color)
{
    for (int ch = 0; ch < FONT_CHAR_COUNT; ++ch) {
        int tile_idx = FONT_START + ch;
        int tile_x = (tile_idx % ATLAS_COLUMNS) * TILE_SIZE;
        int tile_y = (tile_idx / ATLAS_COLUMNS) * TILE_SIZE;

        // Clear tile to transparent
        ::sdlpp::rect<int> tile_rect{tile_x, tile_y, TILE_SIZE, TILE_SIZE};
        surf.fill_rect(tile_rect, {0, 0, 0, 0});

        // Draw the glyph (5x7 centered in 8x8)
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = FONT_DATA[ch][row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (0x10 >> col)) {  // MSB first
                    int px = tile_x + 1 + col;  // Offset by 1 to center
                    int py = tile_y + row;
                    surf.put_pixel(px, py, text_color);
                }
            }
        }
    }
}

/**
 * @brief Generate a procedural tile atlas
 * @param renderer SDL renderer to create texture
 * @return Generated texture, or nullopt on failure
 */
std::optional<::sdlpp::texture> generate_atlas(::sdlpp::renderer& renderer)
{
    // Create surface
    auto surf_result = ::sdlpp::surface::create_rgb(
        ATLAS_WIDTH, ATLAS_HEIGHT,
        ::sdlpp::pixel_format_enum::RGBA8888);

    if (!surf_result) {
        std::cerr << "Failed to create atlas surface: " << surf_result.error() << "\n";
        return std::nullopt;
    }

    auto& surf = *surf_result;

    // Clear to transparent (no locking needed for software surfaces)
    surf.fill({0, 0, 0, 0});

    // Define colors
    ::sdlpp::color panel_bg{60, 60, 80, 255};
    ::sdlpp::color panel_border{100, 100, 140, 255};
    ::sdlpp::color panel_highlight{120, 120, 160, 255};
    ::sdlpp::color panel_shadow{40, 40, 60, 255};

    ::sdlpp::color dark_bg{30, 30, 45, 255};
    ::sdlpp::color dark_border{50, 50, 70, 255};
    ::sdlpp::color dark_highlight{60, 60, 80, 255};
    ::sdlpp::color dark_shadow{20, 20, 30, 255};

    ::sdlpp::color button_bg{80, 80, 120, 255};
    ::sdlpp::color button_border{100, 100, 150, 255};
    ::sdlpp::color button_highlight{140, 140, 180, 255};
    ::sdlpp::color button_shadow{50, 50, 80, 255};

    ::sdlpp::color text_color{220, 220, 240, 255};

    // Draw nine-slice sets
    draw_nine_slice(surf, PANEL_LIGHT_START, panel_bg, panel_border, panel_highlight, panel_shadow);
    draw_nine_slice(surf, PANEL_DARK_START, dark_bg, dark_border, dark_highlight, dark_shadow);
    draw_nine_slice(surf, BUTTON_NORMAL_START, button_bg, button_border, button_highlight, button_shadow);

    // Draw font glyphs
    draw_font_glyphs(surf, text_color);

    // Create texture from surface
    auto tex_result = ::sdlpp::texture::create(renderer, surf);
    if (!tex_result) {
        std::cerr << "Failed to create atlas texture: " << tex_result.error() << "\n";
        return std::nullopt;
    }

    return std::move(*tex_result);
}

/**
 * @brief Create nine_slice for light panel
 */
constexpr nine_slice light_panel_slice() {
    return nine_slice{
        PANEL_LIGHT_START + 0,  // top_left
        PANEL_LIGHT_START + 1,  // top
        PANEL_LIGHT_START + 2,  // top_right
        PANEL_LIGHT_START + 3,  // left
        PANEL_LIGHT_START + 4,  // center
        PANEL_LIGHT_START + 5,  // right
        PANEL_LIGHT_START + 6,  // bottom_left
        PANEL_LIGHT_START + 7,  // bottom
        PANEL_LIGHT_START + 8,  // bottom_right
        TILE_SIZE / 2,          // margin_h
        TILE_SIZE / 2           // margin_v
    };
}

/**
 * @brief Create nine_slice for dark panel
 */
constexpr nine_slice dark_panel_slice() {
    return nine_slice{
        PANEL_DARK_START + 0,
        PANEL_DARK_START + 1,
        PANEL_DARK_START + 2,
        PANEL_DARK_START + 3,
        PANEL_DARK_START + 4,
        PANEL_DARK_START + 5,
        PANEL_DARK_START + 6,
        PANEL_DARK_START + 7,
        PANEL_DARK_START + 8,
        TILE_SIZE / 2,
        TILE_SIZE / 2
    };
}

/**
 * @brief Create nine_slice for button
 */
constexpr nine_slice button_slice() {
    return nine_slice{
        BUTTON_NORMAL_START + 0,
        BUTTON_NORMAL_START + 1,
        BUTTON_NORMAL_START + 2,
        BUTTON_NORMAL_START + 3,
        BUTTON_NORMAL_START + 4,
        BUTTON_NORMAL_START + 5,
        BUTTON_NORMAL_START + 6,
        BUTTON_NORMAL_START + 7,
        BUTTON_NORMAL_START + 8,
        TILE_SIZE / 2,
        TILE_SIZE / 2
    };
}

} // anonymous namespace

// ============================================================================
// Strategy UI Widget
// ============================================================================

/**
 * @brief Main strategy game UI implementing the design.txt layout
 */
template<typename Backend>
class strategy_ui : public widget_container<Backend> {
public:
    using base = widget_container<Backend>;

    strategy_ui()
        : base(
            std::make_unique<linear_layout<Backend>>(direction::vertical, 0),
            nullptr
          )
    {
        build_ui();
    }

    ~strategy_ui() override = default;

    // Rule of Five
    strategy_ui(const strategy_ui&) = delete;
    strategy_ui& operator=(const strategy_ui&) = delete;
    strategy_ui(strategy_ui&&) noexcept = default;
    strategy_ui& operator=(strategy_ui&&) noexcept = default;

protected:
    void do_render(render_context<Backend>& ctx) const override {
        // Render background (dark fill for the gaps between panels)
        auto* renderer = get_renderer();
        if (renderer) {
            const auto& pos = ctx.position();
            const int x = point_utils::get_x(pos);
            const int y = point_utils::get_y(pos);
            const auto& bounds = this->bounds();

            // Use base class rect type for clear_region (inherited from sdlpp_renderer)
            onyxui::sdlpp::rect bg_rect{
                x, y,
                static_cast<int>(bounds.width.value),
                static_cast<int>(bounds.height.value)
            };
            renderer->clear_region(bg_rect, onyxui::sdlpp::color{20, 20, 30, 255});
        }

        // Render children
        base::do_render(ctx);
    }

private:
    void build_ui() {
        // Root layout: vertical box with no spacing (we use explicit gaps)
        auto root = std::make_unique<vbox<Backend>>(spacing::none);

        // Root vbox should expand to fill available space
        size_constraint expand_both;
        expand_both.policy = size_policy::expand;
        root->set_width_constraint(expand_both);
        root->set_height_constraint(expand_both);

        // 1. Menu bar - using OnyxUI menu framework
        auto menu_bar_widget = create_menu_bar_widget();
        // Set fixed height via constraint
        size_constraint height_c;
        height_c.policy = size_policy::fixed;
        height_c.preferred_size = logical_unit(MENU_HEIGHT);
        menu_bar_widget->set_height_constraint(height_c);
        size_constraint menu_w;
        menu_w.policy = size_policy::expand;
        menu_bar_widget->set_width_constraint(menu_w);
        root->add_child(std::move(menu_bar_widget));

        // 2. Gap after menu
        root->add_child(std::make_unique<spacer<Backend>>(0, GAP_SIZE));

        // 3. Middle section (map + minimap with gap)
        auto middle = create_middle_section();
        // Middle section expands both ways (reuse expand_both)
        middle->set_height_constraint(expand_both);
        middle->set_width_constraint(expand_both);
        root->add_child(std::move(middle));

        // 4. Gap before toolbox
        root->add_child(std::make_unique<spacer<Backend>>(0, GAP_SIZE));

        // 5. Toolbox
        auto toolbox = create_toolbox();
        size_constraint toolbox_h;
        toolbox_h.policy = size_policy::fixed;
        toolbox_h.preferred_size = logical_unit(TOOLBOX_HEIGHT);
        toolbox->set_height_constraint(toolbox_h);
        size_constraint toolbox_w;
        toolbox_w.policy = size_policy::expand;
        toolbox->set_width_constraint(toolbox_w);
        root->add_child(std::move(toolbox));

        this->add_child(std::move(root));
    }

    /**
     * @brief Create the top menu bar using OnyxUI menu framework
     */
    std::unique_ptr<menu_bar<Backend>> create_menu_bar_widget() {
        auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();

        // File menu
        auto file_menu = std::make_unique<menu<Backend>>();
        auto new_item = std::make_unique<menu_item<Backend>>("&New Game");
        new_item->clicked.connect([]() { std::cout << "New Game clicked\n"; });
        file_menu->add_item(std::move(new_item));

        auto load_item = std::make_unique<menu_item<Backend>>("&Load Game");
        load_item->clicked.connect([]() { std::cout << "Load Game clicked\n"; });
        file_menu->add_item(std::move(load_item));

        auto save_item = std::make_unique<menu_item<Backend>>("&Save Game");
        save_item->clicked.connect([]() { std::cout << "Save Game clicked\n"; });
        file_menu->add_item(std::move(save_item));

        file_menu->add_separator();

        auto exit_item = std::make_unique<menu_item<Backend>>("E&xit");
        exit_item->clicked.connect([]() { std::cout << "Exit clicked\n"; });
        file_menu->add_item(std::move(exit_item));

        menu_bar_widget->add_menu("&File", std::move(file_menu));

        // Edit menu
        auto edit_menu = std::make_unique<menu<Backend>>();
        auto undo_item = std::make_unique<menu_item<Backend>>("&Undo");
        undo_item->clicked.connect([]() { std::cout << "Undo clicked\n"; });
        edit_menu->add_item(std::move(undo_item));

        auto redo_item = std::make_unique<menu_item<Backend>>("&Redo");
        redo_item->clicked.connect([]() { std::cout << "Redo clicked\n"; });
        edit_menu->add_item(std::move(redo_item));

        menu_bar_widget->add_menu("&Edit", std::move(edit_menu));

        // View menu
        auto view_menu = std::make_unique<menu<Backend>>();
        auto zoom_in = std::make_unique<menu_item<Backend>>("Zoom &In");
        zoom_in->clicked.connect([]() { std::cout << "Zoom In clicked\n"; });
        view_menu->add_item(std::move(zoom_in));

        auto zoom_out = std::make_unique<menu_item<Backend>>("Zoom &Out");
        zoom_out->clicked.connect([]() { std::cout << "Zoom Out clicked\n"; });
        view_menu->add_item(std::move(zoom_out));

        view_menu->add_separator();

        auto minimap_item = std::make_unique<menu_item<Backend>>("Toggle &Minimap");
        minimap_item->clicked.connect([]() { std::cout << "Toggle Minimap clicked\n"; });
        view_menu->add_item(std::move(minimap_item));

        menu_bar_widget->add_menu("&View", std::move(view_menu));

        // Game menu
        auto game_menu = std::make_unique<menu<Backend>>();
        auto end_turn = std::make_unique<menu_item<Backend>>("&End Turn");
        end_turn->clicked.connect([]() { std::cout << "End Turn clicked\n"; });
        game_menu->add_item(std::move(end_turn));

        auto diplomacy = std::make_unique<menu_item<Backend>>("&Diplomacy");
        diplomacy->clicked.connect([]() { std::cout << "Diplomacy clicked\n"; });
        game_menu->add_item(std::move(diplomacy));

        menu_bar_widget->add_menu("&Game", std::move(game_menu));

        // Help menu
        auto help_menu = std::make_unique<menu<Backend>>();
        auto about = std::make_unique<menu_item<Backend>>("&About");
        about->clicked.connect([]() { std::cout << "About clicked\n"; });
        help_menu->add_item(std::move(about));

        menu_bar_widget->add_menu("&Help", std::move(help_menu));

        return menu_bar_widget;
    }

    /**
     * @brief Create the middle section with strategic map and minimap
     */
    std::unique_ptr<hbox<Backend>> create_middle_section() {
        auto container = std::make_unique<hbox<Backend>>(spacing::none);

        // hbox should expand to fill available width
        size_constraint expand_w;
        expand_w.policy = size_policy::expand;
        container->set_width_constraint(expand_w);

        // Strategic Map (flexible, takes remaining space)
        auto map_panel = create_strategic_map();
        map_panel->set_width_constraint(expand_w);
        container->add_child(std::move(map_panel));

        // Gap between map and minimap
        container->add_child(std::make_unique<spacer<Backend>>(GAP_SIZE, 0));

        // Minimap (fixed width)
        auto minimap = create_minimap();
        size_constraint fixed_w;
        fixed_w.policy = size_policy::fixed;
        fixed_w.preferred_size = logical_unit(MINIMAP_WIDTH);
        minimap->set_width_constraint(fixed_w);
        container->add_child(std::move(minimap));

        return container;
    }

    /**
     * @brief Create the strategic map panel
     */
    std::unique_ptr<tile_panel<Backend>> create_strategic_map() {
        auto panel = std::make_unique<tile_panel<Backend>>(nullptr, 4);
        panel->set_nine_slice(dark_panel_slice());

        // Map content (placeholder)
        auto content = std::make_unique<vbox<Backend>>(spacing::small);

        // Title
        content->add_child(make_label_centered<Backend>("Strategic Map"));

        // Spring to center content vertically
        content->add_child(std::make_unique<spring<Backend>>());

        // Placeholder text
        content->add_child(make_label_centered<Backend>("[ Map View Area ]"));
        content->add_child(make_label_centered<Backend>("Click to select units"));
        content->add_child(make_label_centered<Backend>("Drag to pan view"));

        content->add_child(std::make_unique<spring<Backend>>());

        // Coordinates display
        content->add_child(make_label<Backend>("Position: (0, 0)"));

        panel->add_child(std::move(content));
        return panel;
    }

    /**
     * @brief Create the minimap panel
     */
    std::unique_ptr<tile_panel<Backend>> create_minimap() {
        auto panel = std::make_unique<tile_panel<Backend>>(nullptr, 4);
        panel->set_nine_slice(light_panel_slice());

        auto content = std::make_unique<vbox<Backend>>(spacing::small);

        // Title
        content->add_child(make_label_centered<Backend>("Minimap"));

        // Minimap area placeholder
        content->add_child(std::make_unique<spring<Backend>>());
        content->add_child(make_label_centered<Backend>("[Mini]"));
        content->add_child(make_label_centered<Backend>("[Map]"));
        content->add_child(make_label_centered<Backend>("[View]"));
        content->add_child(std::make_unique<spring<Backend>>());

        // Zoom controls
        auto zoom_row = std::make_unique<hbox<Backend>>(spacing::small);
        zoom_row->add_child(make_button<Backend>("-", []() {
            std::cout << "Zoom out\n";
        }));
        zoom_row->add_child(make_label<Backend>("100%"));
        zoom_row->add_child(make_button<Backend>("+", []() {
            std::cout << "Zoom in\n";
        }));
        content->add_child(std::move(zoom_row));

        panel->add_child(std::move(content));
        return panel;
    }

    /**
     * @brief Create the bottom toolbox panel
     */
    std::unique_ptr<tile_panel<Backend>> create_toolbox() {
        auto panel = std::make_unique<tile_panel<Backend>>(nullptr, 4);
        panel->set_nine_slice(light_panel_slice());

        auto content = std::make_unique<hbox<Backend>>(spacing::medium);

        // Toolbox label
        content->add_child(make_label<Backend>("Toolbox:"));

        // Tool buttons
        content->add_child(make_button<Backend>("Select", []() {
            std::cout << "Select tool\n";
        }));
        content->add_child(make_button<Backend>("Move", []() {
            std::cout << "Move tool\n";
        }));
        content->add_child(make_button<Backend>("Attack", []() {
            std::cout << "Attack tool\n";
        }));
        content->add_child(make_button<Backend>("Build", []() {
            std::cout << "Build tool\n";
        }));
        content->add_child(make_button<Backend>("Defend", []() {
            std::cout << "Defend tool\n";
        }));

        // Separator (spring)
        content->add_child(std::make_unique<spring<Backend>>());

        // Unit info section
        content->add_child(make_label<Backend>("Selected: None"));

        // End turn button
        content->add_child(make_button<Backend>("End Turn", []() {
            std::cout << "End turn clicked\n";
        }));

        panel->add_child(std::move(content));
        return panel;
    }
};

// ============================================================================
// Main
// ============================================================================

#include <sdlpp/core/core.hh>
#include <sdlpp/video/window.hh>
#include <sdlpp/events/events.hh>
#include <onyxui/services/ui_context.hh>
#include <onyxui/core/backend_metrics.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/events/event_router.hh>
#include <chrono>

int main()
{
    std::cout << "Strategy UI Demo - Implementing design.txt layout\n";
    std::cout << "=================================================\n";
    std::cout << "Using self-contained procedural tile atlas\n\n";

    std::cout << "Layout:\n";
    std::cout << "  - Gap size: " << GAP_SIZE << " pixels\n";
    std::cout << "  - Menu height: " << MENU_HEIGHT << " pixels\n";
    std::cout << "  - Toolbox height: " << TOOLBOX_HEIGHT << " pixels\n";
    std::cout << "  - Minimap width: " << MINIMAP_WIDTH << " pixels\n";
    std::cout << "\nPress ESC or close window to exit.\n\n";

    try {
        // Create backend metrics with 1:1 scaling for tile backend
        // Tile widgets work with direct pixel coordinates
        backend_metrics<sdlpp_tile_backend> tile_metrics{
            .logical_to_physical_x = 1.0,
            .logical_to_physical_y = 1.0,
            .dpi_scale = 1.0
        };
        scoped_ui_context<sdlpp_tile_backend> ui_ctx(tile_metrics);

        // Initialize SDL
        ::sdlpp::init sdl(::sdlpp::init_flags::video);

        // Create window
        auto window_result = ::sdlpp::window::create(
            "Strategy UI Demo", 1024, 768,
            ::sdlpp::window_flags::resizable);

        if (!window_result) {
            std::cerr << "Failed to create window: " << window_result.error() << "\n";
            return 1;
        }

        auto& window = *window_result;

        // Create SDL renderer
        auto renderer_result = window.create_renderer();
        if (!renderer_result) {
            std::cerr << "Failed to create renderer: " << renderer_result.error() << "\n";
            return 1;
        }

        auto& sdl_renderer = *renderer_result;
        sdl_renderer.set_vsync(1);

        // Generate procedural atlas texture NOW (after SDL is initialized)
        std::cout << "Generating procedural tile atlas...\n";
        auto atlas_tex_opt = generate_atlas(sdl_renderer);
        if (!atlas_tex_opt) {
            std::cerr << "Failed to generate atlas\n";
            return 1;
        }
        auto atlas_texture = std::move(*atlas_tex_opt);
        std::cout << "Atlas generated: " << ATLAS_WIDTH << "x" << ATLAS_HEIGHT << " pixels\n";

        // Create atlas structure with the generated texture
        tile_atlas atlas{
            .texture = &atlas_texture,
            .tex_type = texture_type::sdlpp,
            .tile_width = TILE_SIZE,
            .tile_height = TILE_SIZE,
            .columns = ATLAS_COLUMNS
        };

        // Create font structure
        bitmap_font font{
            .atlas = &atlas,
            .glyph_width = TILE_SIZE,
            .glyph_height = TILE_SIZE,
            .first_char = FONT_FIRST_CHAR,
            .char_count = FONT_CHAR_COUNT,
            .first_tile_id = FONT_START
        };

        // Create theme
        tile_theme theme{
            .atlas = &atlas,
            .button = {
                .normal = button_slice(),
                .hover = button_slice(),
                .pressed = button_slice(),
                .disabled = button_slice()
            },
            .panel = {
                .background = light_panel_slice()
            },
            .label = {},
            .progress_bar = {},
            .scrollbar = {},
            .slider = {},
            .checkbox = {},
            .radio = {},
            .tab = {},
            .text_input = {},
            .combo = {},
            .list = {},
            .window = {},
            .menu = {},
            .tooltip = {},
            .group_box = {},
            .font_normal = font,
            .font_disabled = font,
            .font_highlight = font,
            .font_title = font,
            .font_small = font,
            .animations = {}
        };

        // Set tile theme for tile-specific widgets
        set_theme(theme);

        // Set the bitmap font for standard widgets via the tile_renderer adapter.
        // This allows menu_bar, label, button etc. to render using the bitmap font
        // instead of TTF fonts.

        // Create tile renderer
        tile_renderer onyx_renderer(sdl_renderer);
        onyx_renderer.set_atlas(&atlas);
        onyx_renderer.set_texture(&atlas_texture);
        onyx_renderer.set_default_bitmap_font(&font);  // Use bitmap font for standard widgets
        set_renderer(&onyx_renderer);

        // Create the UI widget
        strategy_ui<sdlpp_tile_backend> ui;

        // Get initial viewport
        auto viewport = onyx_renderer.get_viewport();
        int viewport_w = viewport.w;
        int viewport_h = viewport.h;

        // Main event loop
        bool quit = false;
        while (!quit) {
            // Process events
            while (auto event = ::sdlpp::event_queue::poll()) {
                if (event->type() == ::sdlpp::event_type::quit) {
                    quit = true;
                    break;
                }

                // Handle ESC key
                if (event->type() == ::sdlpp::event_type::key_down) {
                    auto& key_evt = event->key();
                    if (key_evt.scancode == SDL_SCANCODE_ESCAPE) {
                        quit = true;
                        break;
                    }
                }

                // Handle window resize
                if (event->type() == ::sdlpp::event_type::window_resized) {
                    viewport = onyx_renderer.get_viewport();
                    viewport_w = viewport.w;
                    viewport_h = viewport.h;
                }

                // Convert SDL event to ui_event and route
                auto ui_evt = sdlpp_tile_backend::create_event(*event);
                if (ui_evt) {
                    // First, let layer_manager handle events (for popup menus)
                    auto* layers = ui_services<sdlpp_tile_backend>::layers();
                    bool handled_by_layer = false;
                    if (layers) {
                        handled_by_layer = layers->route_event(*ui_evt);
                    }

                    if (!handled_by_layer) {
                        if (auto* mouse_evt = std::get_if<mouse_event>(&*ui_evt)) {
                            hit_test_path<sdlpp_tile_backend> hit_path;
                            auto* target = ui.hit_test_logical(
                                mouse_evt->x, mouse_evt->y, hit_path);

                            // Update hover state via input_manager (clears previous hover)
                            ui_ctx.input().set_hover(target);

                            if (target && !hit_path.empty()) {
                                route_event(*ui_evt, hit_path);
                            }
                        } else if (std::get_if<keyboard_event>(&*ui_evt)) {
                            auto* focused = ui_ctx.input().get_focused();
                            if (focused) {
                                focused->handle_event(*ui_evt, event_phase::target);
                            } else {
                                ui.handle_event(*ui_evt, event_phase::target);
                            }
                        }
                    }
                }
            }

            if (quit) break;

            // Layout - 1:1 scaling for tile backend (1 logical unit = 1 pixel)
            (void)ui.measure(
                logical_unit(static_cast<double>(viewport_w)),
                logical_unit(static_cast<double>(viewport_h)));
            ui.arrange(logical_rect{
                0.0_lu, 0.0_lu,
                logical_unit(static_cast<double>(viewport_w)),
                logical_unit(static_cast<double>(viewport_h))
            });

            // Render
            sdl_renderer.set_draw_color(::sdlpp::color{20, 20, 30, 255});
            sdl_renderer.clear();

            const auto* ui_theme = ui_ctx.themes().get_current_theme();

            ui.render(onyx_renderer, ui_theme, ui_ctx.metrics());

            // Render popup layers (menus, dialogs, etc.)
            if (auto* layers = ui_services<sdlpp_tile_backend>::layers()) {
                onyxui::sdlpp::rect viewport_rect{0, 0, viewport_w, viewport_h};
                layers->render_all_layers(onyx_renderer, viewport_rect, ui_theme, ui_ctx.metrics());
            }

            sdl_renderer.present();
        }

        // Cleanup
        set_renderer(nullptr);
        return 0;
    }
    catch (const std::exception& e) {
        set_renderer(nullptr);
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
