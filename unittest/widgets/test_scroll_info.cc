/**
 * @file test_scroll_info.cc
 * @brief Unit tests for scroll_info structure
 * @author Testing Infrastructure Team
 * @date 2025-10-28
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/scroll/scroll_info.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;

TEST_CASE("scroll_info - Construction") {
    SUBCASE("Default constructor zero initializes") {
        scroll_info<test_backend> info;

        // Default initialized values should be zero
        CHECK(info.content_width == 0.0);
        CHECK(info.content_height == 0.0);
        CHECK(info.viewport_width == 0.0);
        CHECK(info.viewport_height == 0.0);
        CHECK(info.scroll_x == 0.0);
        CHECK(info.scroll_y == 0.0);
    }

    SUBCASE("Construct with explicit double values") {
        scroll_info<test_backend> info(200.0, 300.0, 100.0, 150.0, 10.0, 20.0);

        CHECK(info.content_width == 200.0);
        CHECK(info.content_height == 300.0);
        CHECK(info.viewport_width == 100.0);
        CHECK(info.viewport_height == 150.0);
        CHECK(info.scroll_x == 10.0);
        CHECK(info.scroll_y == 20.0);
    }

    SUBCASE("Construct from Backend types (backward compatibility)") {
        test_backend::size_type content{200, 300};
        test_backend::size_type viewport{100, 150};
        test_backend::point_type offset{10, 20};

        scroll_info<test_backend> info(content, viewport, offset);

        CHECK(info.content_width == 200.0);
        CHECK(info.content_height == 300.0);
        CHECK(info.viewport_width == 100.0);
        CHECK(info.viewport_height == 150.0);
        CHECK(info.scroll_x == 10.0);
        CHECK(info.scroll_y == 20.0);
    }

    SUBCASE("Construct from Backend types with default offset") {
        test_backend::size_type content{200, 300};
        test_backend::size_type viewport{100, 150};

        scroll_info<test_backend> info(content, viewport);

        CHECK(info.scroll_x == 0.0);
        CHECK(info.scroll_y == 0.0);
    }
}

TEST_CASE("scroll_info - needs_horizontal_scroll") {
    SUBCASE("Content wider than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 200.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_horizontal_scroll() == true);
    }

    SUBCASE("Content equals viewport width") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_horizontal_scroll() == false);
    }

    SUBCASE("Content narrower than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 50.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_horizontal_scroll() == false);
    }
}

TEST_CASE("scroll_info - needs_vertical_scroll") {
    SUBCASE("Content taller than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 200.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_vertical_scroll() == true);
    }

    SUBCASE("Content equals viewport height") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_vertical_scroll() == false);
    }

    SUBCASE("Content shorter than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 50.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_vertical_scroll() == false);
    }
}

TEST_CASE("scroll_info - max_scroll_x") {
    SUBCASE("Content wider than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 300.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_x() == 200.0);  // 300 - 100
    }

    SUBCASE("Content equals viewport width") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_x() == 0.0);
    }

    SUBCASE("Content narrower than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 50.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_x() == 0.0);  // No negative scroll
    }

    SUBCASE("Large content") {
        scroll_info<test_backend> info;
        info.content_width = 10000.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_x() == 9900.0);
    }
}

TEST_CASE("scroll_info - max_scroll_y") {
    SUBCASE("Content taller than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 300.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_y() == 200.0);  // 300 - 100
    }

    SUBCASE("Content equals viewport height") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 100.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_y() == 0.0);
    }

    SUBCASE("Content shorter than viewport") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 50.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_y() == 0.0);  // No negative scroll
    }

    SUBCASE("Large content") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 10000.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.max_scroll_y() == 9900.0);
    }
}

TEST_CASE("scroll_info - Legacy int accessors") {
    scroll_info<test_backend> info;
    info.content_width = 300.5;
    info.content_height = 400.7;
    info.viewport_width = 100.2;
    info.viewport_height = 150.8;
    info.scroll_x = 50.3;
    info.scroll_y = 75.9;

    SUBCASE("scroll_x_int floors to int") {
        CHECK(info.scroll_x_int() == 50);
    }

    SUBCASE("scroll_y_int floors to int") {
        CHECK(info.scroll_y_int() == 75);
    }

    SUBCASE("max_scroll_x_int floors to int") {
        // 300.5 - 100.2 = 200.3, floor = 200
        CHECK(info.max_scroll_x_int() == 200);
    }

    SUBCASE("max_scroll_y_int floors to int") {
        // 400.7 - 150.8 = 249.9, floor = 249
        CHECK(info.max_scroll_y_int() == 249);
    }
}

TEST_CASE("scroll_info - Edge cases") {
    SUBCASE("Zero content size") {
        scroll_info<test_backend> info;
        info.content_width = 0.0;
        info.content_height = 0.0;
        info.viewport_width = 100.0;
        info.viewport_height = 100.0;

        CHECK(info.needs_horizontal_scroll() == false);
        CHECK(info.needs_vertical_scroll() == false);
        CHECK(info.max_scroll_x() == 0.0);
        CHECK(info.max_scroll_y() == 0.0);
    }

    SUBCASE("Zero viewport size") {
        scroll_info<test_backend> info;
        info.content_width = 100.0;
        info.content_height = 100.0;
        info.viewport_width = 0.0;
        info.viewport_height = 0.0;

        CHECK(info.needs_horizontal_scroll() == true);
        CHECK(info.needs_vertical_scroll() == true);
        CHECK(info.max_scroll_x() == 100.0);
        CHECK(info.max_scroll_y() == 100.0);
    }

    SUBCASE("Both zero") {
        scroll_info<test_backend> info;
        info.content_width = 0.0;
        info.content_height = 0.0;
        info.viewport_width = 0.0;
        info.viewport_height = 0.0;

        CHECK(info.needs_horizontal_scroll() == false);
        CHECK(info.needs_vertical_scroll() == false);
        CHECK(info.max_scroll_x() == 0.0);
        CHECK(info.max_scroll_y() == 0.0);
    }
}

TEST_CASE("scroll_info - Comparison operators") {
    scroll_info<test_backend> info1;
    info1.content_width = 200.0;
    info1.content_height = 300.0;
    info1.viewport_width = 100.0;
    info1.viewport_height = 150.0;
    info1.scroll_x = 10.0;
    info1.scroll_y = 20.0;

    scroll_info<test_backend> info2;
    info2.content_width = 200.0;
    info2.content_height = 300.0;
    info2.viewport_width = 100.0;
    info2.viewport_height = 150.0;
    info2.scroll_x = 10.0;
    info2.scroll_y = 20.0;

    CHECK(info1 == info2);

    info2.scroll_x = 20.0;
    info2.scroll_y = 30.0;
    CHECK(info1 != info2);
}
