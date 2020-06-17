AC_PREREQ([2.64])
AC_INIT([Scroom Plugins for Canon Production Printing],
        m4_esyscmd([build-aux/git-version-gen | tr -d '\n']),
        [],
        [scroom-cpp-plugins],
        [])

# Detect the target system
AC_CANONICAL_HOST
case "${host_os}" in
    linux*)
        build_linux=yes
        AC_MSG_NOTICE([detected linux environment])
        ;;
    cygwin*|mingw*|msys*)
        build_windows=yes
        AC_MSG_NOTICE([detected windows environment])
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported."])
        ;;
esac
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])

AM_INIT_AUTOMAKE([-Wall -Werror foreign silent-rules subdir-objects])
AM_SILENT_RULES([yes])
AM_PROG_AR
LT_PREREQ([2.2])

# Properly create shared libraries on both Linux and Windows
LT_INIT([dlopen win32-dll])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([plugins/example/example.hh])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
dnl AC_PROG_CC
AC_PROG_LN_S
dnl AC_PROG_INSTALL

AC_LANG([C++])
SCROOM_COMPILE_STDCXX_0X
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wall])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-W])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wextra])
dnl SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wconversion])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wpointer-arith])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wwrite-strings])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wunused])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wuninitialized])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Winit-self])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wcast-qual])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wswitch-enum])
dnl SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Weffc++])
dnl SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wold-style-cast])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wfloat-equal])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wnon-virtual-dtor])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Woverloaded-virtual])
SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wsign-promo])
SCROOM_TEST_ISYSTEM
    
# Disabled warnings
AM_COND_IF([LINUX], [
    # - Lots of warnings on Windows
    SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wredundant-decls])
])   
AM_COND_IF([WINDOWS], [
    # - Warns for C pointer declarations - On windows there are a lot of those in gtk headers
    SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wno-parentheses])
    # - On Windows, gtk uses deprecated glib stuff - need to move to gtk3 ;-)
    SCROOM_ADD_CXXFLAGS_IF_SUPPORTED([-Wno-deprecated-declarations])
])  

# This flag is used in Makefile.am and needed for properly generating DLLs on Windows
loadflag=""
AM_COND_IF([WINDOWS], 
    [loadflag="-no-undefined"])
AC_SUBST([loadflag])

# Configurable features
AC_ARG_ENABLE([doxygen-all-code],
  [AS_HELP_STRING([--enable-doxygen-all-code], 
    [Generate doxygen documentation for all code, instead of just the api @<:@default=no@:>@])],
  [want_doxygen_all_code=${enableval}],
  [want_doxygen_all_code=no])
AC_ARG_ENABLE([xml-test-output],
  [AS_HELP_STRING([--enable-xml-test-output], 
    [Generate test results in XML format @<:@default=no@:>@])],
  [want_xml_test_output=${enableval}],
  [want_xml_test_output=no])
  

# Doxygen
DX_DOXYGEN_FEATURE(ON)
DX_DOT_FEATURE(ON)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([$PACKAGE_NAME], [doxyfile], [doc])
  
# Checks for packages.
PKG_CHECK_MODULES([EXAMPLEPLUGIN], [gtk+-2.0 >= 2.0.0, cairo])

# Checks for libraries.
have_tiled_bitmap=no
SCROOM_SEARCH_LIBS([_Z17createTiledBitmapiiRKSt6vectorIN5boost10shared_ptrI15LayerOperationsEESaIS3_EE],
                   [stiledbitmap], [have_tiled_bitmap=yes])
AC_SUBST([TILEDBITMAPLIB], [$ac_cv_search__Z17createTiledBitmapiiRKSt6vectorIN5boost10shared_ptrI15LayerOperationsEESaIS3_EE])

have_libscroom=no
SCROOM_SEARCH_LIBS([_ZN6Scroom6Bitmap13BitmapSurface6createEii13_cairo_format],
                   [sscroom], [have_libscroom=yes])
if test "x${have_libscroom}" = xno
then
  ac_cv_search__ZN6Scroom6Bitmap13BitmapSurface6createEii13_cairo_format=""
fi
AC_SUBST([SCROOMLIB], [$ac_cv_search__ZN6Scroom6Bitmap13BitmapSurface6createEii13_cairo_format])

have_libsutil=no
SCROOM_SEARCH_LIBS([_ZN6Scroom5Utils6Detail15assertionFailedEPKcS3_S3_S3_j],
                   [sutil], [have_libsutil=yes])
if test "x${have_libsutil}" = xno
then
  ac_cv_search__ZN6Scroom5Utils6Detail15assertionFailedEPKcS3_S3_S3_j=""
