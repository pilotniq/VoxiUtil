/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  General purpose error handling.

  based on:
    The Hemuls error functions (Which are, of course, never called)

  adapted to support POSIX threads in May 1998 by erl
  integrated into the Voxi cvs in January 2000 by erl
  
  $Id$
  
  */
#define ERR_C

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
/* I think unistd.h is required to get _POSIX_THREADS definition */
#if 0
#ifdef WIN32
#include <windows.h>
#include <voxi/util/win32_glue.h>
#else
#include <unistd.h>
#endif
#endif /* 0 */
#if _REENTRANT && defined(_POSIX_THREADS)
#include <pthread.h>
#endif

#include <voxi/alwaysInclude.h>
#include <voxi/types.h>
#include <voxi/util/vector.h>
#include <voxi/util/strbuf.h>
#include <voxi/util/err.h>

/*
 * These don't work with the thread pool since
 * they set error stack on specific threads and
 * the thread pool reuse the threads. (The pthred_key stuff)
 * Disabled push and pop until this is fixed
 */
#define DISABLE_PUSH_POP

CVSID("$Id$");

__inline static Vector getErrorStack();


#if _REENTRANT && defined(_POSIX_THREADS)
static pthread_key_t ErrorStackKey, InErrorKey, IndentCountKey;
#else
static Vector ErrorStack;
static Boolean InError = FALSE;
static int indentCount = 0;
#endif
static Boolean dispTraceState = FALSE;
static Boolean Initialized = FALSE;

Boolean ErrInit(void)
{
#if _REENTRANT && defined(_POSIX_THREADS)
  int err;
#endif

  DEBUG("enter\n");

  if(Initialized)
    return(TRUE);

#if _REENTRANT && defined(_POSIX_THREADS)
  err = pthread_key_create( &ErrorStackKey, NULL /*  vectorRefcountDecrease */ );
  assert( err == 0 );
  
  err = pthread_key_create( &InErrorKey, NULL );
  assert( err == 0 );

  err = pthread_key_create( &IndentCountKey, NULL );
  assert( err == 0 );
#else
  ErrorStack = vectorCreate( "Error Function Trace Stack", sizeof( char * ), 
                             8, 8, NULL );
#endif
  Initialized = TRUE;

  DEBUG("leave\n");

  return(TRUE);
}

/*
  resulting length of printf'ing must be less than 256 characters
  */
void ErrPushFunc(char *funcname, ...)
{

#ifndef DISABLE_PUSH_POP /* Doesn't work with thread pool, disabled until bug is fixed */
  
  va_list args;
  char buf[256];
  char *string;
#if _REENTRANT && defined(_POSIX_THREADS)
  Vector ErrorStack;
  Boolean inError;
#endif
  /*  Boolean DATraceStatus; */

  if(!Initialized) /* The error stack must have been initialized */
    return;

#if _REENTRANT && defined(_POSIX_THREADS)
  inError = (Boolean) pthread_getspecific( InErrorKey );
  if( inError )
    return;

  pthread_setspecific( InErrorKey, (void *) TRUE );

  ErrorStack = getErrorStack();
#else
  if(InError)
    return;
  else
    InError = TRUE;
#endif

  /* Don't display trace of DA-calls from error handlers */
  /* DATraceStatus = DA_Trace(FALSE); */

  va_start(args, funcname);

  vsprintf(buf, funcname, args); /* print parameters int buf */

  /* allocate a string the exact length of the sprintf'd string */
  
  string = strdup( buf );
  
  if( string == NULL )
    ERR ERR_ABORT, "Ran out of memory in ErrPushFunc, strdup(%d)", 
      (strlen(buf)+1) ENDERR;

  vectorAppend( ErrorStack, &string );

  if(dispTraceState)
  {
    int i;
#if _REENTRANT && defined(_POSIX_THREADS)
    int indentCount;

    fprintf( stderr, "Thread %p:", (void *) pthread_self() );

    indentCount = (int) pthread_getspecific( IndentCountKey );

    pthread_setspecific( IndentCountKey, (void *) (indentCount + 1) );
#endif
    fflush(stdout);
    fflush(stderr);

    for(i=0; i<indentCount; i++) fprintf(stderr, "  ");
    fprintf(stderr, "Entering %s\n", string);
    indentCount++;

    fflush(stderr);
  }
#if _REENTRANT && defined(_POSIX_THREADS)
  pthread_setspecific( InErrorKey, (void *) FALSE );
#else
  InError=FALSE;
#endif
  /* DA_Trace(DATraceStatus); */

#endif /* Disable push pop */
}

