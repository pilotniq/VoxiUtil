/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  textRPC.c
  
  Implements a Remote Procedure Call (RPC) protocol using text-based 
  messages over TCP.
    On top of this protocol may be layered application bindings to functions
  etcetera. 
*/

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <voxi/alwaysInclude.h>
#include <voxi/util/bag.h>
#include <voxi/util/err.h>
#include <voxi/util/hash.h>
#include <voxi/util/mem.h>
#include <voxi/util/sock.h>
#include <voxi/util/strbuf.h>
#include <voxi/util/threading.h>
#include <voxi/util/threadpool.h>

#include <voxi/util/textRPC.h>

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif /* WIN32 */

CVSID("$Id$");

#define USE_THREADED_CALLS
/*#define DEBUG_TEXTRPC_THREADS  */
#ifdef DEBUG_TEXTRPC_THREADS
static int noExitedThreads = 0;
static int noEnteredThreads = 0;
static sVoxiMutex noCallThreadsMutex;
static Boolean debugThreadsInited = FALSE;
#endif

/*
 *  Constant definitions
 */
#define LATEST_PROTOCOL_VERSION 0
#define LATEST_PROTOCOL_VERSION_STRING "0"
#define OUTSTANDING_FUNC_HASH_SIZE 31

#define TRPC_USE_THREADPOOL
#define TRPC_THREADPOOL_SIZE 5

/*
 *  Macro definitions
 */

/*
 * Type definitions
 */
typedef void * (*thread_start_routine) (void *);

/* 
   "local" types
*/

typedef enum { PROTOCOL_STATE_INITIALIZING,
               PROTOCOL_STATE_NEGOTIATING_PROTOCOL_1, 
               PROTOCOL_STATE_NEGOTIATING_PROTOCOL_2, 
               PROTOCOL_STATE_RUNNING,
               PROTOCOL_STATE_DISCONNECTED} ProtocolState;
  
typedef struct sOutstandingCall
{
  int id;
  /*sVoxiMutex lock; */
  pthread_mutex_t mutex;
  pthread_cond_t finishedCondition;
  Error error;
  TextRPCConnection connection;
  char *input;
  char *result;
} sOutstandingCall, *OutstandingCall;

typedef struct sIncomingCall
{
  char *input;
  TextRPCConnection connection;
} sIncomingCall, *IncomingCall;

/*
  "Public" types
 */

typedef struct sTextRPCServer 
{
  Server tcpServer;
  void *applicationServerData;
  
  TRPC_FunctionDispatchFunc dispatchFunc;
  TRPC_FunctionConnectionOpenedFunc connectionOpenedFunc;
  TRPC_FunctionConnectionClosedFunc connectionClosedFunc;

#ifdef TRPC_USE_THREADPOOL
  ThreadPool threadPool;
#endif
} sTextRPCServer;

typedef struct sTextRPCConnection
{
  pthread_mutex_t lock;
  /*sVoxiMutex lock; */
  /*int callId; */

  ProtocolState protocolState;  
  SocketConnection tcpSocketConnection; /* added this because lazy */
  Socket tcpConnection;
  
  int protocolVersion;
  TRPC_FunctionDispatchFunc dispatchFunc;
  
  /* sVoxiMutex nextCallIdMutex; */
  pthread_mutex_t nextCallIdMutex;
  unsigned int nextCallId;
  
  HashTable outstandingCalls;
  void *applicationData;
  TextRPCServer server;

  pthread_mutex_t initMutex;
  pthread_cond_t initFinishedCondition;

#ifdef TRPC_USE_THREADPOOL
  ThreadPool threadPool;
#endif
} sTextRPCConnection;


/*
 * static function prototypes
 */
static void server_sock_handler( SocketConnection connection, 
                                 TextRPCServer rpcServer, 
                                 sock_Event what, ... );
static void connection_sock_handler( SocketConnection connection, 
                                     TextRPCConnection client,
                                     sock_Event what, ... );
static void *negotiate_protocol( TextRPCConnection client );

static Error handleReturn( Boolean isException, TextRPCConnection client, 
                           char *message );
