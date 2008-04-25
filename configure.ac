
# Process this file with autoconf to produce a configure script.
AC_INIT([voxiUtil],[1.4alpha2],[bugs@voxi.com],[util])

# insert this when things are stable.
AC_CONFIG_AUX_DIR(config)

#
# Check what platform we run on.
#
AC_CANONICAL_HOST
case $host in
  i*86-*-linux*)
    ;;
  x86_64-*-linux*)
    ;;
  i*86-*-cygwin*)
    ;;
  *)
    AC_MSG_ERROR([Unsupported platform: $host.])
    ;;
esac

#
# Make sure we use the right compiler and preprocessor.
#
AC_PROG_CC(cl gcc cc)
AC_PROG_CPP

case $host in
  i*86-*-linux*)
    ;;
  i*86-*-cygwin*)
    # On cygwin, we require direct use of MSVC command-line tools,
    # since we do not want dependencies on any cygwin DLLs in the
    # final binaries. I.e. we use cygwin as a "building shell" and
    # get "the best of two worlds".
    if test "x$CC" != "xcl" ; then
      AC_MSG_ERROR(
        [MSVC command-line tools (cl, link etc.) are required
         to build this package on Win32 (detected $CC).]
      )
    fi
    ;;
esac

#
# Collect LDFLAGS and LIBS in other variable names to
# avoid interference with AC macros. Will be collected at
# the end before generating makefiles etc.
#
LIBS_FINAL="$LIBS"
LDLAGS_FINAL="$LDFLAGS"

#
# Initialize Automake
#
# Requre automake version 1.7 or later, earlier versions give problems with 
# AM_CONFIG_HEADER
#
AM_INIT_AUTOMAKE(1.7) 
#
# Check the automake version
#
AC_MSG_CHECKING([automake version])
AUTOMAKE_VERSION=`automake --version | head -1 | awk '{print $4}'`
AC_MSG_RESULT($AUTOMAKE_VERSION)

#
# Initialize libtool, but not on Win32.
#
# AC_LIBTOOL_DLOPEN must be before AM_PROG_LIBTOOL
case $host in

  *-*-linux*)
    AM_CONDITIONAL([USE_LIBTOOL], 1)
    AC_ENABLE_SHARED
    AC_DISABLE_STATIC
    AC_LIBTOOL_WIN32_DLL
    AC_LIBTOOL_DLOPEN
    AC_PROG_LIBTOOL
    AM_PROG_LIBTOOL

    ;;

  i*86-*-cygwin*)
    AM_CONDITIONAL([USE_LIBTOOL], 0)
    ;;
esac

#
# This is really just the copyright of our configure scripts, not the code 
# itself
#
#AC_COPYRIGHT( "Copyright 2002, Voxi AB" )

AC_CONFIG_SRCDIR([src/bag.c])
AC_CONFIG_HEADER([config.h])

#
# This macro causes export/import definitions in .h files to
# be configured correctly when compiling.
#
# NOTE: If you copy this configure script to another source tree,
# change the macro name to comply with the module name.
#
CFLAGS="$CFLAGS -DLIB_UTIL_INTERNAL"

#
# This is for err.c to compile it for multithreaded environments.
# 
CFLAGS="$CFLAGS -D_REENTRANT"

#
# Check if the C preprocessor can handle variable argument lists to 
# preprocessor macros
#
AH_TEMPLATE( [HAVE_CPP_VARARGS], 
             [True if the C preprocessor can handle variable length argument \
              lists to macros.] )

AC_MSG_CHECKING([if the preprocessor handles variable argument lists])

AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#define VMACRO(x...) printf( ## x)
            VMACRO( "1", "2" );]])],[AC_DEFINE( [HAVE_CPP_VARARGS], 1 ) AC_MSG_RESULT([yes])],[AC_DEFINE( [HAVE_CPP_VARARGS], 0 ) AC_MSG_RESULT([no])])


#
# Check if the C compiler supports the "long long" datatype.
#
AH_TEMPLATE( [HAVE_LONG_LONG], 
             [Define to 1 if the C compiler supports the type long long.] )

AC_MSG_CHECKING([if the C compiler supports long long])

AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[long long int foo;]])],[AC_DEFINE( [HAVE_LONG_LONG], 1 ) AC_MSG_RESULT([yes])],[AC_DEFINE( [HAVE_LONG_LONG], 0 ) AC_MSG_RESULT([no])])


#
# Check if the C compiler supports the "__int64" datatype.
#
AH_TEMPLATE( [HAVE___INT64], 
             [Define to 1 if the C compiler supports the type __int64.] )

AC_MSG_CHECKING([if the C compiler supports __int64])

AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[__int64 foo;]])],[AC_DEFINE( [HAVE___INT64], 1 ) AC_MSG_RESULT([yes])],[AC_DEFINE( [HAVE___INT64], 0 ) AC_MSG_RESULT([no])])

#
# Check for openSSL / libcrypto
#
AC_ARG_WITH(
  openssl,
  [--with-openssl         Use openssl. Default is yes on Linux, no on Win32.
                          On Win32 you must give the path as argument value.],
)

AH_TEMPLATE( [HAVE_LIBCRYPTO], [If the crypto library exists] )

