/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 *  @file win32_glue.h
 * 
 * Miscellaneous Unix/Linux functions needed by Voxi implemented using
 * the Win32 API.
 *
 * The functions here are defined only if we compile under win32.
 * On linux the functions are available from the standard libraries and
 * this header file is preprocessed into being empty.
 *
 * Numbers within parentheses indecate the man-section in linux man-pages
 * where documentation of the functions can be found.
 *
 *
 * @note
 *  not all of the functions below are in the POSIX-standard.
 *
 * $Id$
 *
 */ 

#ifndef WIN32_GLUE_H
#define WIN32_GLUE_H

#include <voxi/util/config.h>

#if defined(WIN32) || defined(DOXYGEN)

/*
 * Posix macros that are supported using various means.
 * We sort of mimick unistd.h in that respect here.
 */

/* Implemented here. */
#ifndef _POSIX_MAPPED_FILES
#define _POSIX_MAPPED_FILES
#endif

#ifndef _POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO
#endif

/* Not implemented here, we blatantly assume use of third-party
   pthreads. Even though these macros will indeed be defined in such an
   implementaton, pthread.h, semaphore.h etc are not included at all
   unless these macros are defined beforehand. (An old Swedish saying
   about hens and eggs comes to mind). All this is more nicely
   solved with autoconf or similar tools. Until we use such tools, we
   define the macros heere... */
#ifndef _POSIX_THREADS
#define _POSIX_THREADS
#endif

#ifndef _POSIX_SEMAPHORES
#define _POSIX_SEMAPHORES
#endif

/*  typedef int pid_t; */

/* Apparently winsock2.h must be included before windows.h or all hell breaks loose */
#include <winsock2.h> 
#include <windows.h> /* for HANDLE */
#include <process.h> /* defines getpid */
#include <io.h> /* defines close */
#include <voxi/util/libcCompat.h> /* to get strsep */

#ifdef __cplusplus
extern "C" {
#endif 

/*
 * Name translation of certain string comparison functions
 */
/*  #define strcasecmp(s1, s2) _stricmp((s1), (s2)) */
/*  #define strncasecmp(s1, s2, n) _strnicmp((s1), (s2), (n)) */

/** strcasecmp(3) */
#define strcasecmp stricmp

/** strncasecmp(3) */
#define strncasecmp strnicmp

/** snprintf(3) */
#define snprintf _snprintf


/** rint(3) */
EXTERN_UTIL double rint(double x);

/** 
 * gettimeofday(2) 
 * @note tz (timezone) is ignored. 
 */
struct timeval;
EXTERN_UTIL int gettimeofday(struct timeval *tv, void *tz);

/** sleep(3) */
/* Nuance defines sleep as Sleep( x * 1000 ) */
#ifndef sleep
EXTERN_UTIL void sleep(unsigned int sec);
#endif
/** usleep(3).
 * Has worse resolution: Mapped to Win32 Sleep(usec/1000). 
 */
EXTERN_UTIL void usleep(unsigned long usec);

/* mmap(2), munmap(2)
   A subset of their functionality.
   For all uses in the Voxi platform, however, we consider their
   implementation sufficient, and define the POSIX macros to indicate
   so.
*/
#include <sys/types.h>          /* off_t */
/** mmap(2) constant.  */
#define PROT_NONE 0
/** mmap(2) constant.  */
#define PROT_READ 1
/** mmap(2) constant.  */
#define PROT_WRITE 2
/** mmap(2) constant.  */
#define PROT_EXEC 4
/** mmap(2) constant.  */
#define MAP_SHARED 1
/** mmap(2) constant.  */
#define MAP_PRIVATE 2
/** mmap(2) constant.  */
#define MAP_FIXED 0x10
/** mmap(2) constant.  */
#define MAP_FAILED -1


/** The __attrbute__((stuff)) on gcc */
#define __attribute__(X)

/** ftime (3) */
#define ftime _ftime
#define timeb _timeb

/**
 * mmap(2). 
 *  A subset of the functionality.
 */ 
#define mmap(start, length, prot, flags, fd, offset) \
  mmap_win32(start, length, prot, flags, (HANDLE) _get_osfhandle(fd), offset)

// including <windows.h> cause all sorts of problems for erl on MSVC++ 6.0
// #include <windows.h>
// #include <windef.h>
EXTERN_UTIL void *mmap_win32(void *start, size_t length, int prot,
                 int flags, HANDLE hFile, off_t offset);

/**
 * munmap(2). 
 *  A subset of the functionality.
 */ 
EXTERN_UTIL int munmap(void *start, size_t length);

#endif /* WIN32 */

#ifdef __cplusplus
}
#endif 

#endif /* WIN32_GLUE_H */

