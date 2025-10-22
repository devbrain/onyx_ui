//
// SDL2 Backend Color Type
//

#pragma once

#include <cstdint>

namespace onyxui::sdl2 {
    struct color {
        // Color component constants
        static constexpr uint8_t MIN_COMPONENT = 0;
        static constexpr uint8_t MAX_COMPONENT = 255;

        constexpr color(uint8_t r, uint8_t g, uint8_t b)
            : r(r),
              g(g),
              b(b) {
        }

        // Default constructor - black
        constexpr color()
            : r(MIN_COMPONENT),
              g(MIN_COMPONENT),
              b(MIN_COMPONENT) {
        }

        uint8_t r, g, b;

        bool operator==(const color&) const = default;
    };
}
