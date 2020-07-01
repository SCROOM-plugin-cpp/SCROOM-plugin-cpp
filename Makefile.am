include $(top_srcdir)/aminclude/doxygen.am

ACLOCAL_AMFLAGS = -I m4

########################################################################
### plugins/sep
########################################################################

sep_plugin_libs = \
	plugins/sep/src/libspsep.la

plugins_sep_src_libspsep_la_CPPFLAGS = @SEPPLUGIN_CFLAGS@

plugins_sep_src_libspsep_la_SOURCES = \
	plugins/sep/main.cc \
	plugins/sep/sep.cc \
	plugins/sep/sep.hh \
	plugins/sep/seppresentation.cc \
	plugins/sep/seppresentation.hh \
	plugins/sep/sepsource.cc \
	plugins/sep/sepsource.hh \
	plugins/sep/sep-helpers.cc \
	plugins/sep/sep-helpers.hh \
	plugins/sep/sli/slipresentation.cc \
	plugins/sep/sli/slipresentation.hh \
	plugins/sep/sli/slisource.cc \
	plugins/sep/sli/slisource.hh \
	plugins/sep/sli/slicontrolpanel.cc \
	plugins/sep/sli/slicontrolpanel.hh \
	plugins/sep/sli/slilayer.cc \
	plugins/sep/sli/slilayer.hh \
	plugins/sep/sli/sli-helpers.cc \
	plugins/sep/sli/sli-helpers.hh \
	plugins/sep/varnish/varnish.cc \
	plugins/sep/varnish/varnish.hh

plugins_sep_src_libspsep_la_LIBADD = @SEPPLUGIN_LIBS@ @BOOSTTHREADLIB@ @TILEDBITMAPLIB@ @SCROOMLIB@ @BOOSTFILESYSTEMLIB@ @THREADPOOLLIB@ -ltiff -ldl -lpthread

plugins_sep_src_libspsep_la_LDFLAGS = $(loadflag) -I../scroom/gui/src/ -pthread

if BOOST_UNITTESTS

sep_plugin_tests = \
	plugins/sep/test/sep-tests

sep_plugin_check_programs = \
	plugins/sep/test/sep-tests

plugins_sep_test_sep_tests_CPPFLAGS = @SEPPLUGIN_CFLAGS@

plugins_sep_test_sep_tests_LDFLAGS = -pthread

plugins_sep_test_sep_tests_SOURCES = \
	plugins/sep/test/main.cc \
	plugins/sep/test/slipresentation-tests.cc \
	plugins/sep/test/slisource-tests.cc \
	plugins/sep/test/slihelpers-tests.cc \
	plugins/sep/test/sepsource-tests.cc \
	plugins/sep/test/sephelpers-tests.cc \
	plugins/sep/test/varnish-tests.cc \
	plugins/sep/test/seppresentation-tests.cc \
	plugins/sep/test/sep-tests.cc

plugins_sep_test_sep_tests_LDADD = \
	@BOOSTTESTLIB@ @SEPPLUGIN_LIBS@ @BOOSTTHREADLIB@ @TILEDBITMAPLIB@ @SCROOMLIB@ @BOOSTFILESYSTEMLIB@ @THREADPOOLLIB@ -ltiff -ldl -lpthread \
	plugins/sep/src/libspsep.la \
	plugins/sep/*.lo \
	plugins/sep/sli/*.lo \
	plugins/sep/varnish/*.lo

endif BOOST_UNITTESTS

########################################################################
### plugins/pipette
########################################################################

pipette_plugin_libs = \
	plugins/pipette/src/libsppipette.la

plugins_pipette_src_libsppipette_la_CPPFLAGS = @PIPETTEPLUGIN_CFLAGS@

plugins_pipette_src_libsppipette_la_SOURCES = \
	plugins/pipette/src/pipette.cc \
	plugins/pipette/src/pipette.hh \
	plugins/pipette/src/main.cc

plugins_pipette_src_libsppipette_la_LIBADD = @PIPETTEPLUGIN_LIBS@ @BOOSTTHREADLIB@ @SCROOMLIB@ @THREADPOOLLIB@

plugins_pipette_src_libsppipette_la_LDFLAGS = $(loadflag)

if BOOST_UNITTESTS

pipette_plugin_tests = \
	plugins/pipette/test/pipette-tests

pipette_plugin_check_programs = \
	plugins/pipette/test/pipette-tests

plugins_pipette_test_pipette_tests_CPPFLAGS = @PIPETTEPLUGIN_CFLAGS@

plugins_pipette_test_pipette_tests_LDFLAGS = -pthread

plugins_pipette_test_pipette_tests_SOURCES = \
	plugins/pipette/test/main.cc \
	plugins/pipette/test/pipette-tests.cc

plugins_pipette_test_pipette_tests_LDADD = \
	@BOOSTTESTLIB@ @PIPETTEPLUGIN_LIBS@ @BOOSTTHREADLIB@ @SCROOMLIB@ @BOOSTFILESYSTEMLIB@ @THREADPOOLLIB@ -ldl \
	plugins/pipette/src/libsppipette.la \
	plugins/pipette/src/*.lo

endif BOOST_UNITTESTS

########################################################################
### Wrapping up
########################################################################

### Plugins
plugin_LTLIBRARIES = $(pipette_plugin_libs) $(sep_plugin_libs)
plugins_check_programs = $(sep_plugin_check_programs) $(pipette_plugin_check_programs) 
plugins_tests = $(sep_plugin_tests) $(pipette_plugin_tests)

########################################################################
### Root
########################################################################

EXTRA_DIST = \
	build-aux/git-version-gen \
	.project-version

check_PROGRAMS = $(plugins_check_programs)
TESTS = $(plugins_tests) 
