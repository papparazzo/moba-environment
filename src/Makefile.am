AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = \
    moba-environment

moba_environment_SOURCES = \
    main.cpp \
    bridge.cpp \
    msgloop.cpp \
    eclipsecontrol.cpp

moba_environment_LDADD = \
    $(GLIB_LIBS) \
    -lwiringPi \
    -lmobacommon \
    $(srcdir)/../modules/lib-msghandling/src/moba/libmsghandling.a

AM_LDFLAGS = -L/usr/local/lib -pthread

AM_CPPFLAGS = \
    -I$(srcdir)/../modules/lib-msghandling/src \
    $(GLIB_CFLAGS) \
    -DSYSCONFIR='"$(sysconfdir)"' \
    -ggdb -g3 -O3 -Wall -Wno-deprecated-declarations

CLEANFILES = *~
