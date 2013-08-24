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

#ifdef WIN32
/* #include <crtdbg.h> */ /* include this for memory debugging */
#endif

/* LIB_UTIL_LOGGING_INTERNAL is required to prevent DLL symbol strudel on WIN32.
   It modifies the LOGGING_EXTERN macro defined in libCCompat.h, and used in
   logging.h
   */
#define LIB_UTIL_LOGGING_INTERNAL

#include <voxi/util/err.h>
#include <voxi/util/logging.h>
#include <voxi/util/mem.h>
#include <voxi/util/path.h>
#include <voxi/util/libcCompat.h>
#include <voxi/util/hash.h>

LOG_MODULE_DECL( "VoxiLogging", LOGLEVEL_NONE );

/*
 * Constants
 */

#define BUFFER_LENGTH 65536
#define MAX_FILENAME_LENGTH 4096
#define FILELOG_DEFAULT_EXTENSION "log"

/*
 * Type definitions
 */
typedef Error (*LoggingCreateFuncPtr)( LoggingDriver driver,
                                const char *applicationName, 
                                const char *parameters, Logger *logger );
typedef void (*LoggingDestroyFuncPtr)( Logger logger );
typedef Error (*LoggingLogTextFuncPtr)( Logger logger, const char *destination,
                                        const char *moduleName, LogLevel logLevel,
                                        const char *sourceFile, int sourceLine,
                                        const char *format, va_list args );

typedef struct sLoggingDriver
{
  LoggingCreateFuncPtr create;
  LoggingDestroyFuncPtr destroy;
  LoggingLogTextFuncPtr logText;
} sLoggingDriver;

#define STRUCT_LOGGER_COMMON_MEMBERS \
  LoggingDriver driver; \
  char *applicationName; \
  pthread_mutex_t mutex; \
  char *logFileFullName; \
  char date[15]; \
  LogFormat logFormat; \
  HashTable logFiles;

typedef struct sLogger
{
  STRUCT_LOGGER_COMMON_MEMBERS
  void *data;
} sLogger;

typedef struct sFileLogger
{
  STRUCT_LOGGER_COMMON_MEMBERS
  void *data;
} sFileLogger;

typedef struct sDualFileLogger {
  STRUCT_LOGGER_COMMON_MEMBERS
  void *data;
} sDualFileLogger;

typedef struct sFileLogger *FileLogger;
typedef struct sDualFileLogger *DualFileLogger;

typedef struct LogFileEntry {
  char *fullName;
  char *fileName;
  char *fileExtension;
  FILE *commonFd;
  FILE *errorFd;
} sLogFileEntry, *LogFileEntry;

/*
 * static function prototypes 
 */

//
// LogFileEntry creation and destruction.
//
static Error logFileEntryCreate(const char *name, LogFileEntry *e, int dualFile);

static void logFileEntryDestroy(LogFileEntry e);

//
// File driver methods
//
static Error fileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger );

static void fileDestroy( Logger logger );

static Error fileLogText( Logger logger, const char *destination,
                          const char *moduleName, 
                          LogLevel logLevel, const char *sourceFile, 
                          int sourceLine, const char *format,
                          va_list args );

//
// DualFile driver methods
//
static Error dualFileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger );

static void dualFileDestroy( Logger logger );

static Error dualFileLogText( Logger logger, const char *destination,
                              const char *moduleName, 
                              LogLevel logLevel, const char *sourceFile, 
                              int sourceLine, const char *format,
                              va_list args );

//
// Log file hash table hooks
//
static int logFileHashTable_hash( LogFileEntry e );
static int logFileHashTable_comp( LogFileEntry e1, LogFileEntry e2);
static void logFileHashTable_dest( LogFileEntry e );

//
// Log file haash table utilities
//
static void logFileHashTable_add(HashTable ht,
                                 const char* fullName,
                                 const char* fileName,
                                 const char* fileExtension,
                                 FILE *commonFd,
                                 FILE *errorFd,
                                 void *funcPtr);

static LogFileEntry logFileHashTable_find(HashTable ht, const char* fullName);

//
// Utility methods
//
void fileLogRotate(FILE **f, 
                   char *logFileName, char *logFileSpecifier, char *logFileExtension,
                   char *logDate);

Error fileLogWrite(FILE *f, char *buffer);

