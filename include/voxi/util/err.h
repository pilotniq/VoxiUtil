/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
  @file err.h

  General purpose error handling.

  This file contains interfaces for two different 
  error handling schemes. 

  The old one passes the call-trace downwards and prints or aborts 
  if an error ocurrs. This error handling is only used in older code.

  The new one is more similar to exceptions and relies on callers
  to pass the errors upwards and append more context specific error info
  if they have any. 

  @remark
   the new error handling uses the old error handling in its 
   implementation.
*/

/*
  Based on the ProgK project by the Hemuls:
    Interface to the Hemuls error functions
    (Which have never even been tested, because we just can't make errors!)

  $Id$
  */

#ifndef ERR_H
#define ERR_H

#ifdef __cplusplus
extern "C" {
#endif

#define ERR Err(__FILE__, __LINE__,
#define ENDERR )

/**
 * Error categories for the old error handling
 */
typedef enum { ERR_WARN, ERR_ABORT } Err_Action;


/**
 * pointer to Error struct for the new error handling
 */
typedef /*@null@*/struct s_Error *Error;
typedef const struct s_Error *ConstError;

/**
   Predefined error types for the new error handling.
 */
/*
  Make sure to add any new error types defined here to the Java class
  se.voxi.common.err.VoxiException
*/
typedef enum { ERR_UNKNOWN, ERR_ERRNO, ERR_OSERR, ERR_SOCK, ERR_APP, ERR_SND, 
               ERR_MAC, ERR_FFT, ERR_BROKER, ERR_MEMORY, ERR_SHLIB, 
               ERR_THREADING, 
               ERR_TEXTRPC, ERR_STRBUF, ERR_LOOP, ERR_ISI_IDE, ERR_OOW,
               ERR_PARSER, ERR_JAVA_GENERIC, ERR_COOWA, ERR_WINSOCK,
               ERR_SPEECH_RECOGNITION, ERR_HTTP, ERR_IVR, ERR_WIN32, 
               ERR_DRIVER, ERR_NUANCE, ERR_STATE_MACHINE, ERR_LOGGING, 
               ERR_LLT, ERR_DIALOGIC_GLOBAL_CALL, ERR_DIALOGIC_CALL_CONTROL,
               ERR_DIALOGIC_DX, ERR_QUEUE, ERR_SSL
} ErrType;

#include <voxi/types.h>
#include <voxi/util/strbuf.h>

#include <string.h> /* Needed for strerror */
#include <errno.h>

#include <voxi/util/libcCompat.h>

/*
 * Macros
 */
#define ERR_GOTO( func, label ) \
  { \
    error = (func); \
    if( error != NULL ) \
      goto label; \
  }

#define ERR_REPORT_AND_IGNORE( func, wrapMessage ) \
  { \
    Error error; \
    \
    error = (func); \
    if( error != NULL ) \
    { \
      error = ErrNew( ERR_APP, 0, error, "%s:%d: %s", __FILE__, __LINE__, \
                      (const char *) (wrapMessage) ); \
      \
      ErrReport( error ); \
      ErrDispose( error, TRUE ); \
    } \
  }


/**
 * Initalize the old error handling
 */
EXTERN_UTIL Boolean ErrInit(void);

/**
 * Push an entry on the caller stack in the old error handling.
 */ 
EXTERN_UTIL void ErrPushFunc(char *funcname, ...);

/**
 * Pop an entry from the caller stack in the old error handling.
 */ 
EXTERN_UTIL void ErrPopFunc(void);

/**
 * Create an error message in the old error handling.
 */ 
EXTERN_UTIL void Err(char *file, unsigned int line, Err_Action action,
                     char *string, ...);

/** 
 * Toggle trace-printouts
 *
 * If TRUE, all calls to ErrPushFunc and ErrPopFunc prints
 * out the arguemts they were called with. 
 */
EXTERN_UTIL Boolean DisplayTrace(Boolean newState);


/**
 * Create a new error object in the new error handling.
 *
 * @param t the error category. 
 * @param number a category specific identifier.
 * @param reason a sub-error that was the cause of this error.
 * @param description a printf-style format string. Followed by
 *        appropriate arguements. 
 *
 */ 
#if 0
#define ErrNew( type, number, reason, description, x... ) \
  ErrNew2( (type), (number), (reason), __FILE__, __LINE__, (description), ## x )

EXTERN_UTIL Error ErrNew2( ErrType t, int number, /*@only@*/Error reason, 
                           const char *file, int line, 
                           const char *description, ...);

#else
EXTERN_UTIL Error ErrNew( ErrType t, int number, /*@only@*/Error reason, 
                          const char *description, ...);
#endif

/**
 * Create a copy of an error
 */
EXTERN_UTIL Error ErrCopy( ConstError originalError, 
                           Error *errorCopy );

/**
 * Free an error object in the new error handling.
 *
 * @param err the error object
 * @param recursive whether or not to free all the sub-errors as well-
 */ 
EXTERN_UTIL void ErrDispose(Error err, Boolean recursive);

/*
 *  Error Reporting functions (greatly expanded 2003-10-10)
 */

/*
 *  predefined drivers are:
 *    "file", parameter is complete pathname
 *    "stdout", noparameter, prints to stdout 
 *    "stderr", no parameter, prints to stderr (Default)
 *    "mail", parameters: smtpserver, sending mail address, receiving mail address
 *    
 */
EXTERN_UTIL void ErrReportAddDestination( const char *driver, ... );

/**
 * Print the error object on stderr (in the new error handling).
 */
EXTERN_UTIL void ErrReport( ConstError error );

EXTERN_UTIL Error ErrToHumanReadableString( ConstError error, char **string );

/**
 * get error-category (in the new error handling)
 */
EXTERN_UTIL ErrType Err_getType( ConstError err);
/**
 * get category specific identifier (in the new error handling)
 */
EXTERN_UTIL int Err_getNum( ConstError err);

/**
 * convenience macro.
 * 
 * Create a new voxi-error from errors in libraries that are returned
 * in the global variable errno and that have a description that can be 
 * obtained by calling the POSIX function strerror.
 */
#define ErrErrno() ErrNew( ERR_ERRNO, errno, NULL, "%s", strerror( errno ) )

/**
  Converts an error to a string representation. 

  The representation is:
  
  Error( <type> <number> <description> <reason> )
  
  It is intended for marshalling purposes rather than for human consumption.
*/
EXTERN_UTIL Error ErrToString( ConstError error, StringBuffer strbuf );

#ifdef WIN32
/*
   ErrWin32() - calls Window's GetLastError() and then FormatMessage, 
   to return a cleartext error.
   */
EXTERN_UTIL Error ErrWin32();
#endif

#ifdef MACOS
#include <Types.h>

EXTERN_UTIL const char *ErrOSErrName(OSErr err);
#define  ErrCheckMacErr( err )  \
     (err == 0) ? NULL : ErrNew( ERR_OSERR, (int) err, ErrOSErrName(err), NULL)

#define ErrCreateOSErr( /* OSErr */ err, /* Error */ reason) \
                        ErrNew(ERR_OSERR, (int) err, ErrOSErrName(err), reason)
#endif /* MACOS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ifndef ERR_H */

