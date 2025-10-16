//
// Created by igor on 14/10/2025.
//

#pragma once

namespace onyxui::conio {
    struct rect {
        rect(int x, int y, int w, int h)
            : x(x),
              y(y),
              w(w),
              h(h) {
        }

        rect() : rect(0,0,0,0) {}
        int x,y,w,h;
    };
}