/*
 * Global variables
 */

const char* LogLevelName[NUMBER_OF_LOGLEVELS] = {
  "None", "Critical", "Error", "Warning", "Info", "Debug", "Trace"
};

static sLoggingDriver fileLoggingDriver = { fileCreate, fileDestroy, fileLogText };
static sLoggingDriver dualFileLoggingDriver = { dualFileCreate, dualFileDestroy, dualFileLogText };

static sFileLogger sDefaultLogger = {
  &fileLoggingDriver,           
  "unknown",                    /* applicationName */
  PTHREAD_MUTEX_INITIALIZER,    /* mutex */
  NULL,                         /* logFileName */
  "2004-12-24",                 /* date */
  LOGFORMAT_STANDARD,           /* logFormat */
  NULL,                         /* logFiles */
  NULL                          /* data */
};

static Logger DefaultLogger = (Logger)&sDefaultLogger;

LoggingDriver LoggingDriverFile = &fileLoggingDriver;
LoggingDriver LoggingDriverDualFile = &dualFileLoggingDriver;

LogLevel _voxiUtilGlobalLogLevel = LOGLEVEL_NONE;

/*
 *  Code
 */


static int logFileHashTable_hash( LogFileEntry e )
{
  return HashString( e->fullName );
}

static int logFileHashTable_comp( LogFileEntry e1, LogFileEntry e2) 
{
  return strcmp(e1->fullName, e2->fullName);
}

static void logFileHashTable_dest( LogFileEntry e ) 
{
  logFileEntryDestroy(e);
}

static LogFileEntry logFileHashTable_find(HashTable ht, const char* fullName)
{
  sLogFileEntry template;
  template.fullName = (char*)fullName;
  return (LogFileEntry)HashFind( ht, &template );
}


Error log_create( const char *applicationName, LoggingDriver driver, 
                      const char *driverArguments, Boolean setAsDefault,
                      Logger *logger )
{
  Error error;

  if (driver == NULL ) {
    return ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, NULL, "log_create: Cannot use NULL driver.");
  }

  error = driver->create( driver, applicationName, driverArguments, logger );

  if (error == NULL) {
    (*logger)->logFormat = LOGFORMAT_STANDARD;
    if (setAsDefault) {
      DefaultLogger = *logger;
    }
  }

  return error;
}

void log_destroy( Logger logger )
{
  logger->driver->destroy( logger );
}

void log_setFormat( Logger logger, LogFormat logFormat )
{
  logger->logFormat = logFormat;
}

Error log_logText( Logger logger, const char *logDestination,
                   const char *moduleName, LogLevel logLevel, 
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

  error = logger->driver->logText( logger, logDestination, moduleName, logLevel, filename, 
                                   sourceLine, format, args );

  va_end( args );

  return error;
}

Error log_logError( Logger logger, const char *logDestination,
                    const char *moduleName, LogLevel logLevel, 
                    const char *sourceFile, int sourceLine, ConstError error )
{
  char *errorMessage;
  Error internalError;

  internalError = ErrToHumanReadableString( error, &errorMessage );
  if( internalError != NULL )
    return internalError;

  log_logText( logger, logDestination, moduleName, logLevel, sourceFile, sourceLine, "%s", 
                errorMessage );

  free( errorMessage );

  return NULL;
}

Error log_noLogText( Logger logger, const char *logDestination,
                     const char *moduleName, LogLevel logLevel,
                     const char *sourceFile, int sourceLine, const char *format, ... )
{
  return NULL;
}

Error log_noLogError( Logger logger, const char *logDestination,
                      const char *moduleName, LogLevel logLevel, 
                      const char *sourceFile, int sourceLine, ConstError error )
{
  return NULL;
}


/*
 * Implementation of methods common to all loggers.
 */
