/*
 * logging.c
 *
 * Voxi/Icepeak Logging functions
 *
 * (C) Copyright 2004 Voxi AB & Icepeak AB
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <voxi/util/err.h>
#include <voxi/util/logging.h>
#include <voxi/util/mem.h>

/*
 * Constants
 */

#define BUFFER_LENGTH 65536

/*
 * Type definitions
 */
typedef Error (*CreateFuncPtr)( LoggingDriver driver,
                                const char *applicationName, 
                                const char *parameters, Logger *logger );

typedef Error (*LogTextFuncPtr)( Logger logger, const char *moduleName, 
                                 LogLevel logLevel,
                                 const char *sourceFile, int sourceLine,
                                 const char *format, va_list args );

typedef struct sLoggingDriver
{
  CreateFuncPtr create;
  LogTextFuncPtr logText;
} sLoggingDriver;

typedef struct sLogger
{
  LoggingDriver driver;
  const char *applicationName;
  void *data;
} sLogger;

/*
 * static function prototypes 
 */
static Error fileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger );

static Error fileLogText( Logger logger, const char *moduleName, 
                          LogLevel logLevel, const char *sourceFile, 
                          int sourceLine, const char *format,
                          va_list args );
/*
 * Global variables
 */

static const char* LogLevelName[] = { "Critical", "Error", "Info", "Debug" };

static sLoggingDriver fileLoggingDriver = { fileCreate, fileLogText };
static sLogger sDefaultLogger = { &fileLoggingDriver, "unknown", NULL };
static Logger DefaultLogger = &sDefaultLogger;

LoggingDriver LoggingDriverFile = &fileLoggingDriver;

/*
 *  Code
 */
Error log_create( const char *applicationName, LoggingDriver driver, 
                  const char *driverArguments, Boolean setAsDefault,
                  Logger *logger )
{
  Error error;

  assert( driver != NULL );

  error = driver->create( driver, applicationName, driverArguments, logger );

  if( (error == NULL) && setAsDefault )
    DefaultLogger = *logger;

  return error;
}

Error log_logText( Logger logger, const char *moduleName, LogLevel logLevel, 
                   const char *sourceFile, int sourceLine,
                   const char *format, ... )
{
  Error error;
  va_list args;

  if( logger == NULL )
    logger = DefaultLogger;

  va_start( args, format );

  error = logger->driver->logText( logger, moduleName, logLevel, sourceFile, 
                                   sourceLine, format, args );

  va_end( args );

  return error;
}

/*
 * Implementation of the file logging driver
 */
static Error fileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger )
{
  Error error;

  assert( driver == LoggingDriverFile );

  error = emalloc( logger, sizeof( sLogger ) );
  if( error != NULL )
    return error;

  (*logger)->driver = driver;
  (*logger)->applicationName = strdup( appName );

  if( (args == NULL) || (strlen( args ) == 0) )
    (*logger)->data = stderr;
  else
  {
    (*logger)->data = fopen( args, "a" ); 
    if( (*logger)->data == NULL )
    {
      error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                      "Failed to open logging file '%s'", args );
      goto FAIL;
    }
  }

  return NULL;

FAIL:
  free( *logger );

  assert( error != NULL );

  return error;
}

static Error fileLogText( Logger logger, const char *moduleName, 
                          LogLevel logLevel, const char *sourceFile, 
                          int sourceLine, const char *format,
                          va_list args )
{
  char buffer[ BUFFER_LENGTH ];
  time_t now;
  int index, tempInt;

  /* Special case for the default logger */
  if( logger->data == NULL )
    logger->data = stderr;

  now = time( NULL ); /* check error control here */

  index = strftime( buffer, sizeof( buffer ), "%c", localtime( &now ) );
  assert( index > 0 ); /* Make better handling here */

  /* This is windows specific! is snprintf under Linux */
  tempInt = _snprintf( &(buffer[ index ]), sizeof( buffer ) - index, 
                      "\t%s\t%s\t%s\t%s:%d\t",
                      logger->applicationName, moduleName, 
                      LogLevelName[ logLevel ], sourceFile, sourceLine );
  assert( tempInt >= 0 );

  index += tempInt;

  /* Warning - no underscore on Linux */
  tempInt = _vsnprintf(  &(buffer[ index ]), sizeof( buffer ) - index, 
                        format, args );
  
  assert( tempInt >= 0 );

  tempInt = fputs( buffer, (FILE *) logger->data );
  assert( tempInt >= 0 ); /* fix error handling */

  fputc( '\n', (FILE *) logger->data );

  fflush( logger->data );

  return NULL;
}