static void *call_threadFunc( IncomingCall call );

static int hashFunction( OutstandingCall call );
static int compFunction( OutstandingCall call1, OutstandingCall call2 );


static void *callConnectionOpened(TextRPCConnection connection);
static void *callConnectionClosed(TextRPCConnection connection);

/*
 *  The Code
 */

Error textRPCConnection_destroy(TextRPCConnection connection) {
  Error error = NULL;
  int err1, err2, err3, err4, err5, err6, err = 0;

  err1 = pthread_mutex_lock(&(connection->lock));
  if (err1 != 0)
    DEBUG("Warning, couldn't grab connection lock "
          "in TextRPCConnection_destroy\n");

  err2 = pthread_mutex_destroy(&(connection->nextCallIdMutex));
  if (err2 != 0)
    DEBUG("Couldn't destroy nextCallIdmutex when closing textrpc connection\n");
  
  err3 = pthread_mutex_destroy(&(connection->initMutex));
  if (err3 != 0)
    DEBUG("Couldn't destroy initMutex when closing textrpc connection\n");
  
  err4 = pthread_cond_destroy(&(connection->initFinishedCondition));
  if (err4 != 0)
    DEBUG("Couldn't destroy initFinishedCond when closing textrpc connection\n");
  
  HashDestroyTable(connection->outstandingCalls);

  /* THESE SHOULD BE DESTROYED ALSO */
  if (connection->tcpSocketConnection != NULL)
    free(connection->tcpSocketConnection);
  if (connection->tcpConnection != NULL)
    free(connection->tcpConnection);

  connection->tcpSocketConnection = NULL;
  connection->tcpConnection       = NULL;
  
  err5 = pthread_mutex_unlock(&(connection->lock));
  err6 = pthread_mutex_destroy(&(connection->lock));
  if (err6 != 0)
    DEBUG("Couldn't destroy lock when closing textrpc connection, "
          "err=%d strerr=%s \n", err6, strerror(err));
  
  if ((err = (err1 | err2 | err3 | err4 | err5 | err6)) != 0)
    error = ErrNew(ERR_TEXTRPC, 0, NULL,
                   "Couldn't destroy mutex when closing textrpc connection\n");

  free(connection);
  connection = NULL;
  
  return error;
}

/*
   Deficiency: Does not allow the caller to specify the interface to listen on
   Bug: does not support IPv6
   Deficiency: does not allow the caller to dynamically allocate the port
 */
Error textRPC_server_create( TRPC_FunctionDispatchFunc dispatchFunc,
                             TRPC_FunctionConnectionOpenedFunc connectionOpenedFunc,
                             TRPC_FunctionConnectionClosedFunc connectionClosedFunc,
                             void *applicationServerData, unsigned short port, 
                             TextRPCServer *server )
{
  Error error = NULL;
  struct in_addr address;
  unsigned short port2 = port;

  address.s_addr = INADDR_ANY;

  assert( server != NULL );
  *server = NULL;

  error = threading_init(); 
  if (error != NULL) {
    goto ERR1;
  }
  
#ifdef DEBUG_TEXTRPC_THREADS
  if (!debugThreadsInited) {
    error = threading_mutex_init(&noCallThreadsMutex);
    if (error != NULL) {
      goto ERR1;
    }
    debugThreadsInited = TRUE;
  }
#endif
    
  *server = malloc( sizeof( sTextRPCServer ) );
  assert( *server != NULL );
  
  (*server)->applicationServerData = applicationServerData;
  (*server)->dispatchFunc = dispatchFunc;
  (*server)->connectionOpenedFunc = connectionOpenedFunc;
  (*server)->connectionClosedFunc = connectionClosedFunc;

#ifdef TRPC_USE_THREADPOOL
  {
    pthread_attr_t attr;
    int err;

    err = pthread_attr_init(&attr);
    assert(err == 0);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Create a detached thread pool */
    error = threadPool_create(TRPC_THREADPOOL_SIZE,
                              attr,
                              &((*server)->threadPool));
    if (error != NULL)
     goto ERR1;
  }
#endif
  
  error = sock_create_server( (sock_handler) server_sock_handler, &address, &port,
                              (sock_handler) connection_sock_handler, 
                              *server, &((*server)->tcpServer) );

 ERR1:
  if( error != NULL )
  {
    free( *server );
    *server = NULL;
  }
  
  return error;
}

