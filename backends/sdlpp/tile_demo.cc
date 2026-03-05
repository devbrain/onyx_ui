/**
 * @file tile_demo.cc
 * @brief Minimal tile backend demo to verify run_app template compiles
 *
 * This demo forces instantiation of sdlpp_tile_backend::run_app<> templates,
 * catching any compilation errors in:
 * - resolved_style initialization (all 12 fields required)
 * - scoped_ui_context setup
 * - backend_metrics initialization
 * - keyboard/resize event routing
 */

#include <onyxui/tile/tile_backend.hh>
#include <onyxui/tile/tile_theme.hh>
#include <onyxui/core/element.hh>
#include <iostream>

using namespace onyxui;
using namespace onyxui::tile;

// Simple test widget that does nothing (just for template instantiation)
template<typename Backend>
class minimal_tile_widget : public ui_element<Backend> {
public:
    minimal_tile_widget() : ui_element<Backend>(nullptr) {}

    void do_render(render_context<Backend>& /*ctx*/) const override {
        // Intentionally empty - this is just for compilation verification
    }
};

// Concrete widget type for the second template overload
using concrete_widget = minimal_tile_widget<sdlpp_tile_backend>;

int main()
{
    std::cout << "Tile backend template instantiation test\n";
    std::cout << "This demo verifies that run_app compiles correctly.\n";
    std::cout << "\n";

    // Create minimal theme (required by run_app)
    // Use correct field names from tile_types.hh
    tile_atlas atlas{
        .texture = nullptr,  // Would need actual texture for real rendering
        .tile_width = 8,
        .tile_height = 8,
        .columns = 16
    };

    // Use correct bitmap_font field names
    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 8,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Create minimal tile_theme with correct field names
    tile_theme theme{
        .atlas = &atlas,
        .button = {},
        .panel = {},
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

    set_theme(theme);

    // Force template instantiation by taking function pointers
    // This verifies the templates compile without actually running them
    // (running requires actual SDL window/renderer setup)

    std::cout << "Checking template version with rebindable widget...\n";
    using fn_type1 = int(*)(const char*, int, int,
                            std::function<void(minimal_tile_widget<sdlpp_tile_backend>&)>);
    auto* fn1 = static_cast<fn_type1>(&sdlpp_tile_backend::run_app<minimal_tile_widget>);
    std::cout << "  Function pointer: " << (fn1 ? "OK" : "NULL") << "\n";

    std::cout << "Checking template version with concrete widget...\n";
    using fn_type2 = int(*)(const char*, int, int,
                            std::function<void(concrete_widget&)>);
    auto* fn2 = static_cast<fn_type2>(&sdlpp_tile_backend::run_app<concrete_widget>);
    std::cout << "  Function pointer: " << (fn2 ? "OK" : "NULL") << "\n";

    std::cout << "\nAll templates instantiated successfully!\n";
    std::cout << "Note: To actually run a tile app, you need a real tile atlas texture.\n";

    return 0;
}
