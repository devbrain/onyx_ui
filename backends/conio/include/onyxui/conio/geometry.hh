//
// Created by igor on 14/10/2025.
//

#pragma once

namespace onyxui::conio {
    struct rect {
        rect(int xc, int yc, int width, int height)
            : x(xc),
              y(yc),
              w(width),
              h(height) {
        }

        rect()
            : rect(0, 0, 0, 0) {
        }

        int x, y, w, h;
    };

    struct size {
        size(int width, int height)
            : w(width), h(height) {
        }

        size()
            : size(0, 0) {
        }

        int w, h;
        bool operator==(const size&) const = default;
    };

    struct point {
        point(int xc, int yc)
            : x(xc), y(yc) {
        }

        point()
            : point(0, 0) {
        }

        int x, y;
        bool operator==(const point&) const = default;
    };
}