void textRPC_server_destroy( TextRPCServer server );

static Error connection_create( Socket socket, 
                                TRPC_FunctionDispatchFunc dispatchFunc,
                                TRPC_FunctionConnectionOpenedFunc connectionOpenedFunc,
                                TRPC_FunctionConnectionClosedFunc connectionClosedFunc,
                                void *applicationData,
                                TextRPCServer rpcServer,
                                TextRPCConnection *connection )
{
  int err;

#ifdef TRPC_USE_THREADPOOL
  ThreadPoolThread thread;
#else
  pthread_t thread;
#endif
  
  Error error = NULL;
    
  (*connection) = malloc( sizeof( sTextRPCConnection ) );
  assert( *connection != NULL );

  (*connection)->protocolState = PROTOCOL_STATE_INITIALIZING;
  err = pthread_mutex_init(&((*connection)->nextCallIdMutex), NULL );
  assert(err == 0);
  
  (*connection)->tcpConnection = socket;
  (*connection)->tcpSocketConnection = NULL;
  
  /* (*connection)->callId = 1; */ /*Fast hack*/
  (*connection)->nextCallId = 0;
  (*connection)->outstandingCalls = 
    HashCreateTable( OUTSTANDING_FUNC_HASH_SIZE, 
                     (HashFuncPtr) hashFunction, 
                     (CompFuncPtr) compFunction, NULL );
  (*connection)->server = rpcServer;
  (*connection)->dispatchFunc = dispatchFunc;
  (*connection)->applicationData = applicationData;
    
  err = pthread_mutex_init( &((*connection)->initMutex), NULL );
  assert( err == 0 );
    
  err = pthread_cond_init( &((*connection)->initFinishedCondition), NULL );
  assert( err == 0 );
  
  sock_setUserData( socket, *connection );
      
  /*
   *  Perform protocol version negotiation with the client
   *  Do this in a separate thread, so as not to block the server socket
   *  event handling.
   */
  (*connection)->protocolVersion = -1; /* -1 signals invalid/not negotiated yet */
  (*connection)->protocolState = PROTOCOL_STATE_NEGOTIATING_PROTOCOL_1;

#ifdef TRPC_USE_THREADPOOL
  (*connection)->threadPool = rpcServer->threadPool;
  error = threadPool_runThread((*connection)->threadPool,
                               (ThreadFunc) negotiate_protocol,
                               *connection, &thread);
#else
  err = threading_pthread_create( &thread, &detachedThreadAttr, 
                        (ThreadFunc) negotiate_protocol, 
                        *connection );
  assert( err == 0 );
#endif
  
  return error;
}

static void server_sock_handler( SocketConnection connection, 
                                 TextRPCServer rpcServer, 
                                 sock_Event what, ... )
{
  va_list args;
  ServerConn serverConn;
  /* Bag repositories; */
  TextRPCConnection client;
  Error error;
  
  va_start( args, what );
  
  switch( what )
  {
    case SOCK_CONNECTION:
      DEBUG( "textRPC.c: server_sock_handler: server port connection!\n" );
      serverConn = va_arg( args, ServerConn );
      
      error = connection_create( (Socket) serverConn, rpcServer->dispatchFunc,
                                 rpcServer->connectionOpenedFunc,
                                 rpcServer->connectionClosedFunc,
                                 rpcServer->applicationServerData,
                                 rpcServer, &client );
      assert( error == NULL );
#if 0
#endif
      break;
      
    case SOCK_CLOSED:
      printf("Sock closed.\n");
      DEBUG( "server port closed\n" );
      break;
      
    default:
      DEBUG( "ERROR: loopServer: server_sock_handler: what=%d.\n", what );
  }
  
  va_end( args );
}

static int hashFunction( OutstandingCall call )
{
  return call->id;
}