void ErrPopFunc(void)
{
#ifndef DISABLE_PUSH_POP /* Doesn't work with thread pool, disabled until bug is fixed */
  
  char *funcname;
  int stackCount;
#ifdef _POSIX_THREADS
  Boolean inError;
  Vector ErrorStack;
#endif
 /* Boolean DATraceStatus; */

  if(!Initialized)
    return;

#if _REENTRANT && defined(_POSIX_THREADS)
  inError = (Boolean) pthread_getspecific( InErrorKey );
  if( inError )
    return;

  pthread_setspecific( InErrorKey, (void *) TRUE );

  ErrorStack = getErrorStack();
#else
  if(InError)
    return;
  else
    InError = TRUE;
#endif
  /* DATraceStatus = DA_Trace(FALSE); */
  
  stackCount = vectorGetElementCount( ErrorStack );
  
  if( stackCount == 0 )
    ERR ERR_WARN, "ErrFuncPop: Pop failed.\n" ENDERR;
  else
  {
    vectorGetElementAt( ErrorStack, stackCount - 1, &funcname );
    vectorRemoveLastElement( ErrorStack );

    if(dispTraceState)
    {
      int i;
#if _REENTRANT 
      int indentCount;

      fprintf( stderr, "Thread %p:", (void *) pthread_self() );

      indentCount = (int) pthread_getspecific( IndentCountKey );

      pthread_setspecific( IndentCountKey, (void *) (indentCount - 1) );
#endif
      fflush(stdout);
      fflush(stderr);

      indentCount--;
      for(i=0; i<indentCount; i++) fprintf(stderr, "  ");
      
      fprintf(stderr, "Leaving %s\n", funcname);
    }
    free(funcname); /* malloc'd in ErrPushFunc */
  }
#if _REENTRANT
  pthread_setspecific( InErrorKey, (void *) FALSE );
#else
  InError = FALSE;
#endif
  /* DA_Trace(DATraceStatus); */

#endif /* Disable push pop */
}

void Err(char *file, unsigned int line, Err_Action action, char *string, ...)
{
  va_list args;
  char *temp;
  int index;

#if _REENTRANT
  Vector ErrorStack = getErrorStack();
#endif

  va_start(args, string);

  fprintf(stderr, "%s in file %s, line %d : ",
   action == ERR_ABORT ? "Fatal Error" : "Warning", file, line);

  vfprintf(stderr, string, args);
  putc('\n', stderr);

  va_end(args);

  if(Initialized)
  {
    fflush(stdout);
    fflush(stderr);

    fputs("Function trace:\n", stderr);


    for( index = vectorGetElementCount( ErrorStack ) - 1;
         index >= 0; vectorGetElementAt( ErrorStack, index, (void **) &temp) )
    {
      fprintf(stderr, "  %s\n", temp);
      index--;
    }
  }
  else
    fputs( "Error occured before Error functions had been initialized!?\n",
           stderr);

  if(action == ERR_ABORT)
    exit(1);
}

Boolean DisplayTrace(Boolean newState)
{
  Boolean retval = dispTraceState;

  if(!Initialized)
    return(retval);

  dispTraceState = newState;

  fflush(stdout);
  fflush(stderr);

#if 0
  switch(dispTraceState = newState) {
    case TRUE:
      if(retval=FALSE) indentCount=0;
      fprintf(stderr, "Tracing turned ON - ");
      break;

    case FALSE:
      fprintf(stderr, "Tracing turned OFF - ");
      break;
    }
  if(DA_Get(ErrorStack, DA_Count(ErrorStack)-1, (void**) &tmp, TRUE))
    fprintf(stderr, "Last entry: %s\n", tmp);
  else
    fprintf(stderr, "No entries pushed\n");

  fflush(stdout);
  fflush(stderr);
#endif
  return(retval);
}

ErrType Err_getType(Error err) {
  assert(err != NULL);
  return err->type;
}

int Err_getNum(Error err) {
  assert(err != NULL);
  return err->number;
}


Error ErrNew(ErrType t, int number, Error reason, const char *description,...)
{
  Error result;
  va_list args;
  static char buf[1024];
  char *string;

  ErrPushFunc("ErrNew");

  va_start(args, description);

  vsprintf(buf, description, args); /* print parameters int buf */
  string = strdup( buf );

  if( string == NULL )
    ERR ERR_ABORT, "Ran out of memory in ErrNew" ENDERR;

  result = malloc(sizeof(sError));
  if(result == NULL)
    ERR ERR_WARN, "malloc failed" ENDERR;
  else
  {
    result->type = t;
    result->number = number;
    result->description = string;
    result->reason = reason;
  }

  ErrPopFunc();
  return result;
}

void ErrDispose(Error err, Boolean recursive)
{
  if (err == NULL)
    return;
  if (recursive && err->reason != NULL)
    ErrDispose(err->reason, TRUE);
/*    free(err->description); */
  free(err);
}

