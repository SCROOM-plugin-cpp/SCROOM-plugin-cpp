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

########################################################################
### plugins/sep
########################################################################

sep_plugin_libs = \
	plugins/sep_parsing/src/sep_parsing.la

plugins_sep_src_libssep_la_CPPFLAGS = @SEPPLUGIN_CFLAGS@

plugins_sep_src_libssep_la_SOURCES = \
	plugins/sep_parsing/sep.cc \
	plugins/sep_parsing/sep.hh \
	plugins/sep_parsing/seppresentation.cc \
	plugins/sep_parsing/seppresentation.hh \
	plugins/sep_parsing/main.cc


plugins_sep_src_libssep_la_LIBADD = @SEPPLUGIN_LIBS@ @BOOSTTHREADLIB@ @TILEDBITMAPLIB@ @SCROOMLIB@



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