static int compFunction( OutstandingCall call1, OutstandingCall call2 )
{
  return call2->id - call1->id;
}

static void *negotiate_protocol( TextRPCConnection client )
{
  Error error;
  
  error = sock_send( (Socket) client->tcpConnection, 
                     "protocolVersion-request " LATEST_PROTOCOL_VERSION_STRING 
                     "\n" );
  assert( error == NULL );
  
  return NULL;
}

static void connection_sock_handler( SocketConnection connection, 
                                     TextRPCConnection client,
                                     sock_Event what, ... )
{
  va_list args;
  char *message, *charPtr;
  Error error;
  
#ifdef TRPC_USE_THREADPOOL
  ThreadPoolThread tempThread;
#else
  pthread_t tempThread;
  int err/*  , retries */;
#endif
  
  va_start( args, what );
  
  switch( what )
  {
    case SOCK_MESSAGE:
      message = va_arg( args, char *);
      
      DEBUG("textRPC.c: connection_sock_handler: message: '%s'.\n", message);
      
      switch( client->protocolState )
      {
        case PROTOCOL_STATE_INITIALIZING:
          assert( FALSE );
          
        case PROTOCOL_STATE_NEGOTIATING_PROTOCOL_1:
        case PROTOCOL_STATE_NEGOTIATING_PROTOCOL_2:
          DEBUG( "Negotioation message='%s'\n", message );
          
          charPtr = strsep( &message, " " );
          assert( charPtr != NULL );
          
          if( strcasecmp( charPtr, "protocolVersion-request" ) == 0 )
          {
            int remoteVersion;
            char buf[ 256 ];
            int err = 0;

#ifdef TRPC_USE_THREADPOOL
            ThreadPoolThread connOpenedThread;
#else
            pthread_t connOpenedThread;
#endif
            
            charPtr = strsep( &message, " " );
            assert( charPtr != NULL );
            
            remoteVersion = atoi( charPtr );
            
            /* Why in the name of Cthulu do we want this?? /JNi */
            /*assert( client->protocolVersion == -1 ); */
            
            client->protocolVersion = (remoteVersion > LATEST_PROTOCOL_VERSION) ?
              LATEST_PROTOCOL_VERSION : remoteVersion;
#if 1
            client->protocolState = PROTOCOL_STATE_RUNNING;
#else
            client->protocolState = PROTOCOL_STATE_NEGOTIATING_PROTOCOL_2;
#endif
            
            sprintf( buf, "protocolVersion %d\n", client->protocolVersion );
            
            error = sock_send( (Socket) connection, buf );
            assert( error == NULL );

            /* Broadcast that the initialization with the server */
            /* has been completed and that calls can now be made. */

            err = pthread_cond_broadcast( &(client->initFinishedCondition) );
            assert(err == 0);
            
            /* Send event to the handler/dispatch func
             * that a connection has been initialized. */
#ifdef TRPC_USE_THREADPOOL
            error = threadPool_runThread(client->threadPool,
                                         (ThreadFunc) callConnectionOpened,
                                         client, &connOpenedThread);
            assert(error == NULL);
#else
            err = threading_pthread_create( &connOpenedThread, &detachedThreadAttr,
                                            (ThreadFunc) callConnectionOpened,
                                            client);
            assert(err == 0);
#endif
          }
          else if( strcasecmp( charPtr, "protocolVersion" ) == 0 )
          {
            int version;
            int err = 0;

#ifdef TRPC_USE_THREADPOOL
            ThreadPoolThread connOpenedThread;
#else
            pthread_t connOpenedThread;
#endif
            
            /* assert( client->protocolState == 
               PROTOCOL_STATE_NEGOTIATING_PROTOCOL_2 );
            */
            
            charPtr = strsep( &message, " " );
            assert( charPtr != NULL );
            
            version = atoi( charPtr );
            
            assert( client->protocolVersion == -1 );
            assert( version <= LATEST_PROTOCOL_VERSION );
            
            client->protocolVersion = version;
            client->protocolState = PROTOCOL_STATE_RUNNING;
            
            /* Broadcast that the initialization with the server */
            /* has been completed and that calls can now be made. */

            err = pthread_cond_broadcast( &(client->initFinishedCondition) );
            assert(err == 0);
            
            /* Send event to the handler/dispatch func
             * that a connection has been initialized. */
#ifdef TRPC_USE_THREADPOOL
            error = threadPool_runThread(client->threadPool,
                                         (ThreadFunc) callConnectionOpened,
                                         client, &connOpenedThread);
            assert(error == NULL);
#else
            err = threading_pthread_create( &connOpenedThread, &detachedThreadAttr,
                                            (ThreadFunc) callConnectionOpened,
                                            client);
            assert(err == 0);
#endif
          }
          else
            DIAG("ERROR: textRPC.c: msg='%s'\n", charPtr );
          break;
        
        case PROTOCOL_STATE_RUNNING:
          /* fprintf( stderr, "Running message='%s'\n", message ); */
          charPtr = strsep( &message, " " );
          assert( charPtr != NULL );
          
          if( strcasecmp( charPtr, "C" ) == 0 )
          {
            IncomingCall call;
            
            call = malloc( sizeof( sIncomingCall ) );
            assert( call != NULL );
            
            call->input = strdup( message );
            call->connection = client;

#ifdef USE_THREADED_CALLS
            pthread_attr_setdetachstate(&detachedThreadAttr,
                                        PTHREAD_CREATE_DETACHED);

#ifdef TRPC_USE_THREADPOOL
            error = threadPool_runThread(client->threadPool,
                                         (ThreadFunc) call_threadFunc,
                                         call, &tempThread);
            assert(error == NULL);
#else
            err = threading_pthread_create( &tempThread, &detachedThreadAttr, 
                                            (ThreadFunc) call_threadFunc, call );
            if( err != 0 )
              DIAG("ERROR: util/textRPC.c: threading_pthread_create "
                   "failed, errno = %d.\n", errno );
#endif /* TRPC_USE_THREAD_POOL */
            
#else  /* no threads for incoming calls. Use for debugging only! */
            call_threadFunc(call);
#endif /* USE_THREADED_CALLS */
            
          }
          else if( strcasecmp( charPtr, "R" ) == 0 )
          {
            DEBUG( "loopServer.c: connection_sock_handler: "
                   "message: '%s', function return.\n", 
                   message ); 
            
            error = handleReturn( FALSE, client, message );
            if( error != NULL )
              ErrReport( error );
          }
          else if( strcasecmp( charPtr, "E" ) == 0 )
          {
            error = handleReturn( TRUE, client, message );
            if( error != NULL )
              ErrReport( error );
          }
          else if (strcasecmp( charPtr, "ping" ) == 0) {
            char pingBuf[256];
            sprintf( pingBuf, "ping-reply \n");
            error = sock_send(client->tcpConnection,
                              pingBuf);
          }
          else {
            DEBUG( "textRPC.c: connection_sock_handler: message: '%s', "
                   "invalid token (must start with 'C', 'R' or 'E').\n", 
                   message );
    }
    break;

      case PROTOCOL_STATE_DISCONNECTED:
  error = ErrNew( ERR_TEXTRPC, -1, NULL, "Reached bad protocol state "
      "PROTOCOL_STATE_DISCONNECTED, textRPC.c");
  assert( FALSE ); 
  break;
      }
      break;

    case SOCK_DISCONNECT:
      {
#ifdef TRPC_USE_THREADPOOL
        ThreadPoolThread connClosedThread;
#else
        pthread_t connClosedThread;
#endif
        DEBUG("textRPC.c: connection_sock_handler: disconnect.\n");
        /* Send event to the handler  func
         * that a connection has been closed. */
        
#ifdef TRPC_USE_THREADPOOL
        error = threadPool_runThread(client->threadPool,
                                     (ThreadFunc) callConnectionClosed,
                                     client, &connClosedThread);
        assert(error == NULL);
#else   
        err = threading_pthread_create( &connClosedThread, &detachedThreadAttr,
                                        (ThreadFunc) callConnectionClosed,
                                        client);
        if (err != 0)
          DIAG("ERROR: util/textRPC.c: threading_pthread_create "
               "failed when socket was disconnected, errno = %d.\n", errno );
#endif
      }
      break;
      
    default:
      DEBUG( "textRPC.c: connection_sock_handler: what=%d.\n", what );
  }
  
  va_end( args );
}

