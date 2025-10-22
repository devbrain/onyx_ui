//
// Exported Termbox2 Wrapper Functions
// These wrappers provide properly exported symbols for termbox2 functions
//

#pragma once

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
