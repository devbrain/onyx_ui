/**
 * @file xpm_parser.cc
 * @brief XPM parser implementation with embedded Windows 3.11 icons
 */

#include <onyxui/sdlpp/xpm_parser.hh>
#include <charconv>
#include <cstring>
#include <sstream>

namespace onyxui::sdlpp {

// ============================================================================
// XPM Parser Implementation
// ============================================================================

std::optional<xpm_color> xpm_image::parse_hex_color(std::string_view hex) {
    if (hex.empty() || hex[0] != '#') {
        return std::nullopt;
    }
    hex.remove_prefix(1);  // Remove '#'

    xpm_color result;
    result.a = 255;  // Opaque by default

    if (hex.length() == 6) {
        // #RRGGBB format
        auto parse_byte = [](std::string_view s) -> std::optional<std::uint8_t> {
            unsigned int val = 0;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val, 16);
            if (ec != std::errc{}) return std::nullopt;
            return static_cast<std::uint8_t>(val);
        };

        auto r = parse_byte(hex.substr(0, 2));
        auto g = parse_byte(hex.substr(2, 2));
        auto b = parse_byte(hex.substr(4, 2));

        if (!r || !g || !b) return std::nullopt;
        result.r = *r;
        result.g = *g;
        result.b = *b;
    } else if (hex.length() == 3) {
        // #RGB format (expand to #RRGGBB)
        auto parse_nibble = [](char c) -> std::optional<std::uint8_t> {
            unsigned int val = 0;
            auto [ptr, ec] = std::from_chars(&c, &c + 1, val, 16);
            if (ec != std::errc{}) return std::nullopt;
            return static_cast<std::uint8_t>(val * 17);  // 0xF -> 0xFF
        };

        auto r = parse_nibble(hex[0]);
        auto g = parse_nibble(hex[1]);
        auto b = parse_nibble(hex[2]);

        if (!r || !g || !b) return std::nullopt;
        result.r = *r;
        result.g = *g;
        result.b = *b;
    } else {
        return std::nullopt;
    }

    return result;
}

std::optional<xpm_image> xpm_image::parse(const char* const* data) {
    if (!data || !data[0]) {
        return std::nullopt;
    }

    // Parse header: "width height num_colors chars_per_pixel"
    int width = 0, height = 0, num_colors = 0, cpp = 0;
    if (std::sscanf(data[0], "%d %d %d %d", &width, &height, &num_colors, &cpp) != 4) {
        return std::nullopt;
    }

    if (width <= 0 || height <= 0 || num_colors <= 0 || cpp <= 0 || cpp > 2) {
        return std::nullopt;  // Only support 1-2 chars per pixel
    }

    // Parse color definitions
    std::unordered_map<std::string, xpm_color> color_map;
    for (int i = 0; i < num_colors; ++i) {
        const char* line = data[1 + i];
        if (!line) return std::nullopt;

        // Extract character(s) for this color
        std::string chars(line, static_cast<std::size_t>(cpp));

        // Find color specification (after 'c' token)
        const char* c_ptr = std::strstr(line + cpp, "\tc ");
        if (!c_ptr) c_ptr = std::strstr(line + cpp, " c ");
        if (!c_ptr) {
            // Try without space before 'c'
            c_ptr = std::strchr(line + cpp, 'c');
        }

        xpm_color color{};
        if (c_ptr) {
            // Skip to color value
            c_ptr += 2;  // Skip "c "
            while (*c_ptr == ' ' || *c_ptr == '\t') ++c_ptr;

            std::string color_str;
            while (*c_ptr && *c_ptr != ' ' && *c_ptr != '\t' && *c_ptr != '"') {
                color_str += *c_ptr++;
            }

            // Check for "None" (transparent)
            if (color_str == "None" || color_str == "none" || color_str == "NONE") {
                color.a = 0;  // Transparent
            } else if (color_str[0] == '#') {
                if (auto parsed = parse_hex_color(color_str)) {
                    color = *parsed;
                }
            }
        }

        color_map[chars] = color;
    }

    // Parse pixel data
    xpm_image img;
    img.m_width = width;
    img.m_height = height;
    img.m_pixels.resize(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        const char* line = data[1 + num_colors + y];
        if (!line) return std::nullopt;

        std::size_t line_len = std::strlen(line);
        for (int x = 0; x < width; ++x) {
            std::size_t char_idx = static_cast<std::size_t>(x * cpp);
            if (char_idx + static_cast<std::size_t>(cpp) > line_len) {
                return std::nullopt;  // Line too short
            }

            std::string chars(line + char_idx, static_cast<std::size_t>(cpp));
            auto it = color_map.find(chars);
            if (it != color_map.end()) {
                img.m_pixels[static_cast<std::size_t>(y * width + x)] = it->second;
            }
        }
    }

    return img;
}

std::optional<xpm_image> xpm_image::parse_string(std::string_view xpm_string) {
    // This is more complex - would need to extract quoted strings
    // For now, prefer the array-based parse() method
    (void)xpm_string;
    return std::nullopt;  // Not implemented
}