static Error handleReturn( Boolean isException, TextRPCConnection client, 
                           char *message )
{
  char *charPtr;
  sOutstandingCall callTemplate, *outstandingCall;
  Error error = NULL;
  int err;
  
  /* printf("Handling return %s \n", message);*/
  charPtr = strsep( &message, " \t" );
  
  callTemplate.id = atoi( charPtr );
  
  /* Find the outstanding call */
  outstandingCall = HashFind( client->outstandingCalls, &callTemplate );
  
  if( outstandingCall == NULL )
    error = ErrNew( ERR_APP, 0, NULL, "No outstanding call with id %d.", 
                    callTemplate.id );
  
  if( error == NULL )
  {
    err = pthread_mutex_lock(&((outstandingCall)->mutex));
    assert( err == 0 );
    
    if( isException )
    {
      outstandingCall->error = ErrNew( ERR_TEXTRPC, 
                                       TEXTRPC_ERR_REMOTE_EXCEPTION, NULL, 
                                       message );
      outstandingCall->result = NULL;
    }
    else
    {
      outstandingCall->result = strdup( message );
      outstandingCall->error = NULL;
    }

    err = pthread_mutex_unlock( &(outstandingCall->mutex) );
    assert( err == 0 );
    
    /*
      Notify the caller that the result is ready 
    */
    /* printf("Releasing return lock\n"); */
    err = pthread_cond_broadcast( &(outstandingCall->finishedCondition) );
    assert(err == 0);
  }

  return error;
}

