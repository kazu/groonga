SUBDIRS =		\
	stress		\
	performance	\
	inverted-index

if WITH_CUTTER
noinst_LTLIBRARIES =				\
	suite-groonga-test-fixtures.la
endif

INCLUDES =			\
	-I$(srcdir)		\
	-I$(srcdir)/../lib	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

suite_groonga_test_fixtures_la_SOURCES = suite-groonga-test-fixtures.c
