//
// Created by igor on 13/10/2025.
//

#pragma once

#include <memory>
#include "colors.hh"
#include "rect.hh"

namespace onyxui::conio {

    class vram {
        public:
            vram();
            ~vram();

            void set_clip(const rect& r);
            rect get_clip() const noexcept;
            void put(int x, int y, int ch, color fg, color bg);

            [[nodiscard]] int get_width() const;
            [[nodiscard]] int get_height() const;

            void present();
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}