static Error loggerCreateCommon( Logger logger, LoggingDriver driver,
                                 const char *appName )
{
  Error error = NULL;
  int mutexInited = FALSE;
  struct timeb now;
  
  logger->driver = driver;
  
  // Get current date.
  ftime(&now);
  strftime( logger->date, sizeof(logger->date), "%Y-%m-%d", localtime( &(now.time) ) );

  // Initialize hash table.
  if (error == NULL) {
    logger->logFiles = HashCreateTable(16, logFileHashTable_hash,
                                       logFileHashTable_comp, logFileHashTable_dest);
    if (logger->logFiles == NULL) {
      error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, NULL, "Failed to create log file hash table.");
    }
  }

  // Application name.
  if (error == NULL) {
    logger->applicationName = _strdup( appName );
    if( logger->applicationName == NULL ) {
      error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(),
                      "Failed to allocate memory for application string '%s'", appName );
    }
  }

  // File name - should be filled in later.
  logger->logFileFullName = NULL;

  // Mutex.
  if (error == NULL) {
    int err = pthread_mutex_init( &(logger->mutex), NULL );
    if( err != 0 ) {
      error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), "Failed to initialize mutex" );
    }
	else {
      mutexInited = TRUE;
	}
  }
  

  // Cleanup.
  if (error) {
    if (logger->applicationName) {
      free(logger->applicationName);
    }
    if (mutexInited) {
      pthread_mutex_destroy( &(logger->mutex) );
    }
  }
  
  return error;
}

static void loggerDestroyCommon( Logger logger )
{

  if (logger->logFileFullName) {
    free((char*)(logger->logFileFullName));
  }
  if (logger->applicationName) {
    free(logger->applicationName);
  }
  pthread_mutex_destroy(&(logger->mutex));
  if (logger->logFiles) {
    HashDestroyTable(logger->logFiles);
  }
}


/*
 * Implementation of the file logging driver
 */
static Error fileCreate( LoggingDriver driver, const char *appName, 
                         const char *args, Logger *logger )
{
  Error error;
  FileLogger fileLogger;
  
  assert( driver == LoggingDriverFile );

  error = emalloc( (void **) &fileLogger, sizeof( sFileLogger ) );
  if( error != NULL ) {
    fileLogger = NULL;
    *logger = NULL;
  }
  else {
    error = loggerCreateCommon((Logger)fileLogger, driver, appName);
    if (error == NULL) {
      LogFileEntry e;
      *logger = (Logger)fileLogger;
      error = logFileEntryCreate(args, &e, FALSE);
      if (error == NULL) {
        fileLogger->logFileFullName = _strdup(e->fullName);
        if (!HashAdd(fileLogger->logFiles, e)) {
          logFileEntryDestroy(e);
          e = NULL;
        }
      }
    }
  }

  if (error) {
    if (fileLogger) {
      loggerDestroyCommon((Logger)fileLogger);
      free(fileLogger);
      *logger = NULL;
      fileLogger = NULL;
    }
  }

  return error;
}

static void fileDestroy( Logger logger )
{
  FileLogger fileLogger = (FileLogger)logger;
  
  assert( logger != NULL );
  assert( fileLogger->driver == LoggingDriverFile );

  loggerDestroyCommon((Logger)fileLogger);
  
  free( fileLogger );

  return;
}

Error fileLogWrite(FILE *f, char *buffer)
{
  int tempInt = fputs( buffer, f );
  if (tempInt < 0) {
    return ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, NULL, "fputs failed.");
  }
  fputc('\n', f);
  fflush(f);
  return NULL;
}

