# Process this file with autoconf to produce a configure script.
AC_INIT(voxiUtil, 1.4alpha, bugs@voxi.com, util)
# insert this when things are stable.
AC_CONFIG_AUX_DIR(config)
#
# Initialize Automake
#
AM_INIT_AUTOMAKE
#
# Initialize libtool
#
# AC_LIBTOOL_DLOPEN must be before AM_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

#
# This is really just the copyright of our configure scripts, not the code 
# itself
#
#AC_COPYRIGHT( "Copyright 2002, Voxi AB" )

AC_CONFIG_SRCDIR([src/bag.c])
AM_CONFIG_HEADER
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC(cl.exe gcc cc)

#
# Check if the C preprocessor can handle variable argument lists to 
# preprocessor macros
#
AH_TEMPLATE( [HAVE_CPP_VARARGS], 
             [True if the C preprocessor can handle variable length argument \
              lists to macros.] )

AC_MSG_CHECKING([if the preprocessor handles variable argument lists])

AC_TRY_CPP([#define VMACRO(x...) printf( ## x)
            VMACRO( "1", "2" );], 
           [AC_DEFINE( [HAVE_CPP_VARARGS], 1 ) AC_MSG_RESULT([yes])],
           [AC_DEFINE( [HAVE_CPP_VARARGS], 0 ) AC_MSG_RESULT([no])])

# Checks for libraries.
AC_CHECK_LIB([crypto], [OpenSSL_add_all_algorithms], 
             have_libcrypto=true, have_libcrypto=false )

AH_TEMPLATE( [HAVE_LIBCRYPTO], [If the crypto library exists] )
if test "x$have_libcrypto" = "xtrue" ; then
  AC_DEFINE( [HAVE_LIBCRYPTO], 1 )
fi

AM_CONDITIONAL(HAVE_LIBCRYPTO, $have_crypto)

AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([m], [rint])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/socket.h sys/time.h sys/timeb.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero floor gethostbyname gettimeofday inet_ntoa memset munmap rint sqrt strcasecmp strchr strdup strerror strpbrk strrchr strstr strtoul])

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
AC_SUBST(CFLAGS)

AH_TEMPLATE([_FILE_IO],
            [Whether to use File IO])

AC_DEFINE( [_FILE_IO], 1 )


AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
