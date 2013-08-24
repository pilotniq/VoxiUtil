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
typedef enum { LOGLEVEL_NONE, LOGLEVEL_CRITICAL, LOGLEVEL_ERROR, 
               LOGLEVEL_WARNING,
               LOGLEVEL_INFO, LOGLEVEL_DEBUG, LOGLEVEL_TRACE,
               NUMBER_OF_LOGLEVELS } LogLevel;

/*
 * LOGFORMAT_LEGACY:
 * Timestamp Application Module Level [thread] File:Line Message
 *
 * LOGFORMAT_STANDARD (default):
 * Timestamp Level [process;thread] Application/Module File:Line Message
 */
typedef enum { LOGFORMAT_LEGACY, LOGFORMAT_STANDARD,
               NUMBER_OF_LOGFORMATS} LogFormat;
    

/* The possible error codes */
enum { ERR_LOGGING_UNSPECIFIED };

typedef struct sLoggingDriver *LoggingDriver;
typedef struct sLogger *Logger;

/*
 * Global variables
 */

/*
  The LOGGING_EXTERN macro is defined in libcCompat.h, and modified by 
  LIB_UTIL_LOGGING_INTERNAL which is defined before the Voxi includes in logging.c

  LOGGING_EXTERN must be used before global variables defined in this file.
*/

/* 
   The file logging driver takes a filename as parameter. If the filename is
   NULL or the empty string, errors will be written to stderr instead of to a 
   file.
   */
LOGGING_EXTERN LoggingDriver LoggingDriverFile;

/* 
   The DualFile logging driver takes a filename as parameter.

   The log will be written into two logfiles:
   - A Common log for all logging messages.
   - An Error log for warning, error or critical logging messages.

   The file names are deduced from the name given as parameter:
   'Logname.txt' --> 'Logname_Common.txt' and 'Logname_Error.txt'

   If no extension is given in the parameter, the extension '.log'
   will be used.  The logs will be rotated at each day shift. Old logs
   will be moved to file named 'Logname_Common_YYYY-MM-DD.txt', etc,
   and erased after one week if the process is running then.
   
   */
LOGGING_EXTERN LoggingDriver LoggingDriverDualFile;

LOGGING_EXTERN const char* LogLevelName[NUMBER_OF_LOGLEVELS];
  
/* This is the global log level, defined in logging.c
   The macros need to access it.
   It should be renamed to GlobalLogLevel to make everything clear.
*/  
LOGGING_EXTERN LogLevel _voxiUtilGlobalLogLevel;
  
/*
 * Public function prototypes
 */
EXTERN_UTIL Error log_create( const char *applicationName, 
                              LoggingDriver driver, 
                              const char *driverArguments, 
                              Boolean setAsDefault, 
                              Logger *logger );
EXTERN_UTIL void log_destroy( Logger logger );

EXTERN_UTIL void log_setFormat( Logger logger, LogFormat logFormat );

/* Logger may be NULL, in which case the default logger is used */
EXTERN_UTIL Error log_logText( Logger logger, const char *logDestination,
                               const char *moduleName, 
                               LogLevel logLevel, 
                               const char *sourceFile, int sourceLine,
                               const char *format, ... );

EXTERN_UTIL Error log_logError( Logger logger, const char *logDestination,
                                const char *moduleName, 
                                LogLevel logLevel, 
                                const char *sourceFile, int sourceLine,
                                ConstError error );

/* 
  This function does nothing, then returns NULL. Hack to work around
   C preprocessors that don't support variable argument lists.
*/
EXTERN_UTIL Error log_noLogText( Logger logger, const char *logDestination,
                                 const char *moduleName,
                                 LogLevel logLevel, const char *sourceFile,
                                 int sourceLine, const char *format, ... );
  
EXTERN_UTIL Error log_noLogError( Logger logger, const char *logDestination,
                                  const char *moduleName, 
                                  LogLevel logLevel, 
                                  const char *sourceFile, int sourceLine,
                                  ConstError error );

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
 *
 * The LOG_MODULE_LOGGER_SET macro may be called to use a different logger
 * than the default logger within a module. Example:
 *
 *   Logger myLogger;
 *   err = log_create(..., &myLogger);
 *   if (err == NULL) {
 *     LOG_MODULE_LOGGER_SET(myLogger);
 *   }
 *   else { ... }
 *
 * To use the default logger again, use:
 *
 *   LOG_MODULE_LOGGER_SET(NULL);
 *
 */
  
EXTERN_UTIL LogLevel log_GlobalLogLevelSet(LogLevel level);
EXTERN_UTIL LogLevel log_GlobalLogLevelGet();

#define LOG_GLOBAL_LEVEL_SET( level) \
  log_GlobalLogLevelSet( level );
  
#define LOG_MODULE_LEVEL_SET( level ) \
  _voxiUtilModuleLogLevel = (level);

#define LOG_MODULE_LOGGER_SET(logger) \
  _voxiUtilModuleLogger = logger;
  
#define LOG_MODULE_DESTINATION_SET(destination) \
  _voxiUtilLogModuleDestination = (destination);
  
#define LOG_GLOBAL_LEVEL_GET() log_GlobalLogLevelGet()
#define LOG_MODULE_LEVEL_GET() (_voxiUtilModuleLogLevel)