//
// Build a complete formatted line for logging to a file.
//
// lockedMutex, if supplied, must be a locked mutex and will be
// temporarily unlocked when the printf formatting of the user format
// and arguments is done. It will be relocked after the
// operation. During this operation the format and buffer parameters
// will be used and modified respectively in a non-locked situation.
//
static Error fileLogBuildLine( pthread_mutex_t *lockedMutex,
                               char *buffer, size_t buffer_length,
                               LogFormat logFormat, 
                               const char *applicationName, const char *moduleName, 
                               LogLevel logLevel, const char *sourceFile, 
                               int sourceLine, const char *format,
                               va_list args )
{
  struct timeb now;
  struct tm *localTime;
  int index, tempInt;
  pthread_t self;
  unsigned long threadID;

  /*now = time( NULL );*/ /* check error control here */
  ftime(&now);
  localTime = localtime( &(now.time) );

  index = strftime( buffer, buffer_length, "%c",
                    localtime( /*&now*/ &(now.time) ) );
  if (index <= 0) {
    return ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, NULL, "strftime failed.");
  }
  
  /*
   * In Pthreads win32 versions 2 or greater. pthread_t is a struct, and not
   * an int. 
   *
   * If pthread_t is a struct larger than unsigned long, then:
   *   If we are using pthreads win32
   *     use pthread_getw32threadhandle_np to get a printable thread handle
   *   Else
   *     don't print a thread ID
   * Else
   *   Print the pthread_t value as an unsigned long
   *
   */
  self = pthread_self();
  if( sizeof( self ) > sizeof( unsigned long ) ) {
#if defined(PTW32_VERSION) && (PTW32_LEVEL >= PTW32_LEVEL_MAX)
    threadID = (unsigned long) pthread_getw32threadhandle_np( self );
#else /* don't print any thread identifier */
    threadID = 0;
#endif
  }    
  else {
    threadID = *((unsigned long *) &self);
  }

  if (logFormat == LOGFORMAT_LEGACY) {
    // LOGFORMAT_LEGACY:
    // Timestamp Application Module Level [thread] File:Line Message
    tempInt = snprintf( &(buffer[ index ]), buffer_length - index, 
                        ".%03ld\t%s\t%s\t%s\t[%lu]\t%s:%d\t",
                        now.millitm,
                        applicationName, moduleName, 
                        LogLevelName[ logLevel ],
                        threadID,
                        sourceFile, sourceLine );
  }
  else {
    // LOGFORMAT_STANDARD:
    // Timestamp Level [process;thread] Application/Module File:Line Message
    tempInt = snprintf( &(buffer[ index ]), buffer_length - index, 
                        ".%03ld\t%s\t[%lu]\t%s/%s\t%s:%d\t",
                        now.millitm,
                        LogLevelName[ logLevel ],
                        threadID,
                        applicationName, moduleName, 
                        sourceFile, sourceLine );
  }

  if (tempInt >= 0) {
    
    index += tempInt;

    if (lockedMutex != NULL) {
      pthread_mutex_unlock(lockedMutex);
    }

    // This call has a higher than zero possibility of resulting in a
    // runtime error due to faults in the user-supplied format and
    // args parameters. To reduce the risk of deadlocks in such a
    // situation we temporarily unlock the locked mutex.
    tempInt = vsnprintf(  &(buffer[ index ]), buffer_length - index, 
                          format, args );
    
    if (lockedMutex != NULL) {
      pthread_mutex_lock(lockedMutex);
    }

    buffer[buffer_length-1] = '\0';
  }
  return NULL;
}
  
static Error fileLogText( Logger logger, const char *logDestination,
                          const char *moduleName, LogLevel logLevel,
                          const char *sourceFile, int sourceLine, const char *format, va_list args )
{
  Error error = NULL;
  FileLogger fLogger = (FileLogger)logger;
  char buffer[ BUFFER_LENGTH ];
  char tmpDate[ 15 ];
  struct timeb now;
  struct tm *localTime;
  int shouldRotate = FALSE;
  LogFileEntry e = NULL;
  const char *destToUse = LOG_DESTINATION_CONSOLE;
  char fullDestToUse[MAX_FILENAME_LENGTH];

  pthread_mutex_lock(&fLogger->mutex);

  if ((logDestination != NULL) && (strlen(logDestination) > 0)) {
    destToUse = logDestination;
    if ((_stricmp(destToUse, LOG_DESTINATION_CONSOLE) != 0) && (strrchr(destToUse, '.') == NULL)) {
      snprintf(fullDestToUse, sizeof(fullDestToUse), "%s.%s", destToUse, FILELOG_DEFAULT_EXTENSION);
      destToUse = fullDestToUse;
    }
  }
  else {
    destToUse = fLogger->logFileFullName;
  }

  if (fLogger->logFiles != NULL) {
    e = logFileHashTable_find(fLogger->logFiles, destToUse);
    if (e == NULL) {
      error = logFileEntryCreate(destToUse, &e, FALSE);
      if (error == NULL) {
        if (!HashAdd(fLogger->logFiles, e)) {
          logFileEntryDestroy(e);
          e = NULL;
          error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, NULL,
                         "Failed to create log file entry for '%s'.", destToUse);
        }
      }
    }
  }

  if (e != NULL && error == NULL) {
    ftime(&now);
    localTime = localtime( &(now.time) );
    
    /* Get the current date string */
    strftime( tmpDate, sizeof(tmpDate), "%Y-%m-%d", localTime );
    if ( strcmp( tmpDate, fLogger->date ) != 0 ) {
      shouldRotate = TRUE;
    }
    
    if (shouldRotate) {
      
      /* Rotate the log */
      if ((e->commonFd != NULL) && (e->commonFd != stderr) && (e->commonFd != stdout)) {
        fileLogRotate(&(e->commonFd), e->fileName, "", e->fileExtension, tmpDate);
      }
      
      /* Save the new date */
      strncpy(fLogger->date, tmpDate, sizeof(fLogger->date));
    }

    error = fileLogBuildLine(&fLogger->mutex, buffer, BUFFER_LENGTH,
                             fLogger->logFormat,
                             fLogger->applicationName, moduleName,
                             logLevel, sourceFile,
                             sourceLine, format,
                             args);
  }

  if (error == NULL) {
    FILE *fd = e ? e->commonFd : stderr;
    if (fd) {
      error = fileLogWrite(fd, buffer);
    }
  }

  pthread_mutex_unlock(&fLogger->mutex);

  return error;
}

