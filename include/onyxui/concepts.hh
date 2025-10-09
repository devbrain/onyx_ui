//
// Created by igor on 08/10/2025.
//

#pragma once

#include <concepts>

namespace onyxui {
    // Concept for point-like types
    template<typename T>
    concept PointLike = requires(T p)
    {
        { p.x } -> std::convertible_to <int>;
        { p.y } -> std::convertible_to <int>;
    };

    // Concept for size-like types
    template<typename T>
    concept SizeLike = requires(T s)
                       {
                           { s.width } -> std::convertible_to <int>;
                           { s.height } -> std::convertible_to <int>;
                       } || requires(T s)
                       {
                           { s.w } -> std::convertible_to <int>;
                           { s.h } -> std::convertible_to <int>;
                       };

    // Concept for rectangle-like types
    template<typename T>
    concept RectLike = requires(T r)
                       {
                           { r.x } -> std::convertible_to <int>;
                           { r.y } -> std::convertible_to <int>;
                           { r.w } -> std::convertible_to <int>;
                           { r.h } -> std::convertible_to <int>;
                       } || requires(T r)
                       {
                           { r.x } -> std::convertible_to <int>;
                           { r.y } -> std::convertible_to <int>;
                           { r.width } -> std::convertible_to <int>;
                           { r.height } -> std::convertible_to <int>;
                       };

    // Helper functions to work with different rectangle formats
    namespace rect_utils {
        template<RectLike R>
        int get_x(const R& r) {
            return r.x;
        }

        template<RectLike R>
        int get_y(const R& r) {
            return r.y;
        }

        template<RectLike R>
        int get_width(const R& r) {
            if constexpr (requires { r.width; }) {
                return r.width;
            } else {
                return r.w;
            }
        }

        template<RectLike R>
        int get_height(const R& r) {
            if constexpr (requires { r.height; }) {
                return r.height;
            } else {
                return r.h;
            }
        }

        template<RectLike R>
        void set_bounds(R& r, int x, int y, int width, int height) {
            r.x = x;
            r.y = y;
            if constexpr (requires { r.width; }) {
                r.width = width;
                r.height = height;
            } else {
                r.w = width;
                r.h = height;
            }
        }

        template<RectLike R>
        bool contains(const R& r, int px, int py) {
            int x = get_x(r);
            int y = get_y(r);
            int w = get_width(r);
            int h = get_height(r);
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    }

    // Helper functions for size types
    namespace size_utils {
        template<SizeLike S>
        int get_width(const S& s) {
            if constexpr (requires { s.width; }) {
                return s.width;
            } else {
                return s.w;
            }
        }

        template<SizeLike S>
        int get_height(const S& s) {
            if constexpr (requires { s.height; }) {
                return s.height;
            } else {
                return s.h;
            }
        }

        template<SizeLike S>
        void set_size(S& s, int width, int height) {
            if constexpr (requires { s.width; }) {
                s.width = width;
                s.height = height;
            } else {
                s.w = width;
                s.h = height;
            }
        }

        template<SizeLike S1, SizeLike S2>
        bool equal(const S1& a, const S2& b) {
            return get_width(a) == get_width(b) &&
                   get_height(a) == get_height(b);
        }
    }
}
