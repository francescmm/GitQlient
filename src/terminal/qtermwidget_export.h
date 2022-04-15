
#ifndef QTERMWIDGET_EXPORT_H
#define QTERMWIDGET_EXPORT_H

#ifdef QTERMWIDGET_STATIC_DEFINE
#  define QTERMWIDGET_EXPORT
#  define QTERMWIDGET_NO_EXPORT
#else
#  ifndef QTERMWIDGET_EXPORT
#    ifdef qtermwidget5_EXPORTS
        /* We are building this library */
#      define QTERMWIDGET_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define QTERMWIDGET_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef QTERMWIDGET_NO_EXPORT
#    define QTERMWIDGET_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef QTERMWIDGET_DEPRECATED
#  define QTERMWIDGET_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef QTERMWIDGET_DEPRECATED_EXPORT
#  define QTERMWIDGET_DEPRECATED_EXPORT QTERMWIDGET_EXPORT QTERMWIDGET_DEPRECATED
#endif

#ifndef QTERMWIDGET_DEPRECATED_NO_EXPORT
#  define QTERMWIDGET_DEPRECATED_NO_EXPORT QTERMWIDGET_NO_EXPORT QTERMWIDGET_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef QTERMWIDGET_NO_DEPRECATED
#    define QTERMWIDGET_NO_DEPRECATED
#  endif
#endif

#endif /* QTERMWIDGET_EXPORT_H */