// ============================================================================
// Embedded Windows 3.11 XPM Icons (16x16 pixels)
// Colors: . = black, + = white, @ = dark gray (#868A8E), # = button face (#C3C7CB)
// ============================================================================

namespace win311_icons {

// Close button - active state (X symbol)
const char* const close_active[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+##..####..##@@",
    ".+###..##..###@@",
    ".+####....####@@",
    ".+#####..#####@@",
    ".+####....####@@",
    ".+###..##..###@@",
    ".+##..####..##@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Close button - inactive state
const char* const close_inactive[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+##@@####@@##@@",
    ".+###@@@@@@###@@",
    ".+####@@@@####@@",
    ".+#####@@#####@@",
    ".+####@@@@####@@",
    ".+###@@@@@@###@@",
    ".+##@@####@@##@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Minimize button - active state (down arrow)
const char* const minimize_active[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+###......###@@",
    ".+####....####@@",
    ".+#####..#####@@",
    ".+######.#####@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Minimize button - inactive state
const char* const minimize_inactive[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+###@@@@@@###@@",
    ".+####@@@@####@@",
    ".+#####@@#####@@",
    ".+######@#####@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Maximize button - active state (up arrow)
const char* const maximize_active[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+######.#####@@",
    ".+#####..#####@@",
    ".+####....####@@",
    ".+###......###@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Maximize button - inactive state
const char* const maximize_inactive[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+######@#####@@",
    ".+#####@@#####@@",
    ".+####@@@@####@@",
    ".+###@@@@@@###@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Restore button - active state (up+down arrows)
const char* const restore_active[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+######.#####@@",
    ".+#####..#####@@",
    ".+####....####@@",
    ".+############@@",
    ".+############@@",
    ".+####....####@@",
    ".+#####..#####@@",
    ".+######.#####@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Restore button - inactive state
const char* const restore_inactive[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+######@#####@@",
    ".+#####@@#####@@",
    ".+####@@@@####@@",
    ".+############@@",
    ".+############@@",
    ".+####@@@@####@@",
    ".+#####@@#####@@",
    ".+######@#####@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Menu button - active state (spacer icon - small window)
const char* const menu_active[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #C3C7CB",
    "@\tc #FFFFFF",
    "#\tc #868A8E",
    "................",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++...........+.",
    ".++.@@@@@@@@@.#+.",
    ".++...........#+.",
    ".+++###########.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    "................"
};

// Menu button - inactive state
const char* const menu_inactive[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #C3C7CB",
    "@\tc #FFFFFF",
    "#\tc #868A8E",
    "................",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++###########+.",
    ".++#+++++++++##+.",
    ".++############+.",
    ".+++###########.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    ".++++++++++++++.",
    "................"
};

// ============================================================================
// Checkbox icons (13x13 - standard Win3.11 size)
// Win3.11 checkboxes have 2-level sunken border: gray outer, black inner
// ============================================================================

// Checkbox unchecked - empty box with 3D sunken border
const char* const checkbox_unchecked[] = {
    "13 13 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@",
    "@...........#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.#########.#",
    "#############"
};

// Checkbox checked - box with checkmark
const char* const checkbox_checked[] = {
    "13 13 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@",
    "@...........#",
    "@.+++++++++.#",
    "@.+++++++..#",
    "@.++++++..+.#",
    "@.++.++..++.#",
    "@.++...+++.#",
    "@.++..++++.#",
    "@.+++.+++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.#########.#",
    "#############"
};

// Checkbox indeterminate - box with filled square
const char* const checkbox_indeterminate[] = {
    "13 13 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@",
    "@...........#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.++.....++.#",
    "@.++.....++.#",
    "@.++.....++.#",
    "@.++.....++.#",
    "@.++.....++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.#########.#",
    "#############"
};

// Checkbox unchecked disabled
const char* const checkbox_unchecked_disabled[] = {
    "13 13 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #C3C7CB",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@",
    "@...........#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.#########.#",
    "#############"
};

// Checkbox checked disabled
const char* const checkbox_checked_disabled[] = {
    "13 13 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #C3C7CB",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@",
    "@...........#",
    "@.+++++++++.#",
    "@.++++++++..#",
    "@.+++++++..+.#",
    "@.++.++..++.#",
    "@.++...+++.#",
    "@.++..++++.#",
    "@.+++.+++++.#",
    "@.+++++++++.#",
    "@.+++++++++.#",
    "@.#########.#",
    "#############"
};

// ============================================================================
// Radio button icons (16x16)
// Win3.11 radio buttons are circular with 3D sunken border
// Using 16x16 for better detail and consistency with other icons
// ============================================================================

// Radio unchecked - empty circle with 3D border
const char* const radio_unchecked[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "                ",
    "     @@@@@@     ",
    "   @@......##   ",
    "  @..++++++..#  ",
    "  @.++++++++.#  ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    "  @.++++++++.#  ",
    "  #..######..#  ",
    "   ##......##   ",
    "     ######     ",
    "                "
};

// Radio checked - circle with solid dot
const char* const radio_checked[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "                ",
    "     @@@@@@     ",
    "   @@......##   ",
    "  @..++++++..#  ",
    "  @.++++++++.#  ",
    " @.++++++++++.# ",
    " @.+++....+++.# ",
    " @.++......++.# ",
    " @.++......++.# ",
    " @.+++....+++.# ",
    " @.++++++++++.# ",
    "  @.++++++++.#  ",
    "  #..######..#  ",
    "   ##......##   ",
    "     ######     ",
    "                "
};

// Radio unchecked disabled
const char* const radio_unchecked_disabled[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #C3C7CB",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "                ",
    "     @@@@@@     ",
    "   @@......##   ",
    "  @..++++++..#  ",
    "  @.++++++++.#  ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    " @.++++++++++.# ",
    "  @.++++++++.#  ",
    "  #..######..#  ",
    "   ##......##   ",
    "     ######     ",
    "                "
};

// Radio checked disabled
const char* const radio_checked_disabled[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #C3C7CB",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "                ",
    "     @@@@@@     ",
    "   @@......##   ",
    "  @..++++++..#  ",
    "  @.++++++++.#  ",
    " @.++++++++++.# ",
    " @.+++@@@@+++.# ",
    " @.++@@@@@@++.# ",
    " @.++@@@@@@++.# ",
    " @.+++@@@@+++.# ",
    " @.++++++++++.# ",
    "  @.++++++++.#  ",
    "  #..######..#  ",
    "   ##......##   ",
    "     ######     ",
    "                "
};

// ============================================================================
// Scrollbar arrows (16x16)
// Win3.11 scrollbar buttons with 3D raised border and arrow
// ============================================================================

// Arrow up - raised button with up arrow
const char* const arrow_up[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+######.#####@@",
    ".+#####...####@@",
    ".+####.....###@@",
    ".+###.......##@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Arrow down - raised button with down arrow
const char* const arrow_down[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+###.......##@@",
    ".+####.....###@@",
    ".+#####...####@@",
    ".+######.#####@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Arrow left - raised button with left arrow
const char* const arrow_left[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+######.#####@@",
    ".+#####..#####@@",
    ".+####...#####@@",
    ".+###....#####@@",
    ".+####...#####@@",
    ".+#####..#####@@",
    ".+######.#####@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Arrow right - raised button with right arrow
const char* const arrow_right[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #000000",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "................",
    ".++++++++++++++@",
    ".+############@@",
    ".+############@@",
    ".+#####.######@@",
    ".+#####..#####@@",
    ".+#####...####@@",
    ".+#####....###@@",
    ".+#####...####@@",
    ".+#####..#####@@",
    ".+#####.######@@",
    ".+############@@",
    ".+############@@",
    ".+@@@@@@@@@@@@@@",
    ".@@@@@@@@@@@@@@@",
    "................"
};

// Arrow up disabled
const char* const arrow_up_disabled[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@@@@",
    "@++++++++++++++@",
    "@+############@@",
    "@+############@@",
    "@+############@@",
    "@+######@#####@@",
    "@+#####@@@####@@",
    "@+####@@@@@###@@",
    "@+###@@@@@@@##@@",
    "@+############@@",
    "@+############@@",
    "@+############@@",
    "@+############@@",
    "@+@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@"
};

// Arrow down disabled
const char* const arrow_down_disabled[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@@@@",
    "@++++++++++++++@",
    "@+############@@",
    "@+############@@",
    "@+############@@",
    "@+###@@@@@@@##@@",
    "@+####@@@@@###@@",
    "@+#####@@@####@@",
    "@+######@#####@@",
    "@+############@@",
    "@+############@@",
    "@+############@@",
    "@+############@@",
    "@+@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@"
};

// Arrow left disabled
const char* const arrow_left_disabled[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@@@@",
    "@++++++++++++++@",
    "@+############@@",
    "@+############@@",
    "@+######@#####@@",
    "@+#####@@#####@@",
    "@+####@@@#####@@",
    "@+###@@@@#####@@",
    "@+####@@@#####@@",
    "@+#####@@#####@@",
    "@+######@#####@@",
    "@+############@@",
    "@+############@@",
    "@+@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@"
};

// Arrow right disabled
const char* const arrow_right_disabled[] = {
    "16 16 5 1",
    " \tc None",
    ".\tc #868A8E",
    "+\tc #FFFFFF",
    "@\tc #868A8E",
    "#\tc #C3C7CB",
    "@@@@@@@@@@@@@@@@",
    "@++++++++++++++@",
    "@+############@@",
    "@+############@@",
    "@+#####@######@@",
    "@+#####@@#####@@",
    "@+#####@@@####@@",
    "@+#####@@@@###@@",
    "@+#####@@@####@@",
    "@+#####@@#####@@",
    "@+#####@######@@",
    "@+############@@",
    "@+############@@",
    "@+@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@"
};

} // namespace win311_icons

} // namespace onyxui::sdlpp
