#ifndef LIBCCOMPAT_H
#define LIBCCOMPAT_H

#ifdef __cplusplus
extern "C" {
#endif


/*
  C library (in)compatibility stuff
*/

/* This is because on Windows, when somebody else uses this as a DLL,
   we must declare external variables with a special directive.
   Note: Since the EXTERN macro may be redefined in other .h files, the
   following macro sequence must occur after any other inclusion
   made in this .h file. */

#ifdef EXTERN_UTIL
#  error EXTERN_UTIL already defined
#endif
#ifdef WIN32
#  ifdef LIB_UTIL_INTERNAL
#    define EXTERN_UTIL __declspec(dllexport)
#    ifdef LIB_UTIL_LOGGING_INTERNAL
#      define LOGGING_EXTERN __declspec(dllexport)
#    else
#      define LOGGING_EXTERN extern
#    endif
#  else
#    define EXTERN_UTIL __declspec(dllimport)
#    define LOGGING_EXTERN __declspec(dllimport)
#  endif /* LIB_UTIL_INTERNAL */
#else
#  define EXTERN_UTIL extern
#  define LOGGING_EXTERN extern
#endif /* WIN32 */

/*
 * Various naming brain damage on Win32.
 */
#ifdef WIN32
  /* String functions */
#define snprintf _snprintf
#define vsnprintf _vsnprintf
  /* Time, ... */
#define ftime _ftime
#define timeb _timeb
#endif /* WIN32 */

/* We implement this, since it is better than strtok (doesn't modify
   the actual string data, only the pointer). See Linux man pages
   strsep(3) and strtok(3) for further info.

   The strsep in Linux (as of RedHat 6.2) behaves slightly different
   from what is said in the man pages: When no delimiters are found,
   it returns a pointer to the string while the man page sais it
   should return NULL. To mimick this (mis)behaviour, define the
   STRSEP_LINUX_BEHAVIOUR macro below when compiling win32_glue.c.
*/

/* The __attribute__ directive is used in GCC to specify alignment of 
   struct members */
#ifndef __GNUC__
#define __attribute__(x)
#endif

#if !defined( HAVE_STRSEP) || !HAVE_STRSEP

#define STRSEP_LINUX_BEHAVIOUR
/** strsep(3) */
EXTERN_UTIL char *strsep(char **stringp, const char *delim);

#endif

#ifdef __cplusplus
}
#endif


#endif /* ifndef LIBCCOMPAT_H */
