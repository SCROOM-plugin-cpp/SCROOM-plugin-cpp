include $(top_srcdir)/aminclude/doxygen.am

ACLOCAL_AMFLAGS = -I m4

########################################################################
### plugins/example
########################################################################

example_plugin_libs = \
	plugins/example/src/libspexample.la

plugins_example_src_libspexample_la_CPPFLAGS = @EXAMPLEPLUGIN_CFLAGS@

plugins_example_src_libspexample_la_SOURCES = \
	plugins/example/example.cc \
	plugins/example/example.hh \
	plugins/example/examplepresentation.cc \
	plugins/example/examplepresentation.hh \
	plugins/example/main.cc

plugins_example_src_libspexample_la_LIBADD = @EXAMPLEPLUGIN_LIBS@ @BOOSTTHREADLIB@ @TILEDBITMAPLIB@ @SCROOMLIB@

plugins_example_src_libspexample_la_LDFLAGS = $(loadflag)

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
	plugins/sep/sli/slipresentation.cc \
	plugins/sep/sli/slipresentation.hh \
	plugins/sep/sli/slicontrolpanel.cc \
	plugins/sep/sli/slicontrolpanel.hh \
	plugins/sep/sli/slilayer.cc \
	plugins/sep/sli/slilayer.hh

plugins_sep_src_libspsep_la_LIBADD = @SEPPLUGIN_LIBS@ @BOOSTTHREADLIB@ @TILEDBITMAPLIB@ @SCROOMLIB@ -ltiff

plugins_sep_src_libspsep_la_LDFLAGS = $(loadflag) -I../scroom/gui/src/

########################################################################
### Wrapping up
########################################################################

### Plugins
plugin_LTLIBRARIES = $(example_plugin_libs) $(sep_plugin_libs)

########################################################################
### Root
########################################################################

EXTRA_DIST = \
	build-aux/git-version-gen \
	.project-version
