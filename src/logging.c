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
#include <sys/timeb.h>
#include <pthread.h>

#include <voxi/util/err.h>
#include <voxi/util/logging.h>
#include <voxi/util/mem.h>
#include <voxi/util/path.h>
#include <voxi/util/libcCompat.h>

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
typedef void (*DestroyFuncPtr)( Logger logger );
typedef Error (*LogTextFuncPtr)( Logger logger, const char *moduleName, 
                                 LogLevel logLevel,
                                 const char *sourceFile, int sourceLine,
                                 const char *format, va_list args );

typedef struct sLoggingDriver
{
  CreateFuncPtr create;
  DestroyFuncPtr destroy;
  LogTextFuncPtr logText;
} sLoggingDriver;

typedef struct sLogger
{
  LoggingDriver driver;
  const char *applicationName;
  pthread_mutex_t mutex;
  void *data;
  const char *logFileName;
  char date[15];
} sLogger;

/*
 * static function prototypes 
 */
static Error fileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger );

static void fileDestroy( Logger logger );

static Error fileLogText( Logger logger, const char *moduleName, 
                          LogLevel logLevel, const char *sourceFile, 
                          int sourceLine, const char *format,
                          va_list args );
/*
 * Global variables
 */

const char* LogLevelName[NUMBER_OF_LOGLEVELS] = {
  "None", "Critical", "Error", "Warning", "Info", "Debug", "Trace"
};

static sLoggingDriver fileLoggingDriver = { fileCreate, fileDestroy, fileLogText };

static sLogger sDefaultLogger = {
  &fileLoggingDriver,
  "unknown",
  PTHREAD_MUTEX_INITIALIZER,
  NULL,
  NULL,
  "2004-12-24"
};

static Logger DefaultLogger = &sDefaultLogger;

LoggingDriver LoggingDriverFile = &fileLoggingDriver;

LogLevel _voxiUtilGlobalLogLevel = LOGLEVEL_NONE;

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

void log_destroy( Logger logger )
{
  logger->driver->destroy( logger );
}

Error log_logText( Logger logger, const char *moduleName, LogLevel logLevel, 
                   const char *sourceFile, int sourceLine,
                   const char *format, ... )
{
  Error error;
  va_list args;
  const char *filename;

  if( logger == NULL )
    logger = DefaultLogger;

  va_start( args, format );

  filename = strrchr( sourceFile, DIR_DELIM );
  filename = (filename == NULL) ? sourceFile : (filename + 1);

  error = logger->driver->logText( logger, moduleName, logLevel, filename, 
                                   sourceLine, format, args );

  va_end( args );

  return error;
}

Error log_logError( Logger logger, const char *moduleName, LogLevel logLevel, 
                    const char *sourceFile, int sourceLine, Error error )
{
  char *errorMessage;
  Error internalError;

  internalError = ErrToHumanReadableString( error, &errorMessage );
  if( internalError != NULL )
    return internalError;

  log_logText( logger, moduleName, logLevel, sourceFile, sourceLine, "%s", 
                errorMessage );

  free( errorMessage );

  return NULL;
}

Error log_noLogText( Logger logger, const char *moduleName,
                              LogLevel logLevel, const char *sourceFile,
                              int sourceLine, const char *format, ... )
{
  return NULL;
}

Error log_noLogError( Logger logger, const char *moduleName, 
                      LogLevel logLevel, 
                      const char *sourceFile, int sourceLine,
                      Error error )
{
  return NULL;
}

/*
 * Implementation of the file logging driver
 */
static Error fileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger )
{
  Error error;
  struct timeb now;
  int err;
  
  assert( driver == LoggingDriverFile );

  error = emalloc( (void **) logger, sizeof( sLogger ) );
  if( error != NULL ) {
    *logger = NULL;
    return error;
  }

  (*logger)->driver = driver;
  (*logger)->logFileName = strdup( args );
  /* Get the current date string */
  ftime(&now);
  strftime( (*logger)->date, sizeof((*logger)->date), "%Y-%m-%d",
            localtime( &(now.time) ) );

  (*logger)->applicationName = strdup( appName );
  if( (*logger)->applicationName == NULL )
  {
    error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(),
                    "Failed to allocate memory for application string '%s'", 
                    appName );
    goto FAIL;
  }
  
  err = pthread_mutex_init( &((*logger)->mutex), NULL );
  if( err != 0 )
  {
    error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(),
                    "Failed to initialize mutex" );
    goto FAIL_2;
  }
  
  if( (args == NULL) || (strlen( args ) == 0) )
    (*logger)->data = stderr;
  else
  {
    (*logger)->data = fopen( args, "a" ); 
    if( (*logger)->data == NULL )
    {
      error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                      "Failed to open logging file '%s'", args );
      goto FAIL_3;
    }
  }

  return NULL;
  
FAIL_3:
  err = pthread_mutex_destroy( &((*logger)->mutex) );
  /* Not critical if this fails, but it shouldn't, so if it happens, we want
     to catch it at debug time and investigate */
  assert( err == 0 ); 

FAIL_2:
  free( (char *) ((*logger)->applicationName) );

FAIL:
  free((char*)((*logger)->logFileName));
  free( *logger );
  *logger = NULL;

  return error;
}

