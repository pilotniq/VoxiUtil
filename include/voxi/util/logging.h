/*
 * logging.h
 *
 * Voxi/Icepeak Logging functions
 *
 * (C) Copyright 2004 Icepeak AB & Voxi AB
 */

#ifndef VOXIUTIL_LOGGING_H
#define VOXIUTIL_LOGGING_H

#include <voxi/util/libcCompat.h>
#include <voxi/util/config.h>
#include <voxi/util/err.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Macros and constants
 */
#define LOG_FILE_LINE __FILE__, __LINE__

/*
 * Type definitions
 */
typedef enum { LOGLEVEL_CRITICAL, LOGLEVEL_ERROR, LOGLEVEL_INFO, 
               LOGLEVEL_DEBUG } LogLevel;

/* The possible error codes */
enum { ERR_LOGGING_UNSPECIFIED };

typedef struct sLoggingDriver *LoggingDriver;
typedef struct sLogger *Logger;

/*
 * Global variables
 */

/* 
   The file logging driver takes a filename as parameter. If the filename is
   NULL or the empty string, errors will be written to stderr instead of to a 
   file.

   */
EXTERN_UTIL LoggingDriver LoggingDriverFile;

/*
 * Public function prototypes
 */
EXTERN_UTIL Error log_create( const char *applicationName, LoggingDriver driver, 
                  const char *driverArguments, Boolean setAsDefault, 
                  Logger *logger );

/* Logger may be NULL, in which case the default logger is used */
EXTERN_UTIL Error log_logText( Logger logger, const char *moduleName, 
                               LogLevel logLevel, 
                               const char *sourceFile, int sourceLine,
                               const char *format, ... );

EXTERN_UTIL Error log_logError( Logger logger, const char *moduleName, 
                               LogLevel logLevel, 
                               const char *sourceFile, int sourceLine,
                               Error error );

/* 
  This function does nothing, then returns NULL. Hack to work around
   C preprocessors that don't support variable argument lists.
*/
EXTERN_UTIL Error log_noLogText( Logger logger, const char *moduleName,
                                 LogLevel logLevel, const char *sourceFile,
                                 int sourceLine, const char *format, ... );
  
EXTERN_UTIL Error log_noLogError( Logger logger, const char *moduleName, 
                                  LogLevel logLevel, 
                                  const char *sourceFile, int sourceLine,
                                  Error error );


#define LOG( condition ) ((condition) ? log_logText : log_noLogText)
#define LOGERR( condition ) ((condition) ? log_logError : log_noLogError)


/*
Error log_logError( const char *moduleName, LogLevel logLevel,
                    const char *sourceFile, int sourceLIne,
                    Error error );
*/

#ifdef __cplusplus
}
#endif

#endif