#define LOG_MODULE_SET(moduleName) \
  static char *_voxiUtilLogModuleName = (moduleName)

// Declare a module.
// Messages will end up in the default logger destination (file).
#define LOG_MODULE_DECL(moduleName, defaultLevel) \
  static Logger _voxiUtilModuleLogger = NULL; \
  static LogLevel _voxiUtilModuleLogLevel = (defaultLevel); \
  static char *_voxiUtilLogModuleName = (moduleName); \
  static char *_voxiUtilLogModuleDestination = ""

// Declare a module.
// Specify a separate destination. The default logger will direct
// messages to this destination for logg messages in this module.
// For a file-based logger, this would be a file name.
#define LOG_MODULE_DECL_OWN_DEST(moduleName, logDestination, defaultLevel) \
  static Logger _voxiUtilModuleLogger = NULL; \
  static LogLevel _voxiUtilModuleLogLevel = (defaultLevel); \
  static char *_voxiUtilLogModuleName = (moduleName); \
  static char *_voxiUtilLogModuleDestination = (logDestination)

#define LOG_DESTINATION_CONSOLE "[console]"

#define LOG( condition ) ((condition) ? log_logText : log_noLogText)
#define LOGERR( condition ) ((condition) ? log_logError : log_noLogError)

#define LOG_CRITICAL \
   LOG( (_voxiUtilGlobalLogLevel >= LOGLEVEL_CRITICAL) || \
        (_voxiUtilModuleLogLevel >= LOGLEVEL_CRITICAL))
#define LOG_CRITICAL_ARG \
   _voxiUtilModuleLogger, _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_CRITICAL, __FILE__, __LINE__
#define LOGGER_CRITICAL_ARG \
   _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_CRITICAL, __FILE__, __LINE__

#define LOG_ERROR \
   LOG( (_voxiUtilGlobalLogLevel >= LOGLEVEL_ERROR) || \
        (_voxiUtilModuleLogLevel >= LOGLEVEL_ERROR) )
#define LOG_ERROR_ARG \
   _voxiUtilModuleLogger, _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_ERROR, __FILE__, __LINE__
#define LOGGER_ERROR_ARG \
   _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_ERROR, __FILE__, __LINE__

#define LOG_WARNING \
   LOG( (_voxiUtilGlobalLogLevel >= LOGLEVEL_WARNING ) || \
        (_voxiUtilModuleLogLevel >= LOGLEVEL_WARNING) )
#define LOG_WARNING_ARG \
   _voxiUtilModuleLogger, _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_WARNING, __FILE__, __LINE__
#define LOGGER_WARNING_ARG \
   _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_WARNING, __FILE__, __LINE__

#define LOG_INFO \
   LOG( (_voxiUtilGlobalLogLevel >= LOGLEVEL_INFO ) || \
        (_voxiUtilModuleLogLevel >= LOGLEVEL_INFO))
#define LOG_INFO_ARG \
   _voxiUtilModuleLogger, _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_INFO, __FILE__, __LINE__
#define LOGGER_INFO_ARG \
   _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_INFO, __FILE__, __LINE__

#define LOG_DEBUG \
   LOG( (_voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG ) || \
        (_voxiUtilModuleLogLevel >= LOGLEVEL_DEBUG))
#define LOG_DEBUG_ARG \
   _voxiUtilModuleLogger, _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_DEBUG, __FILE__, __LINE__
#define LOGGER_DEBUG_ARG \
   _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_DEBUG, __FILE__, __LINE__

#define LOG_TRACE \
   LOG( (_voxiUtilGlobalLogLevel >= LOGLEVEL_TRACE) || \
        (_voxiUtilModuleLogLevel >= LOGLEVEL_TRACE))
#define LOG_TRACE_ARG \
   _voxiUtilModuleLogger, _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_TRACE, __FILE__, __LINE__
#define LOGGER_TRACE_ARG \
   _voxiUtilLogModuleDestination, _voxiUtilLogModuleName, LOGLEVEL_TRACE, __FILE__, __LINE__

#define LOGERR_INFO \
   LOGERR( (_voxiUtilGlobalLogLevel >= LOGLEVEL_INFO) || \
           (_voxiUtilModuleLogLevel >= LOGLEVEL_INFO) )

#define LOGERR_DEBUG \
   LOGERR( (_voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG) || \
           (_voxiUtilModuleLogLevel >= LOGLEVEL_DEBUG) )

#define LOGERR_WARNING \
   LOGERR( (_voxiUtilGlobalLogLevel >= LOGLEVEL_WARNING) || \
           (_voxiUtilModuleLogLevel >= LOGLEVEL_WARNING) )

#define LOGERR_ERROR \
   LOGERR( (_voxiUtilGlobalLogLevel >= LOGLEVEL_ERROR) || \
           (_voxiUtilModuleLogLevel >= LOGLEVEL_ERROR) )

#define LOGERR_CRITICAL \
   LOGERR( (_voxiUtilGlobalLogLevel >= LOGLEVEL_CRITICAL) || \
           (_voxiUtilModuleLogLevel >= LOGLEVEL_CRITICAL) )


/*
Error log_logError( const char *moduleName, LogLevel logLevel,
                    const char *sourceFile, int sourceLIne,
                    Error error );
*/

#ifdef __cplusplus
}
#endif

#endif
