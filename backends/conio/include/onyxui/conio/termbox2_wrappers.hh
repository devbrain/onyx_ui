//
// Exported Termbox2 Wrapper Functions
// These wrappers provide properly exported symbols for termbox2 functions
//

#pragma once

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wuseless-cast"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#elif defined(_MSC_VER)
#endif

#include <termbox2.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#endif

#include <onyxui/backends/conio/onyxui_conio_export.h>

// Forward declare tb_event to avoid including termbox2.h here
struct tb_event;

namespace onyxui::conio {
    /**
     * @brief Get terminal width (wrapper for tb_width)
     * @return Terminal width in characters
     */
    ONYXUI_CONIO_EXPORT int conio_get_width();

    /**
     * @brief Get terminal height (wrapper for tb_height)
     * @return Terminal height in characters
     */
    ONYXUI_CONIO_EXPORT int conio_get_height();

    /**
     * @brief Poll for terminal events (wrapper for tb_poll_event)
     * @param event Pointer to event structure to fill
     * @return Status code from tb_poll_event
     */
    ONYXUI_CONIO_EXPORT int conio_poll_event(tb_event* event);
}
