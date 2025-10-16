//
// Created by igor on 13/10/2025.
//

#pragma once

#include <cstdint>

namespace onyxui::conio {
    struct color {
        color(uint8_t r, uint8_t g, uint8_t b)
            : r(r),
              g(g),
              b(b) {
        }

        uint8_t r, g, b;
    };
}
