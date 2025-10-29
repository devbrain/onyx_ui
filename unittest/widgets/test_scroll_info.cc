/**
 * @file test_scroll_info.cc
 * @brief Unit tests for scroll_info structure
 * @author Testing Infrastructure Team
 * @date 2025-10-28
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/scroll_info.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;

TEST_CASE("scroll_info - Construction") {
    SUBCASE("Default constructor zero initializes") {
        scroll_info<test_backend> info;

        // Default initialized values should be zero
        CHECK(size_utils::get_width(info.content_size) == 0);
        CHECK(size_utils::get_height(info.content_size) == 0);
        CHECK(size_utils::get_width(info.viewport_size) == 0);
        CHECK(size_utils::get_height(info.viewport_size) == 0);
        CHECK(point_utils::get_x(info.scroll_offset) == 0);
        CHECK(point_utils::get_y(info.scroll_offset) == 0);
    }

    SUBCASE("Construct with explicit values") {
        test_backend::size_type content{200, 300};
        test_backend::size_type viewport{100, 150};
        test_backend::point_type offset{10, 20};

        scroll_info<test_backend> info(content, viewport, offset);

        CHECK(size_utils::get_width(info.content_size) == 200);
        CHECK(size_utils::get_height(info.content_size) == 300);
        CHECK(size_utils::get_width(info.viewport_size) == 100);
        CHECK(size_utils::get_height(info.viewport_size) == 150);
        CHECK(point_utils::get_x(info.scroll_offset) == 10);
        CHECK(point_utils::get_y(info.scroll_offset) == 20);
    }

    SUBCASE("Construct with default offset") {
        test_backend::size_type content{200, 300};
        test_backend::size_type viewport{100, 150};

        scroll_info<test_backend> info(content, viewport);

        CHECK(point_utils::get_x(info.scroll_offset) == 0);
        CHECK(point_utils::get_y(info.scroll_offset) == 0);
    }
}

TEST_CASE("scroll_info - needs_horizontal_scroll") {
    SUBCASE("Content wider than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {200, 100};
        info.viewport_size = {100, 100};

        CHECK(info.needs_horizontal_scroll() == true);
    }

    SUBCASE("Content equals viewport width") {
        scroll_info<test_backend> info;
        info.content_size = {100, 100};
        info.viewport_size = {100, 100};

        CHECK(info.needs_horizontal_scroll() == false);
    }

    SUBCASE("Content narrower than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {50, 100};
        info.viewport_size = {100, 100};

        CHECK(info.needs_horizontal_scroll() == false);
    }
}

TEST_CASE("scroll_info - needs_vertical_scroll") {
    SUBCASE("Content taller than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {100, 200};
        info.viewport_size = {100, 100};

        CHECK(info.needs_vertical_scroll() == true);
    }

    SUBCASE("Content equals viewport height") {
        scroll_info<test_backend> info;
        info.content_size = {100, 100};
        info.viewport_size = {100, 100};

        CHECK(info.needs_vertical_scroll() == false);
    }

    SUBCASE("Content shorter than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {100, 50};
        info.viewport_size = {100, 100};

        CHECK(info.needs_vertical_scroll() == false);
    }
}

TEST_CASE("scroll_info - max_scroll_x") {
    SUBCASE("Content wider than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {300, 100};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_x() == 200);  // 300 - 100
    }

    SUBCASE("Content equals viewport width") {
        scroll_info<test_backend> info;
        info.content_size = {100, 100};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_x() == 0);
    }

    SUBCASE("Content narrower than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {50, 100};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_x() == 0);  // No negative scroll
    }

    SUBCASE("Large content") {
        scroll_info<test_backend> info;
        info.content_size = {10000, 100};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_x() == 9900);
    }
}

TEST_CASE("scroll_info - max_scroll_y") {
    SUBCASE("Content taller than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {100, 300};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_y() == 200);  // 300 - 100
    }

    SUBCASE("Content equals viewport height") {
        scroll_info<test_backend> info;
        info.content_size = {100, 100};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_y() == 0);
    }

    SUBCASE("Content shorter than viewport") {
        scroll_info<test_backend> info;
        info.content_size = {100, 50};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_y() == 0);  // No negative scroll
    }

    SUBCASE("Large content") {
        scroll_info<test_backend> info;
        info.content_size = {100, 10000};
        info.viewport_size = {100, 100};

        CHECK(info.max_scroll_y() == 9900);
    }
}

TEST_CASE("scroll_info - Edge cases") {
    SUBCASE("Zero content size") {
        scroll_info<test_backend> info;
        info.content_size = {0, 0};
        info.viewport_size = {100, 100};

        CHECK(info.needs_horizontal_scroll() == false);
        CHECK(info.needs_vertical_scroll() == false);
        CHECK(info.max_scroll_x() == 0);
        CHECK(info.max_scroll_y() == 0);
    }

    SUBCASE("Zero viewport size") {
        scroll_info<test_backend> info;
        info.content_size = {100, 100};
        info.viewport_size = {0, 0};

        CHECK(info.needs_horizontal_scroll() == true);
        CHECK(info.needs_vertical_scroll() == true);
        CHECK(info.max_scroll_x() == 100);
        CHECK(info.max_scroll_y() == 100);
    }

    SUBCASE("Both zero") {
        scroll_info<test_backend> info;
        info.content_size = {0, 0};
        info.viewport_size = {0, 0};

        CHECK(info.needs_horizontal_scroll() == false);
        CHECK(info.needs_vertical_scroll() == false);
        CHECK(info.max_scroll_x() == 0);
        CHECK(info.max_scroll_y() == 0);
    }
}

TEST_CASE("scroll_info - Comparison operators") {
    scroll_info<test_backend> info1;
    info1.content_size = {200, 300};
    info1.viewport_size = {100, 150};
    info1.scroll_offset = {10, 20};

    scroll_info<test_backend> info2;
    info2.content_size = {200, 300};
    info2.viewport_size = {100, 150};
    info2.scroll_offset = {10, 20};

    CHECK(info1 == info2);

    info2.scroll_offset = {20, 30};
    CHECK(info1 != info2);
}