static void *call_threadFunc( IncomingCall call )
{
  char *charPtr;
  char *message, *result;
  int callId;
  Error error, error2;
  StringBuffer outBuffer;

#ifdef DEBUG_TEXTRPC_THREADS
  threading_mutex_lock(&noCallThreadsMutex);
  noEnteredThreads++;
  fprintf(stderr, "ENTERING (entered %d, exited %d, running %d)\n",
          noEnteredThreads, noExitedThreads, noEnteredThreads-noExitedThreads);
  threading_mutex_unlock(&noCallThreadsMutex);
#endif
  
  message = call->input;
  
  charPtr = strsep( &message, " " );
  assert( charPtr != NULL );
            
  callId = atoi( charPtr );
  
  error = call->connection->dispatchFunc( 
                              call->connection->applicationData,
                              message, &result );
  
  error2 = strbuf_create( 256, &outBuffer );
  
  if( error2 == NULL )
  {
    if( error == NULL )
    {
      if( result == NULL )
        strbuf_sprintf( outBuffer, "R %d\n", callId );
      else
      {
        strbuf_sprintf( outBuffer, "R %d %s\n", callId, result );
        free( result );
      }
    }
    else
    {
      error2 = strbuf_sprintf( outBuffer, "E %d ", callId );
      
      if( error2 == NULL )
        error2 = ErrToString( error, outBuffer );
    
      if( error2 == NULL )
        error2 = strbuf_sprintf( outBuffer, "\n" );
    }
    /* ErrDestroy( error ); */ /* This function doesn't exist yet */
  }
  
  if( error2 == NULL ) {
  if (call->connection->tcpSocketConnection == NULL) 
    error2 = sock_send((Socket)call->connection->tcpConnection,
                       strbuf_getString( outBuffer ));
  else
    error2 = sock_send((Socket)call->connection->tcpSocketConnection, 
                       strbuf_getString( outBuffer ));
  }
  
  strbuf_destroy( outBuffer );
  free( call->input );
  free( call );
  
  if( error2 != NULL )
  {
    error2 = ErrNew( ERR_TEXTRPC, 0, error2, "Failed to send result" );
    
    ErrReport( error2 );

#ifdef DEBUG_TEXTRPC_THREADS
  threading_mutex_lock(&noCallThreadsMutex);
  noExitedThreads++;
  fprintf(stderr, "EXITING (entered %d, exited %d, running %d)\n",
          noEnteredThreads, noExitedThreads, noEnteredThreads-noExitedThreads);
  threading_mutex_unlock(&noCallThreadsMutex);
#endif

#ifdef USE_THREADED_CALLS
     pthread_exit( error2 );
#endif
     
  }
  
#ifdef DEBUG_TEXTRPC_THREADS
  threading_mutex_lock(&noCallThreadsMutex);
  noExitedThreads++;
  fprintf(stderr, "EXITING (entered %d, exited %d, running %d)\n",
          noEnteredThreads, noExitedThreads, noEnteredThreads-noExitedThreads);
  threading_mutex_unlock(&noCallThreadsMutex);
#endif
  return NULL;
}



