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
### plugins/pipette
########################################################################

pipette_plugin_libs = \
	plugins/pipette/src/libsppipette.la

plugins_pipette_src_libsppipette_la_CPPFLAGS = @PIPETTEPLUGIN_CFLAGS@

plugins_pipette_src_libsppipette_la_SOURCES = \
	plugins/pipette/pipette.cc \
	plugins/pipette/pipette.hh \
	plugins/pipette/main.cc


plugins_pipette_src_libsppipette_la_LIBADD = @PIPETTEPLUGIN_LIBS@ @BOOSTTHREADLIB@ @TILEDBITMAPLIB@ @SCROOMLIB@

plugins_pipette_src_libsppipette_la_LDFLAGS = $(loadflag)

########################################################################
### Wrapping up
########################################################################

### Plugins
plugin_LTLIBRARIES = $(example_plugin_libs) $(pipette_plugin_libs)

########################################################################
### Root
########################################################################

EXTRA_DIST = \
	build-aux/git-version-gen \
	.project-version
