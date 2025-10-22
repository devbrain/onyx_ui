//
// SDL2 Backend Geometry Types
//

#pragma once

namespace onyxui::sdl2 {
    struct rect {
        rect(int x, int y, int w, int h)
            : x(x),
              y(y),
              w(w),
              h(h) {
        }

        rect()
            : rect(0, 0, 0, 0) {
        }

        int x, y, w, h;
    };

    struct size {
        size(int w, int h)
            : w(w), h(h) {
        }

        size()
            : size(0, 0) {
        }

        int w, h;
        bool operator==(const size&) const = default;
    };

    struct point {
        point(int x, int y)
            : x(x), y(y) {
        }

        point()
            : point(0, 0) {
        }

        int x, y;
        bool operator==(const point&) const = default;
    };
}
