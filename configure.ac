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
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([crypto], [OpenSSL_add_all_algorithms])
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
                CFLAGS="$CFLAGS -g"
	],
        [
		AC_MSG_RESULT(debugging disabled)
                CPPFLAGS="$CPPFLAGS -DNDEBUG"
                CFLAGS="$CFLAGS -O3"
        ]
)
AC_SUBST(CFLAGS)

AH_TEMPLATE([_FILE_IO],
            [Whether to use File IO])

AC_DEFINE( [_FILE_IO], 1 )


AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