Error textRPC_client_create( TRPC_FunctionDispatchFunc dispatchFunc, 
                             const char *host, unsigned short port, 
                             void *applicationClientData,
                             TextRPCConnection *connection )
{
  Error error = NULL;
  SocketConnection sockConnection;
  int err = 0;

  error = threading_init();
  if (error != NULL) {
    error = ErrNew(ERR_TEXTRPC, 0, error, "Failed to init threading");
    goto CLIENT_CREATE_FAIL_1;
  }

#ifdef DEBUG_TEXTRPC_THREADS
  if (!debugThreadsInited) {
    error = threading_mutex_init(&noCallThreadsMutex);
    if (error != NULL) {
      error = ErrNew( ERR_TEXTRPC, 0, error, "Failed to setup mutex for "
                      "the TextRPC connection." );
      
      goto CLIENT_CREATE_FAIL_1;
    }
    debugThreadsInited = TRUE;
  }
#endif
  
  error = emalloc( (void **) connection, sizeof( sTextRPCConnection ) );
  if( error != NULL )
  {
    error = ErrNew( ERR_TEXTRPC, 0, error, "Failed to allocate memory for "
                    "the TextRPC connection." );
    
    goto CLIENT_CREATE_FAIL_1;
  }
  
  err = pthread_mutex_init(&((*connection)->nextCallIdMutex), NULL);
  assert(err == 0);
  
  err = pthread_mutex_init(&((*connection)->initMutex), NULL);
  assert(err == 0);
  
  err = pthread_cond_init( &((*connection)->initFinishedCondition), NULL );
  assert( err == 0 );
  
  (*connection)->applicationData = applicationClientData;
  (*connection)->dispatchFunc = dispatchFunc;
  (*connection)->nextCallId = 1;
  (*connection)->protocolState = PROTOCOL_STATE_NEGOTIATING_PROTOCOL_1;
  (*connection)->protocolVersion = LATEST_PROTOCOL_VERSION; /* Me guess? */

  (*connection)->outstandingCalls = HashCreateTable(16,
                                                    (HashFuncPtr) hashFunction,
                                                    (CompFuncPtr)compFunction ,
                                                    (DestroyFuncPtr) NULL
                                                    );
  (*connection)->server = NULL;

#ifdef TRPC_USE_THREADPOOL
  {
    pthread_attr_t attr;
    int err;

    err = pthread_attr_init(&attr);
    assert(err == 0);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Create a detached thread pool */
    error = threadPool_create(TRPC_THREADPOOL_SIZE,
                              attr,
                              &((*connection)->threadPool));
    if (error != NULL)
     goto CLIENT_CREATE_FAIL_2;
  }
#endif
  
  error = sock_connect( host, port, (sock_handler) connection_sock_handler,
                        *connection, &sockConnection );
  if( error != NULL )
  {
    error = ErrNew( ERR_TEXTRPC, 0, error, 
                    "Failed to connect to host '%s', port %d.\n", host, port );
    goto CLIENT_CREATE_FAIL_2;
  }

  (*connection)->tcpConnection = NULL;
  (*connection)->tcpSocketConnection = sockConnection;
    
  err = pthread_mutex_lock(&((*connection)->initMutex));
  assert(err == 0);

  err = pthread_cond_wait( &((*connection)->initFinishedCondition), 
                           &((*connection)->initMutex));
  assert(err == 0);
   
  err = pthread_mutex_unlock(&((*connection)->initMutex));
  assert(err == 0);
  
  return NULL;
  
 CLIENT_CREATE_FAIL_2:
  pthread_mutex_destroy(&((*connection)->nextCallIdMutex));
  pthread_mutex_destroy(&((*connection)->initMutex));
  pthread_cond_destroy(&((*connection)->initFinishedCondition));
  
  HashDestroyTable( (*connection)->outstandingCalls );
  
  free( *connection );
  *connection = NULL;
  
 CLIENT_CREATE_FAIL_1:
  
  return error;
}



