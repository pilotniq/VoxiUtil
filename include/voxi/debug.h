/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
  @file debug.h

  Defines macros for printing messages. 
  
  When compiling with preprocessor flag NDEBUG defined, 
  the DEBUG-macro is defined to nothing.

  This file defines a static variable 'debug' that enables/disables
  printout of DEBUG-messages. The initial value of the variable DEBUG
  is set by defining the macro DEBUG_MESSAGES to 1 or 0 before the inclusion 
  of voxi/debug.h. If DEBUG_MESSAGES is undefined it will be set to 0. 
  
  Typically voxi/debug.h is included indirectly via voxi/alwaysInclude.h.


  @note
   Using MSVC we cannot have variable arguments lists in macros.
   (MSVC does not follow the ISO C99 standard in this respect).
   Instead, use inline functions that become empty when NDEBUG is
   defined. This should allow the compiler to remove the calls to the
   functions completely when compiling binaries without debug code.
   \n
   Sadly, this approach makes the beautiful method of using __FILE__,
   __LINE__, etc pointless, since those will invariably refer to
   debug.h and not to the file and line where the function was
   actually used. Exercise for the intrepid debugger: Find a
   workaround!
  
  $Id$
*/


#ifndef DEBUG_H
#define DEBUG_H


/**
  @var static int debug  
  variable controlling whether or not the DEBUG-messages will be printed.

  Only applicable if compiled with NDEBUG being undefined. 
  The preprocessor constant DEBUG_MESSAGES sets the initial value
  of the debug-variable.

  @note The info about this variable is not printed in the html-documentation.
  It seems to be because of the 'static'-modifier. Fix this somehow.. 
*/

#ifndef DEBUG_MESSAGES
#define DEBUG_MESSAGES 0
#endif

#ifndef NDEBUG
static int debug = DEBUG_MESSAGES;
#define BUILD_INFO_STR "debug build"
#else
#define BUILD_INFO_STR "release build"
#endif /* NDEBUG */

#ifndef WIN32

/* For any ISO C99 compliant compiler the following macros are correct. */

#ifndef NDEBUG
#include <stdio.h>

#ifdef _POSIX_THREADS
#include <pthread.h>

/** @def DEBUG(x...) printf-like output that can be turned of via the static variable debug

    but with info about file, calling function, thread-id and line-number
*/
#define DEBUG(x...) \
 do if(debug!=0){\
     fprintf(stderr, "DEBUG: %s, %s[%ld]:%d: ", \
             __FILE__, \
             __PRETTY_FUNCTION__, \
             (long)pthread_self(),\
             __LINE__);\
     fprintf(stderr, ## x);\
 } while(0)
#else /* if POSIX_THREADS ... else */
#define DEBUG(x...) \
 do if(debug!=0){\
     fprintf(stderr, "DEBUG: %s, %s:%d: ", \
             __FILE__, \
             __PRETTY_FUNCTION__, \
             __LINE__);\
     fprintf(stderr, ## x);\
 } while(0)
#endif /* if POSIX_THREADS ... else ... */
#else  /* Debugging disabled */
#define DEBUG(x...)
#endif

/** @def DIAG(x...) printf-like output

    but with info about file, calling function, thread-id and line-number
*/
#define DIAG(x...) \
 do {\
     fprintf(stderr, "%s, %s[%ld]:%d: ", \
             __FILE__, \
             __PRETTY_FUNCTION__, \
             (long)pthread_self(),\
             __LINE__);\
     fprintf(stderr, ## x);\
 } while(0)


#else  /* WIN32 */

/**
   Kludge since MSVC does allow variable number of args and hence does
   not follow the ISO C99 standard. Instead we use inline-functions. See note
   in the head.
*/
#ifndef NDEBUG
#include <stdio.h>
#include <stdarg.h>
#endif /* NDEBUG */

#ifndef NDEBUG
static void DEBUG(char *str, ...)
{
  if(debug) {
    va_list args;
    va_start(args, str);
    fprintf(stderr, "DEBUG: ");
    vfprintf(stderr, str, args);
    va_end(args);
  } 
}

static void DIAG(char *str, ...)
{
    va_list args;
    va_start(args, str);
    fprintf(stderr, "DIAG: ");
    vfprintf(stderr, str, args);
    va_end(args);
}

#else  /* Debugging disabled */
__inline void DEBUG(char *str, ...) {}
__inline void DIAG(char *str, ...) {}
#endif /* NDEBUG */

#endif /* WIN32 */

#endif /* DEBUG_H */
