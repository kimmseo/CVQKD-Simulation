AM_CPPFLAGS = \
    $(all_includes) \
    @PACKAGE_CFLAGS@ \
    -DPACKAGE_PIXMAPS_DIR=\""$(datadir)/pixmaps/gpredict"\" \
    -DPACKAGE_DATA_DIR=\""$(datadir)/gpredict"\"

noinst_PROGRAMS = weather-stuff

weather_stuff_SOURCES = \
    test.c \
    ../calc-weather-data.c ../calc-weather-data.h \
    ../../sat-log.c        ../../sat-log.h \
    ../../compat.c         ../../compat.h \
    ../../sat-cfg.c        ../../sat-cfg.h \
    ../../gpredict-utils.c ../../gpredict-utils.h \
    ../../strnatcmp.c      ../../strnatcmp.h

weather_stuff_LDADD = @PACKAGE_LIBS@