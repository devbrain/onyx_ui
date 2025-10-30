/**
 * @file widget_impl.hh
 * @brief Implementation of widget convenience methods that require other widgets
 * @author Assistant
 * @date 2025-10-19
 *
 * @details
 * This file contains the implementation of widget methods that depend on other
 * widget types (like label). Include this file AFTER including all widget headers
 * to avoid circular dependencies.
 *
 * ## Usage
 *
 * @code
 * #include <onyxui/widgets/widget.hh>
 * #include <onyxui/widgets/label.hh>
 * #include <onyxui/widgets/widget_impl.hh>  // Include after other widgets
 * @endcode
 */

#pragma once

#include <core/widget.hh>
#include <../../core/raii/scoped_tooltip.hh>

namespace onyxui {

    template<UIBackend Backend>
    scoped_tooltip<Backend> widget<Backend>::show_tooltip_scoped(const std::string& text) {
        return scoped_tooltip<Backend>(text, this->bounds());
    }

} // namespace onyxui