case $host in

  i*86-*-linux*)
    case $with_openssl in
      no)
        have_libcrypto=false
      ;;
      *)
        AC_CHECK_LIB([crypto], [OpenSSL_add_all_algorithms], 
                     have_libcrypto=true, have_libcrypto=false )
        if test "x$have_libcrypto" = "xtrue" ; then
          AC_DEFINE( [HAVE_LIBCRYPTO], 1 )
        fi
      ;;
    esac
  ;;

  i*86-*-cygwin*)
    case $with_openssl in
      no)
        have_libcrypto=false
      ;;
      yes)
        AC_MSG_WARN(You have to specify the path with --with-openssl on Win32)
        have_libcrypto=false
      ;;
      *)
        # TODO: Add checking code here. For now just assume that
        # the user is not lying.
        have_libcrypto=true
        AC_DEFINE( [HAVE_LIBCRYPTO], 1 )
        LIBS_FINAL="$LIBS_FINAL libeay32.lib"
        LDFLAGS_FINAL="$LDFLAGS_FINAL -libpath:${with_openssl}/lib"
        CPPFLAGS="$CPPFLAGS -I${with_openssl}/include"
    esac
  ;;

esac

case $have_libcrypto in
  false)
    AC_MSG_RESULT(Will use openssl: NO)
  ;;
  true)
    AC_MSG_RESULT(Will use openssl: $with_openssl)
  ;;
esac

AM_CONDITIONAL(HAVE_LIBCRYPTO, $have_libcrypto)

AM_CONDITIONAL(USE_LIBTOOL,true)
#
# Give location for pthreads.
#
AC_ARG_WITH(
  pthreads,
  [--with-pthreads        Speicfy where pthreads is, if not by default
                          accessible through OS. On Win32, we require
                          this to be the pthreads-win32 package.],
  [
    case $with_pthreads in
      no)
        AC_MSG_ERROR([Pthread support is required for this package.])
      ;;

      yes)
      ;;

      *)
        case $host in

          i*86-*-cygwin*)
            LIBS_FINAL="$LIBS_FINAL pthreadVSE.lib"
            LDFLAGS_FINAL="$LDFLAGS_FINAL -libpath:${with_pthreads}/DLL"
            CPPFLAGS="$CPPFLAGS -I${with_pthreads}/DLL"
          ;;

          *)
            LDFLAGS_FINAL="$LDFLAGS_FINAL -libpath:${with_pthreads}/lib"
            CPPFLAGS="$CPPFLAGS -I${with_pthreads}/include"
          ;;

        esac
      ;;

    esac
  ]
)
# Bit of a hack. This is a POSIX macro that we define here just
# to be sure (since we always use pthreads anyway).
CPPFLAGS="$CPPFLAGS -D_POSIX_THREADS"

#
# Checks for system libraries.
#
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([m], [rint])
AC_CHECK_LIB([pthread], [pthread_create])


#
# Checks for header files.
#
AC_HEADER_STDC

# Unix/Linux-related
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/socket.h sys/time.h sys/timeb.h unistd.h sys/types.h])

# Threading-related
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([semaphore.h], [], [], [#include <pthread.h>])

# Windows-related
AC_CHECK_HEADERS([windows.h winsock2.h io.h time.h])


#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM


#
# Checks for library functions.
#
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero floor gethostbyname gettimeofday inet_ntoa memset munmap rint sqrt strcasecmp strchr strdup strerror strpbrk strrchr strstr strtoul strsep])

#
# Debug build switch
#
AC_ARG_ENABLE(
  compile-debug,
  [  --enable-compile-debug      enable debug-compiled code],
  [
    AC_MSG_RESULT(debugging ENABLED)
    if test "x$CC" = "xgcc" ; then
      CFLAGS="$CFLAGS -g"
    fi
    if test "x$CC" = "xcl" ; then
      CFLAGS="$CFLAGS -Zi -Yd"
    fi
  ],
  [
    AC_MSG_RESULT(debugging disabled)
    CPPFLAGS="$CPPFLAGS -DNDEBUG"
    if test "x$CC" = "xgcc" ; then
      CFLAGS="$CFLAGS -O3"
    fi
    if test "x$CC" = "xcl" ; then
      CFLAGS="$CFLAGS -O2"
    fi
  ]
)

#
# File IO in certain places.
#
AH_TEMPLATE([_FILE_IO],
            [Whether to use File IO])

AC_DEFINE( [_FILE_IO], 1 )

#
# If we appear to be on Windows (use MSVC), some configuration
# definitions.

if test "x$CC" = "xcl" ; then

#
# define WIN32.
#
# NOTE:
#
# We should strive towards minimizing the use of this macro.  It
# is, however, needed to be able to produce Win32 DLL correctly since
# the DLL export/import definitions differ in a way not automatically
# configured by the libtool macros.
#
# Furthermore, we still have the win32_glue.[ch] stuff which of 
# course gradually should migrate to a generic compatibility layer.
# 
  CFLAGS="$CFLAGS -DWIN32"

#
# The pthreads-win32 library we currently use on Win32 has certain
# "features" we must take care of in voxi source. Unfortunately.
#
  CFLAGS="$CFLAGS -DPTHREADS_WIN32 -DNEED_SEM"

#
# Use with MT-safe libc (MSVCRT.DLL) on Win32
#
  CFLAGS="$CFLAGS -MD"

fi

#
# Collect all LIBS and LDFLAGS.
#
LIBS="$LIBS_FINAL"
LDFLAGS="$LDFLAGS_FINAL"

#
# Substitution macros in Makefiles etc.
#
AC_SUBST(CFLAGS)
AC_SUBST(PACKAGE_NAME)

#
# Generate the Makefiles
#
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
