AM_CPPFLAGS = -I$(top_srcdir)/src/libs -I$(top_builddir)/src/libs $(gwenhywfar_includes) -I$(srcdir)/.. $(GWENGUI_GTK2_CFLAGS) $(GTK2_CFLAGS)


if WITH_GWENGUI_GTK2
noinst_PROGRAMS=abtest imptest test-dlg-setup
else
noinst_PROGRAMS=abtest imptest
endif

abtest_SOURCES=abtest.c
abtest_LDADD = $(aqbanking_internal_libs) $(gwenhywfar_libs)

imptest_SOURCES=imptest.c
imptest_LDADD = $(aqbanking_internal_libs) $(gwenhywfar_libs)


if WITH_GWENGUI_GTK2
test_dlg_setup_SOURCES = test-dlg-setup.c
test_dlg_setup_LDADD = \
  $(aqbanking_internal_libs) \
  $(gwenhywfar_libs) \
  $(GWENGUI_GTK2_LIBS) \
  $(GTK2_LIBS)
endif

EXTRA_DIST = test-dlg-setup.c

#cpptest_SOURCES=cpptest.cpp
#cpptest_LDADD = $(aqbanking_internal_libs) $(top_builddir)/src/libs/aqbanking++/libaqbankingpp.la $(gwenhywfar_libs) -lstdc++

sources:
	for f in $(abtest_la_SOURCES); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

built_sources:


cppcheck:

typefiles:

typedefs:

