/**
 * @file windows_cleanup.hh
 * @brief Defensive un-definition of Win32 macros that collide with onyx_ui
 *        public identifiers.
 *
 * The Windows SDK defines preprocessor macros for several common words that
 * onyx_ui also uses as identifiers:
 *
 *   - `small`  (from <rpcndr.h>)        — collides with `spacing::small`
 *   - `min`    (from <windef.h>)         — collides with `min_size`, `min(a,b)`
 *   - `max`    (from <windef.h>)         — collides with `max_size`, `max(a,b)`
 *
 * If any upstream header (SDL3, system headers, etc.) pulls in <windows.h>
 * before an onyx_ui public header is parsed, these macros replace onyx_ui
 * tokens during preprocessing and break the build with cryptic syntax errors.
 *
 * This header undefines them unconditionally on Windows. Include it from the
 * top of every onyx_ui public header that uses `small`, `min`, or `max` as
 * identifiers (notably `layout_strategy.hh`, but also anywhere comparisons
 * or ranges show up).
 *
 * This file intentionally has no include guard beyond `#pragma once` and no
 * own includes — it must be cheap to include repeatedly and must not pull
 * in any Win32 headers itself.
 */
#pragma once

#if defined(_WIN32)
#  ifdef small
#    undef small
#  endif
#  ifdef min
#    undef min
#  endif
#  ifdef max
#    undef max
#  endif
#endif
