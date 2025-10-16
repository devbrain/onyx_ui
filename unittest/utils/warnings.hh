//
// Created by igor on 16/10/2025.
//

#pragma once

// Portable warning suppression macros for self-move tests
// These macros provide cross-compiler support for suppressing self-move warnings
// in Rule of Five self-assignment safety tests
#if defined(__clang__)
    #define SUPPRESS_SELF_MOVE_BEGIN \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wself-move\"")
    #define SUPPRESS_SELF_MOVE_END \
    _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
    #define SUPPRESS_SELF_MOVE_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wself-move\"")
    #define SUPPRESS_SELF_MOVE_END \
    _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
    // MSVC doesn't have a self-move warning, no suppression needed
    #define SUPPRESS_SELF_MOVE_BEGIN
    #define SUPPRESS_SELF_MOVE_END
#else
    // Unknown compiler, no suppression
    #define SUPPRESS_SELF_MOVE_BEGIN
    #define SUPPRESS_SELF_MOVE_END
#endif