void logFileEntryDestroy(LogFileEntry e) 
{
  if (e->fullName) {
    free(e->fullName);
    e->fullName = NULL;
  }
  if (e->fileName) {
    free(e->fileName);
    e->fileName = NULL;
  }
  if (e->fileExtension) {
    free(e->fileExtension);
    e->fileExtension = NULL;
  }
  if ((e->commonFd != stdout) && (e->commonFd != stderr) && (e->commonFd != NULL)) {
    fclose(e->commonFd);
  }
  if ((e->errorFd != stdout) && (e->errorFd != stderr) && (e->errorFd != NULL)) {
    fclose(e->errorFd);
  }
  free(e);
}

Error logFileEntryCreate(const char *name, LogFileEntry *e, int dualFile)
{
  Error error = NULL;
  const char *theName = name ? name : LOG_DESTINATION_CONSOLE;
  error = emalloc((void **)e, sizeof (sLogFileEntry));
  if (error == NULL) {
    if (_stricmp(theName, LOG_DESTINATION_CONSOLE) == 0) {
      (*e)->fullName = _strdup(LOG_DESTINATION_CONSOLE);
      (*e)->fileName = NULL;
      (*e)->fileExtension = NULL;
      if (dualFile) {
        (*e)->commonFd = stdout;
        (*e)->errorFd = stderr;
      }
      else {
        (*e)->commonFd = stderr;
        (*e)->errorFd = NULL;
      }
    }
    else {
      // Set the name.
      char *extension;
      (*e)->fileName = _strdup(theName);
      if ((extension = strrchr((*e)->fileName, '.')) != NULL) {
        // Argument of type "name.ext". Separate name from extension:
        (*e)->fileExtension = _strdup(&(extension[1]));
        extension[0] = '\0';
      }
      else {
        // Argument of type "name". Append default extension.
        (*e)->fileExtension = _strdup(FILELOG_DEFAULT_EXTENSION);
      }
      (*e)->fullName = (char*)malloc(MAX_FILENAME_LENGTH);
      if ((*e)->fullName == NULL) {
        error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(),
                       "Failed to allocate memory for file name string '%s'", 
                       (*e)->fileName);
      }
      else {
        snprintf((*e)->fullName, MAX_FILENAME_LENGTH, "%s.%s", (*e)->fileName, (*e)->fileExtension);
      }
      
      // Open the file(s)
      if (error == NULL) {
        char fname[MAX_FILENAME_LENGTH];
      
        // Open 'filename_Common.log'
        snprintf(fname, MAX_FILENAME_LENGTH, "%s_Common.%s",
                 (*e)->fileName, (*e)->fileExtension ? (*e)->fileExtension : FILELOG_DEFAULT_EXTENSION );
        (*e)->commonFd = fopen( fname, "a" ); 
        if( (*e)->commonFd == NULL ) {
          error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                          "Failed to open logging file '%s'", fname );
        }
        else {
          // Open 'filename_Error.log'
          snprintf(fname, MAX_FILENAME_LENGTH, "%s_Error.%s",
                   (*e)->fileName, (*e)->fileExtension ? (*e)->fileExtension : FILELOG_DEFAULT_EXTENSION );
          (*e)->errorFd = fopen( fname, "a" ); 
          if( (*e)->errorFd == NULL ) {
            error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                            "Failed to open logging file '%s'", fname );
          }
        }
      }
    }
  }
  if (error) {
    if (*e != NULL) {
      if ((*e)->fullName) free((*e)->fullName);
      if ((*e)->fileName) free((*e)->fileName);
      if ((*e)->fileExtension) free((*e)->fileExtension);
      free(*e);
	  *e = NULL;
    }
  }
  return error;
}