Error textRPC_call( TextRPCConnection connection, const char *text, char **result )
{
  Error error;
  char buffer[10000];
  char *callText;
  OutstandingCall outstandingCall;
  int callId;
  int err = 0;
  
  /*
  if (connection->tcpSocketConnection == NULL)
    printf("Socket null\n");
  else
    printf("Socket is not null\n");
  */
  
  err = pthread_mutex_init(&((connection)->lock),NULL); /* init the mutex */
  assert(err == 0);
  
  err = pthread_mutex_lock(&((connection)->lock));
  assert(err == 0);

  err = pthread_mutex_lock(&((connection)->nextCallIdMutex));
  if (err != 0)
    DIAG("Error in textRPC: %d\n", err);
  assert(err == 0);
  
  callId = (connection->nextCallId)++;
  err = pthread_mutex_unlock(&((connection)->nextCallIdMutex));
  
  DEBUG("callId: %i\n",callId);
  
  err = pthread_mutex_unlock(&((connection)->lock));
  assert(err == 0);
  
  sprintf(buffer,"C %i ",callId);

  callText = strcat(buffer,text);

  callText = strcat(callText,"\n");

  DEBUG("msg: %s\n",callText);
  
  outstandingCall = malloc( sizeof( sOutstandingCall ) );
  assert( outstandingCall != NULL );
  
  outstandingCall->id = callId;
  (outstandingCall)->connection = connection;
  (outstandingCall)->input = callText;
  outstandingCall->error = NULL;

  HashAdd((connection->outstandingCalls),outstandingCall); /*Really this simple? */

  err = pthread_mutex_init(&((outstandingCall)->mutex),NULL); /* init the mutex */
  err = pthread_cond_init(&((outstandingCall)->finishedCondition),NULL);
  assert(err == 0);
  
  err = pthread_mutex_lock(&((outstandingCall)->mutex));
  assert(err == 0);
  /*threading_mutex_lock(&((*outstandingCall)->mutex)); */

  /* printf("TextRPCSend: message %s\n", callText); */
  
  /* Fall back to the normal connection if we are not sending to a server */
  if (connection->tcpSocketConnection == NULL) 
    error = sock_send((Socket)connection->tcpConnection,
                      callText);
  else
    error = sock_send((Socket)connection->tcpSocketConnection,
                      callText);
  
  /*and block. */
  
  DEBUG("Blocking\n");
  
  
  err = pthread_cond_wait( &((outstandingCall)->finishedCondition), 
                           &((outstandingCall)->mutex));
  assert(err == 0);
  
  DEBUG("Blocking done\n");
  
  DEBUG("Not blocking\n");


  /*After return message was recived. */

  if ((outstandingCall)->error != NULL) /* We had an error */
    {
      printf("We have a socket error!\n");
      return (outstandingCall)->error;
    }

  /*copy the result to a "safe" place. */
  *result = strdup((outstandingCall)->result);
  
  HashDelete((connection->outstandingCalls),outstandingCall);

  free(outstandingCall); /*make ara happy. */

  return error;

}

static void *callConnectionOpened(TextRPCConnection connection) {
  if (connection->server)
    return connection->server->connectionOpenedFunc(connection);
  return NULL;
}
static void *callConnectionClosed(TextRPCConnection connection) {
  if (connection->server)
    return connection->server->connectionClosedFunc(connection);
  return NULL;
}