/*
  Converts an error to a string representation. The representation is:
  
  Error( <type> <number> <description> <reason> )
  
  Any spaces within the type, number, description or reason are quoted with
  a percent character followed by the hexadecimal code for the character
*/
Error ErrToString( Error error, StringBuffer strbuf )
{
  Error error2;
  
  error2 = strbuf_sprintfQuoteSubstrings( strbuf, " \n\r'", 
                                          "Error( %d %d '%s' ", 
                                          error->type,
                                          error->number, error->description );
  if( error2 == NULL )
  {
    if( error->reason == NULL )
      error2 = strbuf_sprintf( strbuf, "NULL" );
    else
      error2 = ErrToString( error->reason, strbuf );
  }
  
  if( error2 == NULL )
    error2 = strbuf_sprintf( strbuf, " )" );
  
  return error2;
}

__inline static Vector getErrorStack()
{
#if _REENTRANT
  Vector  ErrorStack = pthread_getspecific( ErrorStackKey );

  if( ErrorStack == NULL )
  {
    ErrorStack = vectorCreate( "Error Function Trace Stack", sizeof( char * ),
                               8, 8, NULL );

    pthread_setspecific( ErrorStackKey, ErrorStack );
  }
 
  return ErrorStack;
#else
  return ErrorStack;
#endif
}

#ifdef MACOS

#include <Errors.h>
#include <MacTCPCommonTypes.h>
#include <erl/mac.h>

void ErrReport(Error err)
{
  static short alertID;
  static Boolean gotAlertID = FALSE;
  DialogPtr dlog;
  short ditlType;
  Handle ditlHandle;
  Rect ditlRect;
  short item;

  if( !gotAlertID )
  {
    Handle res;
    ResType type;
    unsigned char name[ 256 ];

    res = GetNamedResource( 'DLOG', "\pErrReport");
    if(res == NULL)
    {
      ERR ERR_WARN, "Couldn't find ErrReport dialog, GetNamedResource: '%s', #%d",
        ErrOSErrName( ResError() ), ResError() ENDERR;
      return;
    }

    GetResInfo( res, &alertID, &type, (ConstStr255Param) name );
    if( ResError() != 0)
    {
      ERR ERR_WARN, "ErrReport: GetResInfo: '%s', #%d", ErrOSErrName( ResError() ), ResError() 
        ENDERR;
      return;
    }

    gotAlertID = TRUE;

    ReleaseResource(res);
    if( ResError() != 0)
    {
      ERR ERR_WARN, "ErrReport: ReleaseResource: '%s', #%d", ErrOSErrName( ResError() ), ResError() 
        ENDERR;
      return;
    }
  } /* end of if we don't know alert ID */

  macParamText(err->description);

  dlog = GetNewDialog(alertID, NULL, (WindowPtr) -1);
  if(dlog == NULL)
    ERR ERR_WARN, "ErrReport: GetNewDialog failed." ENDERR;


  /* Enable/Disable Why button according to whether we have that info in the Error record */
  /* Disabling doesn't seem to gray the button - how do we do that? */
  GetDItem( dlog, 2, &ditlType, &ditlHandle, &ditlRect );
  if(err->reason == NULL)
    ditlType = ditlType | 0x0080;
  else
    ditlType = ditlType & 0xff7f;
  
  SetDItem( dlog, 2, ditlType, ditlHandle, &ditlRect );

  /* Let user interact with dialog */
  ShowWindow(dlog);
  ModalDialog(NULL, &item);
  if(item == 2) /* Why */
    ErrReport(err->reason);

  DisposDialog(dlog);
}


const char *ErrOSErrName( OSErr err )
{
  char buf[256];

  switch( err )
  {
    case portNotCf: 
      return "AppleTalk: Turned off?";
      break;

    case openErr:
      return "Error opening driver";
      break;

    case resNotFound:
      return "Resource not found";
      break;

    case fnOpnErr:
      return "File not open";
      break;

    case iMemFullErr:
      return "Not enough room in heap zone. (Memory full)";
      break;

    /* MacTCP */
    case ipBadAddr:
      return "MacTCP: Error getting address (not connected to network?)";
      break;

    /* Sound Input */
    case siDeviceBusyErr:
      return "Sound Input: Input device already in use.";
      break;

    default:
      sprintf(buf, "Unknown MacOS error (#%d)", err );
      return strdup(buf);
      break;
  }
}

#else


void ErrReport(Error err)
{
  fprintf( stderr, "Error:\n" );

  while( err != NULL )
  {
    fprintf( stderr, "  %s\n", err->description );
    if( err->reason != NULL )
      fprintf( stderr, "because\n" );
    err = err->reason;
  }
}

#endif

#ifdef WIN32

#include <voxi/util/win32_glue.h>

Error ErrWin32()
{
  Error error;
  LPVOID lpMsgBuf;

  FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
  );

  error = ErrNew( ERR_WIN32, GetLastError(), NULL, "Win32: %s", lpMsgBuf );

  LocalFree( lpMsgBuf );

  return error;
}
#endif