/*
 * Implementation of the dual file logging driver
 */
static Error dualFileCreate( LoggingDriver driver, const char *appName, 
                             const char *args, Logger *logger )
{
  Error error;
  DualFileLogger dFileLogger;
  
  assert( driver == LoggingDriverDualFile );

  error = emalloc( (void **) &dFileLogger, sizeof( sDualFileLogger ) );
  if( error != NULL ) {
    dFileLogger = NULL;
    *logger = NULL;
  }
  else {
    error = loggerCreateCommon((Logger)dFileLogger, driver, appName);
    if (error == NULL) {
      LogFileEntry e;
      *logger = (Logger)dFileLogger;
      error = logFileEntryCreate(args, &e, TRUE);
      if (error == NULL) {
        dFileLogger->logFileFullName = _strdup(e->fullName);
        if (!HashAdd(dFileLogger->logFiles, e)) {
          logFileEntryDestroy(e);
          e = NULL;
        }
      }
    }
  }

  if (error) {
    if (dFileLogger) {
      loggerDestroyCommon((Logger)dFileLogger);
      free( dFileLogger );
      *logger = NULL;
      dFileLogger = NULL;
    }
  }

  return error;
}

static void dualFileDestroy( Logger logger )
{
  DualFileLogger dFileLogger = (DualFileLogger)logger;
  
  assert( logger != NULL );
  assert( dFileLogger->driver == LoggingDriverDualFile );

  loggerDestroyCommon((Logger)dFileLogger);
  
  free( dFileLogger );

  return;
}

void fileLogRotate(FILE **f, 
                   char *logFileName, char *logFileSpecifier, char *logFileExtension,
                   char *logDate)
{
  char fileName[ MAX_FILENAME_LENGTH ];
  char currentLogFileName[ MAX_FILENAME_LENGTH ];
  struct timeb now;
  struct tm *localTime;
  char weekDate[ 15 ];
  time_t aWeekAgo;
  int err;
  
  /* Close the current log file */
  fclose(*f);
  
  /* Rename the current log file */
  snprintf(currentLogFileName, sizeof(currentLogFileName), "%s%s.%s",
           logFileName,
           logFileSpecifier,
           logFileExtension);
  snprintf(fileName, sizeof(fileName), "%s%s_%s.%s",
           logFileName,
           logFileSpecifier,
           logDate,
           logFileExtension);
  err = rename(currentLogFileName, fileName);
  if ( err != 0 ) {
    Error error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                          "Failed to rename logging file '%s' to '%s'",
                          currentLogFileName, fileName );
    ErrReport( error);
    ErrDispose(error, TRUE); error = NULL;
  }
  
  /* Open a new log file */
  *f = fopen(currentLogFileName, "a" ); 
  if (*f == NULL) {
    Error error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                          "Failed to open logging file '%s'", currentLogFileName );
    ErrReport( error);
    ErrDispose(error, TRUE); error = NULL;
    /* Use the error output */
    *f = stderr;
  }
  
  /* Remove a log file that is a week old */
  ftime(&now);
  aWeekAgo = now.time - (7 * 24 * 60 * 60);
  localTime = localtime( &aWeekAgo );
  strftime( weekDate, sizeof(weekDate), "%Y-%m-%d", localTime );
  snprintf(fileName, sizeof(fileName), "%s%s_%s.%s",
           logFileName,
           logFileSpecifier,
           weekDate,
           logFileExtension);
  err = remove( fileName );
  if ( err != 0 ) {
    Error error = ErrNew( ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, ErrErrno(), 
                          "Failed to remove logging file '%s'", fileName );
    ErrReport( error);
    ErrDispose(error, TRUE); error = NULL;
  }
}