fi
AC_SUBST([SUTILLIB], [$ac_cv_search__ZN6Scroom5Utils6Detail15assertionFailedEPKcS3_S3_S3_j])

have_libsthreadpool=no
SCROOM_SEARCH_LIBS([_Z8CpuBoundv],
                   [sthreadpool], [have_libsthreadpool=yes])
if test "x${have_libsthreadpool}" = xno
then
  ac_cv_search__Z8CpuBoundv=""
fi
AC_SUBST([THREADPOOLLIB], [$ac_cv_search__Z8CpuBoundv])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_HEADERS([scroom/observable.hh scroom/unused.hh scroom/utilities.hh])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset pow select sqrt strerror])

# Checking for boost system
have_boost_system_lib=no
SCROOM_SEARCH_LIBS([_ZN5boost6system16generic_categoryEv], [boost_system-mt boost_system], [have_boost_system_lib=yes])
if test "x${have_boost_system_lib}" = xno
then
  ac_cv_search__ZN5boost6system16generic_categoryEv=""
fi

# Checking for boost thread
have_boost_thread_headers=no
AC_CHECK_HEADERS([boost/thread.hpp boost/thread/mutex.hpp], [have_boost_thread_headers=yes])
have_boost_thread_lib=no
SCROOM_SEARCH_LIBS([_ZN5boost6thread6detachEv], [boost_thread-mt boost_thread], [have_boost_thread_lib=yes])
AC_SUBST([BOOSTTHREADLIB], ["$ac_cv_search__ZN5boost6thread6detachEv $ac_cv_search__ZN5boost6system16generic_categoryEv"])

#checking for boost unittests
have_boost_test_headers=no
AC_CHECK_HEADERS([boost/test/unit_test.hpp], [have_boost_test_headers=yes])
have_boost_test_lib=no
SCROOM_SEARCH_LIBS([_ZN5boost9unit_test16results_reporter10set_streamERSo],
                   [boost_unit_test_framework-mt boost_unit_test_framework], [have_boost_test_lib=yes])
AC_SUBST([BOOSTTESTLIB], [$ac_cv_search__ZN5boost9unit_test16results_reporter10set_streamERSo])

want_boost_tests=no
if test x$have_boost_test_headers = xyes -a x$have_boost_test_lib = xyes
then
  want_boost_tests=yes
fi
AM_CONDITIONAL([BOOST_UNITTESTS], [test x$want_boost_tests = xyes])

# If no doxygen, then we can't process all code
if DX_TEST_FEATURE([doc])
then
    want_doxygen=yes
else
    want_doxygen=no
    want_doxygen_all_code=no
fi

if test "x${want_doxygen_all_code}" = xyes
then
  DX_EXTRA_SRC="\$(top_srcdir)/plugins"
else
  DX_EXTRA_SRC=
fi
AC_SUBST(DX_EXTRA_SRC)

if test "x${want_xml_test_output}" = xyes
then
  AC_DEFINE([XML_TEST_OUTPUT], 1, [Have all Boost unittests report in xml format])
fi

# Some sanity checks
if test "x${have_tiled_bitmap}" = "xno"
then
  AC_MSG_ERROR([Scroom TiledBitmap library not found.])
fi   
if test "x${have_libsutil}" = "xno"
then
  AC_MSG_ERROR([Scroom Utilities library not found.])
fi   
if test "x${have_libsthreadpool}" = "xno"
then
  AC_MSG_ERROR([Scroom Threadpool library not found.])
fi   
if test "x${have_libscroom}" = "xno"
then
  AC_MSG_ERROR([Scroom library not found.])
fi   

if test "x${have_boost_thread_headers}" = xno
then
  AC_MSG_ERROR([Boost thread header files not found.])
fi
if test "x${have_boost_thread_lib}" = xno
then
  AC_MSG_ERROR([Boost thread library not found.])
fi

SCROOM_ENABLE_STDCXX_0X

AC_CONFIG_FILES([Makefile
                 doxyfile
])

# Misc defines
AC_SUBST([plugindir], [$libdir/scroom])

AC_OUTPUT


echo \
"-------------------------------------------------

 ${PACKAGE_NAME} version ${PACKAGE_VERSION}

 Prefix:   '${prefix}'.
 Compiler: '${CXX} ${CXXFLAGS} ${CPPFLAGS}'
           
 Package features:
   Doxygen:       ${want_doxygen}
     all code:    ${want_doxygen_all_code}
   Unittests:     ${want_boost_tests}

 Now type 'make @<:@<target>@:>@'
   where the optional <target> is:
     all                - build all binaries
     install            - install everything
     doxygen-doc        - generate documentation
                          (if enabled)
     check              - perform tests

--------------------------------------------------"