static void fileDestroy( Logger logger )
{
  assert( logger != NULL );

  if( logger->data != stderr )
  {
    assert( logger->data != NULL );

    fclose( logger->data );
  }

  if (logger->logFileName) {
    free((char*)(logger->logFileName));
  }

  if (logger->applicationName) {
    free( (char *) (logger->applicationName) );
  }
  
  pthread_mutex_destroy(&(logger->mutex));
  free( logger );

  return;
}

static Error fileLogText( Logger logger, const char *moduleName, 
                          LogLevel logLevel, const char *sourceFile, 
                          int sourceLine, const char *format,
                          va_list args )
{
  Error error = NULL;
  char buffer[ BUFFER_LENGTH ];
  char stdOutBuffer[ BUFFER_LENGTH ];
  char tmpDate[ 15 ];
  char weekDate[ 15 ];
  char fileName[ 512 ];
  /*time_t now;*/
  struct timeb now;
  struct tm *localTime;
  time_t aWeekAgo;
  int err, index, tempInt, stdOutIndex;

  pthread_mutex_lock(&logger->mutex);

  /* Special case for the default logger */
  if( logger->data == NULL )
    logger->data = stderr;

  /*now = time( NULL );*/ /* check error control here */
  ftime(&now);
  localTime = localtime( &(now.time) );

  /* Get the current date string */
  strftime( tmpDate, sizeof(tmpDate), "%Y-%m-%d", localTime );
  if ( strcmp( tmpDate, logger->date ) != 0 ) {
    /* Close the current log file */
    if ( (logger->data != NULL) &&
         (logger->data != stderr) ) {
      /* Close the current log file */
      fclose( logger->data );
      /* Rename the current log file */
      strcpy( fileName, logger->logFileName );
      strcat( fileName, "_" );
      strcat( fileName, logger->date );
      err = rename( logger->logFileName, fileName );
      if ( err != 0 ) {
        Error error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                              "Failed to rename logging file '%s' to '%s'",
                              logger->logFileName, fileName );
        ErrReport( error);
        ErrDispose(error, TRUE); error = NULL;
      }
      /* Open a new log file */
      logger->data = fopen( logger->logFileName, "a" ); 
      if ( logger->data == NULL ) {
        Error error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                              "Failed to open logging file '%s'", logger->logFileName );
        ErrReport( error);
        ErrDispose(error, TRUE); error = NULL;
        /* Use the error output */
        logger->data = stderr;
      }
      /* Remove a log file that is a week old */
      aWeekAgo = now.time - (7 * 24 * 60 * 60);
      localTime = localtime( &aWeekAgo );
      strftime( weekDate, sizeof(tmpDate), "%Y-%m-%d", localTime );
      strcpy( fileName, logger->logFileName );
      strcat( fileName, "_" );
      strcat( fileName, weekDate );
      err = remove( fileName );
      if ( err != 0 ) {
        Error error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                              "Failed to remove logging file '%s'", fileName );
        ErrReport( error);
        ErrDispose(error, TRUE); error = NULL;
      }
    }
    /* Save the new date */
    strcpy( logger->date, tmpDate );
  }

  index = strftime( buffer, sizeof( buffer ), "%c",
                    localtime( /*&now*/ &(now.time) ) );
  if (index <= 0) {
    error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, 0, 
                   "strftime failed.");
    goto ERR_RETURN;
  }
  
  if (logger->data != stderr) {
    stdOutIndex = strftime( stdOutBuffer, sizeof( stdOutBuffer), "%c ",
                            localTime );
    if (index <= 0) {
      error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, 0, 
                     "strftime failed.");
      goto ERR_RETURN;
    }
  }

  tempInt = snprintf( &(buffer[ index ]), sizeof( buffer ) - index, 
                      ".%03ld\t%s\t%s\t%s\t[%ld]\t%s:%d\t",
                      now.millitm,
                      logger->applicationName, moduleName, 
                      LogLevelName[ logLevel ],
                      pthread_self(),
                      sourceFile, sourceLine );
  if (tempInt >= 0) {
    
    index += tempInt;
    
    tempInt = vsnprintf(  &(buffer[ index ]), sizeof( buffer ) - index, 
                          format, args );
    buffer[BUFFER_LENGTH-1] = '\0';
  }

/*   if (logger->data != stderr) */
/*     vsnprintf( &(stdOutBuffer[ stdOutIndex ]), */
/*                sizeof( stdOutBuffer ) - stdOutIndex,  */
/*                format, args ); */

  tempInt = fputs( buffer, (FILE *) logger->data );
  if (tempInt < 0) {
    error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, 0, 
                   "fputs failed.");
    goto ERR_RETURN;
  }

  fputc( '\n', (FILE *) logger->data );

  fflush( logger->data );

/*   if (logger->data != stderr) */
/*     fprintf( stderr, "%s\n", stdOutBuffer ); */

 ERR_RETURN:
  pthread_mutex_unlock(&logger->mutex);

  return error;
}

LogLevel log_GlobalLogLevelSet(LogLevel level)
{
  return (_voxiUtilGlobalLogLevel = level);
}

LogLevel log_GlobalLogLevelGet()
{
  return _voxiUtilGlobalLogLevel;
}