static Error dualFileLogText( Logger logger, const char *logDestination,
                              const char *moduleName, LogLevel logLevel, const char *sourceFile, 
                              int sourceLine, const char *format, va_list args )
{
  Error error = NULL;
  DualFileLogger dfLogger = (DualFileLogger)logger;
  char buffer[ BUFFER_LENGTH ];
  char tmpDate[ 15 ];
  struct timeb now;
  struct tm *localTime;
  int shouldRotate = FALSE;
  LogFileEntry e = NULL;
  const char *destToUse = LOG_DESTINATION_CONSOLE;
  char fullDestToUse[MAX_FILENAME_LENGTH];

  pthread_mutex_lock(&dfLogger->mutex);

  if ((logDestination != NULL) && (strlen(logDestination) > 0)) {
    destToUse = logDestination;
    if ((_stricmp(destToUse, LOG_DESTINATION_CONSOLE) != 0) && (strrchr(destToUse, '.') == NULL)) {
      snprintf(fullDestToUse, sizeof(fullDestToUse), "%s.%s", destToUse, FILELOG_DEFAULT_EXTENSION);
      destToUse = fullDestToUse;
    }
  }
  else {
    destToUse = dfLogger->logFileFullName;
  }

  if (dfLogger->logFiles != NULL) {
    e = logFileHashTable_find(dfLogger->logFiles, destToUse);
    if (e == NULL) {
      error = logFileEntryCreate(destToUse, &e, TRUE);
      if (error == NULL) {
        if (!HashAdd(dfLogger->logFiles, e)) {
          logFileEntryDestroy(e);
          e = NULL;
          error = ErrNew(ERR_LOGGING, ERR_LOGGING_UNSPECIFIED, NULL,
                         "Failed to create log file entry for '%s'.", destToUse);
        }
      }
    }
  }

  if (e != NULL && error == NULL) {
    ftime(&now);
    localTime = localtime( &(now.time) );
    
    /* Get the current date string */
    strftime( tmpDate, sizeof(tmpDate), "%Y-%m-%d", localTime );
    if ( strcmp( tmpDate, dfLogger->date ) != 0 ) {
      shouldRotate = TRUE;
    }
    
    if (shouldRotate) {
      
      /* Rotate Common log */
      if ((e->commonFd != NULL) && (e->commonFd != stderr) && (e->commonFd != stdout)) {
        fileLogRotate(&(e->commonFd), e->fileName, "_Common", e->fileExtension, tmpDate);
      }
      
      /* Rotate Error log */
      if ((e->errorFd != NULL) && (e->errorFd != stderr) && (e->errorFd != stdout)) {
        fileLogRotate(&(e->errorFd), e->fileName, "_Error", e->fileExtension, tmpDate);
      }
      
      /* Save the new date */
      strncpy(dfLogger->date, tmpDate, sizeof(dfLogger->date));
    }
    
    error = fileLogBuildLine(&dfLogger->mutex, buffer, BUFFER_LENGTH,
                             dfLogger->logFormat,
                             dfLogger->applicationName, moduleName,
                             logLevel, sourceFile,
                             sourceLine, format,
                             args);
  }

  /* Write to Error log. */
  if ((e != NULL) && (error == NULL)) {
    if (e->errorFd && (logLevel > LOGLEVEL_NONE) && (logLevel <= LOGLEVEL_WARNING)) {
      error = fileLogWrite((FILE *)e->errorFd, buffer);
    }
  }
  
  /* Write to Common log. */
  if (error == NULL) {
    FILE *fd = e ? e->commonFd : stderr;
    if (fd) {
      error = fileLogWrite(fd, buffer);
    }
  }

  pthread_mutex_unlock(&dfLogger->mutex);

  return error;
}

LogLevel log_GlobalLogLevelSet(LogLevel level)
{
  LogLevel oldLevel;

  oldLevel = _voxiUtilGlobalLogLevel;
  if( level < oldLevel )
    LOG_INFO( LOG_INFO_ARG, "Global logging level changed from %d to %d", 
              _voxiUtilGlobalLogLevel, level );

  _voxiUtilGlobalLogLevel = level;

  if( level >= oldLevel )
    LOG_INFO( LOG_INFO_ARG, "Global logging level changed from %d to %d", 
              oldLevel, level );

  return oldLevel;
}

LogLevel log_GlobalLogLevelGet()
{
  return _voxiUtilGlobalLogLevel;
}
