SUBDIRS=profiles

AM_CPPFLAGS = -I$(top_srcdir)/src/libs \
  -I$(top_builddir)/src/libs \
  $(gwenhywfar_includes)

AM_CFLAGS=-DBUILDING_AQBANKING @visibility_cflags@

extra_sources=\
  sepa_pain_001.c \
  sepa_pain_008.c


EXTRA_DIST=README $(extra_sources)

noinst_HEADERS=sepa_p.h sepa.h

imexporterplugindir = $(aqbanking_plugindir)/imexporters
noinst_LTLIBRARIES=libabimexporters_sepa.la
imexporterplugin_DATA=sepa.xml

libabimexporters_sepa_la_SOURCES=sepa.c


sources:
	for f in $(libabimexporters_sepa_la_SOURCES); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done
	for f in $(imexporterplugin_DATA); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/pdsources; \
	done


cppcheck:
	for f in $(libabimexporters_sepa_la_SOURCES); do \
	  cppcheck --force $$f ; \
	done

