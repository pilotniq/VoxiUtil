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
               LOGLEVEL_DEBUG, NUMBER_OF_LOGLEVELS } LogLevel;

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

EXTERN_UTIL const char* LogLevelName[NUMBER_OF_LOGLEVELS];

/*
 * Public function prototypes
 */
EXTERN_UTIL Error log_create( const char *applicationName, LoggingDriver driver, 
                  const char *driverArguments, Boolean setAsDefault, 
                  Logger *logger );
EXTERN_UTIL void log_destroy( Logger logger );

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

/*
 * Convenience macros.
 *
 * Use LOG_MODULE_DECL() in a module to declare the module name
 * and initial logging level to be used in that module. Example:
 *
 *   LOG_MODULE_DECL("MyModule", LOGLEVEL_INFO);
 *
 *
 * Then, use the LOG_CRITICAL, LOG_ERROR, LOG_INFO, LOG_DEBUG, and
 * LOGERR_ERROR macros for logging. Examples:
 *
 *   LOG_ERROR(LOG_ERROR_ARG, "Error %d\n", 17);
 *
 *   LOG_INFO(LOG_INFO_ARG, "Hello, world\n");
 *
 *
 * LOGERR_ERROR and LOGERR_CRITICAL are special cases for logging an
 * Error structure. Example:
 *
 *   Error error = ErrNew( ... );
 *   ...
 *   LOGERR_ERROR(LOG_ERROR_ARG, error);
 *   ...
 *   ErrDispose(error, ...);
 *
 */
  
EXTERN_UTIL LogLevel _voxiUtilLogLevel;

EXTERN_UTIL LogLevel log_LogLevelSet(LogLevel level);
EXTERN_UTIL LogLevel log_LogLevelGet();

#define LOG_LEVEL_SET(Level) \
  log_LogLevelSet(Level)
  
#define LOG_LEVEL_GET() log_LogLevelGet()

#define LOG_MODULE_SET(moduleName) \
  static char *_voxiUtilLogModuleName = (moduleName)
  
#define LOG_MODULE_DECL(moduleName, defaultLevel) \
  static LogLevel _voxiUtilLogLevelDummy = log_LogLevelSet(defaultLevel); \
  static char *_voxiUtilLogModuleName = (moduleName)

#define LOG( condition ) ((condition) ? log_logText : log_noLogText)
#define LOGERR( condition ) ((condition) ? log_logError : log_noLogError)

#define LOG_CRITICAL \
   LOG( _voxiUtilLogLevel >= LOGLEVEL_CRITICAL )
#define LOG_CRITICAL_ARG \
   NULL, _voxiUtilLogModuleName, LOGLEVEL_CRITICAL, __FILE__, __LINE__

#define LOG_ERROR \
   LOG( _voxiUtilLogLevel >= LOGLEVEL_ERROR )
#define LOG_ERROR_ARG \
   NULL, _voxiUtilLogModuleName, LOGLEVEL_ERROR, __FILE__, __LINE__

#define LOG_INFO \
   LOG( _voxiUtilLogLevel >= LOGLEVEL_INFO )
#define LOG_INFO_ARG \
   NULL, _voxiUtilLogModuleName, LOGLEVEL_INFO, __FILE__, __LINE__

#define LOG_DEBUG \
   LOG( _voxiUtilLogLevel >= LOGLEVEL_DEBUG )
#define LOG_DEBUG_ARG \
   NULL, _voxiUtilLogModuleName, LOGLEVEL_DEBUG, __FILE__, __LINE__

#define LOGERR_ERROR \
   LOGERR( _voxiUtilLogLevel >= LOGLEVEL_ERROR )

#define LOGERR_CRITICAL \
   LOGERR( _voxiUtilLogLevel >= LOGLEVEL_CRITICAL )


/*
Error log_logError( const char *moduleName, LogLevel logLevel,
                    const char *sourceFile, int sourceLIne,
                    Error error );
*/

#ifdef __cplusplus
}
#endif

#endif
