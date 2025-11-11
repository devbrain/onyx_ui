
#ifndef ONYXUI_CONIO_EXPORT_H
#define ONYXUI_CONIO_EXPORT_H

#ifdef ONYXUI_CONIO_STATIC_DEFINE
#  define ONYXUI_CONIO_EXPORT
#  define ONYXUI_CONIO_NO_EXPORT
#else
#  ifndef ONYXUI_CONIO_EXPORT
#    ifdef onyxui_conio_backend_EXPORTS
        /* We are building this library */
#      define ONYXUI_CONIO_EXPORT 
#    else
        /* We are using this library */
#      define ONYXUI_CONIO_EXPORT 
#    endif
#  endif

#  ifndef ONYXUI_CONIO_NO_EXPORT
#    define ONYXUI_CONIO_NO_EXPORT 
#  endif
#endif

#ifndef ONYXUI_CONIO_DEPRECATED
#  define ONYXUI_CONIO_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef ONYXUI_CONIO_DEPRECATED_EXPORT
#  define ONYXUI_CONIO_DEPRECATED_EXPORT ONYXUI_CONIO_EXPORT ONYXUI_CONIO_DEPRECATED
#endif

#ifndef ONYXUI_CONIO_DEPRECATED_NO_EXPORT
#  define ONYXUI_CONIO_DEPRECATED_NO_EXPORT ONYXUI_CONIO_NO_EXPORT ONYXUI_CONIO_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONYXUI_CONIO_NO_DEPRECATED
#    define ONYXUI_CONIO_NO_DEPRECATED
#  endif
#endif

#endif /* ONYXUI_CONIO_EXPORT_H */
