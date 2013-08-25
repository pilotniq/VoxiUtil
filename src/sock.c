/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  socket-functions

  The pthreads version.
*/

#include "config.h"

#ifdef hpux 
#include <stropts.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

/*
 * Win32 related socket includes
 */

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

/*
 * Unix/Linux related socket includes
 */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for getpid */
#endif

/* ----- */

#include <errno.h>
#include <string.h>

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <voxi/alwaysInclude.h>
#include <voxi/types.h>
#include <voxi/util/threading.h>
#include <voxi/util/err.h>
#include <voxi/util/file.h>
#include <voxi/util/bt.h>
#include <voxi/util/logging.h>
#include <voxi/util/mem.h>
#include <voxi/util/threadpool.h>
#include <voxi/util/sock.h>

CVSID("$Id$");

#ifndef HAVE_BZERO
#define bzero(s, n) memset(s, 0, n)
#endif

#define SOCK_BUF_SIZE 10000
#define SOCK_USE_THREADPOOL
#define SOCK_THREADPOOL_SIZE 5

typedef enum { ST_CONNECTION, ST_SERVER, ST_SERVCONN } SocketType;

typedef struct s_Socket
{
  SocketType typ;
  int socket;
  void *data; /* client-supplied data */
} t_Socket;

typedef struct s_Connection
{
  SocketType typ;
  int socket;
  void *data; /* client-supplied data */

  /* SocketConnection next, prev;*/

  sock_handler handler;
  
  char *computer;
  struct sockaddr_in addr;

  int buflen;
  char buffer[SOCK_BUF_SIZE];

  pthread_t thread;
} t_Connection;

typedef struct s_Server
{
  SocketType typ;
  int socket;
  void *data;
  sock_handler handler;

  struct sockaddr_in addr;
  int port;
  sock_handler connectionHandler;
  pthread_t thread;

#ifdef SOCK_USE_THREADPOOL
  ThreadPool threadPool;
#endif
} t_Server;

typedef struct s_ServerConn
{
  SocketType typ;
  int socket;
  void *data;

  sock_handler handler;
  
  Server server;
  /* ServerConn next, prev; */
  struct sockaddr_in addr;
  int buflen;
  char buffer[SOCK_BUF_SIZE];

#ifdef SOCK_USE_THREADPOOL
  ThreadPoolThread thread;
#else
  pthread_t thread;
#endif
} t_ServerConn;

/* External variables */
#ifndef WIN32
extern int h_errno; /* This may be Linux-specific? */
#endif

/* Static functions: */
static void *ConnectionThreadFunc( SocketConnection connection );
static void *server_thread_func( Server server );
static const char *voxi_sock_error( int err );

LOG_MODULE_DECL( "voxiUtil/sock", LOGLEVEL_TRACE );

/*
 *  The Code.
 */
Error sock_init(void)
{
  Error error = NULL;
  
  ErrPushFunc("sock_init"); 

#if 0
  firstServer = NULL;
  firstConn = NULL;

  fd_tree = bt_create((CompFuncPtr) socketCompare);
  FD_ZERO(&fdset);
  fd_width = -1;

  signal(SIGIO, sock_sig_handler);
  signal(SIGPIPE, sock_sig_handler); /* for disconnects */
#endif

#ifdef WIN32
  {
    int wsaErr;
    WSADATA wsaData;
    if ((wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
      error = ErrNew(ERR_SOCK, 0,
                     ErrNew(ERR_WINSOCK, wsaErr, NULL, "WinSock error %d",
                            wsaErr),
                     "Couldn't init socket module.");
  }
#endif
  
  ErrPopFunc();

  return error;
}

Error sock_end(void)
{
  Error error = NULL;
  
#if 0 /* #ifdef WIN32 */
  if (WSACleanup() != 0)
    error = ErrNew(ERR_SOCK, 0, ErrSock(),
                   "Error when shutting down socket module.");
#endif
  
  return error;
}

#if 1
#define sock_socket socket

#else
#ifndef WIN32
#define sock_socket socket
#else
/* We want a non-overlapped socket, to be able to use write, fdopen, etc. */
#define sock_socket(domain, type, protocol) \
  WSASocket((domain), (type), (protocol), NULL, 0, 0)
#endif /* WIN32 */
#endif /* 1 or 0 :-) */

/*---------------------------------------------------------------------*/
Error sock_connect(const char *computer, unsigned short port, sock_handler readproc,
                   void *data, SocketConnection *connection )
/*---------------------------------------------------------------------*/
{
  struct hostent *hostinfo;
  int tempint;
  Error error;
  
  assert( connection != NULL );
  
  error = emalloc( (void **) connection, sizeof( t_Connection ) );
  if( error != NULL )
  {
    error = ErrNew( ERR_SOCK, 0, error, "Failed to allocate memory for the "
                    "connection" );
    goto CONNECT_FAIL_1;
  }
  
  (*connection)->computer = _strdup( computer );
  (*connection)->typ = ST_CONNECTION;
  (*connection)->buflen = 0;
  (*connection)->handler = readproc;
  (*connection)->data = data;

  /* first look up computer name */

  hostinfo = gethostbyname( computer );
  
  if(hostinfo == NULL) 
  {
    /* Different error handling on windows and linux */
#ifdef WIN32
    error = ErrNew( ERR_SOCK, 0, 
                    ErrNew( ERR_UNKNOWN, 0, NULL, 
                    voxi_sock_error(WSAGetLastError())), 
                    "Failed to find the host '%s'.", computer );
#else
    error = ErrNew( ERR_SOCK, 0, 
                    ErrNew( ERR_UNKNOWN, 0, NULL, voxi_sock_error( h_errno )), 
                    "Failed to find the host '%s'.", computer );
#endif
    goto CONNECT_FAIL_2;
  }

  bzero( (char *) &( (*connection)->addr), sizeof( (*connection)->addr ));

  (*connection)->addr.sin_family = AF_INET;
  /* bcopy( hostinfo->h_addr, &c->addr.sin_addr, hostinfo->h_length );*/
  (*connection)->addr.sin_addr = *((struct in_addr *) hostinfo->h_addr_list[0]);
  (*connection)->addr.sin_port = htons( port );

  (*connection)->socket = sock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if((*connection)->socket < 0) 
  {
    error = ErrNew( ERR_SOCK, 0, ErrErrno(), "Failed to create a TCP socket.");
    
    goto CONNECT_FAIL_2;
  }
  

  /*
    order of things here is rather critical...
    (things must be done before signals are enabled for the socket
  */
/*   FD_SET(c->socket, &fdset); */
/*   fd_width = MAX(c->socket, fd_width); */

/*   c->next = firstConn; */
/*   c->prev = NULL; */
/*   if(c->next != NULL) */
/*     c->next->prev = c; */

/*   firstConn = c; */

  tempint = connect( (*connection)->socket, 
                     (struct sockaddr *) &(*connection)->addr,
                     sizeof(struct sockaddr_in));

  /* This is strange - it seems that the connect call completes/succeeds even
     if we get the EINTR error. */
  if(tempint != 0 && (errno != EINTR))
  {
    error = ErrNew( ERR_SOCK, 0, ErrSock(), "Failed to connect to the "
                    "server." );
  
    goto CONNECT_FAIL_3;
  }
#if 0
   /* Send signals for this socket to this process */

   if(fcntl(c->socket, F_SETOWN, getpid()) == -1)
     ERR ERR_ABORT, "Could not set group owner" ENDERR;

   /* Make socket non-blocking */

   if(fcntl(c->socket, F_SETFL, FNDELAY) == -1)
     ERR ERR_ABORT, "Could not set group owner" ENDERR;

   /* Turn signals on for the socket */

#ifdef hpux
   if( ioctl( c->socket, I_SETSIG, S_HANGUP | S_RDNORM ) != 0 )
     ERR ERR_ABORT, "Could not set asyncronous I/O for the stream" ENDERR;
#else
   if(fcntl(c->socket, F_SETFL, FASYNC) == -1)
     ERR ERR_ABORT, "Could not set FASYNC flag" ENDERR;
#endif

   bt_add(fd_tree, c);
#endif
   /* Start a new thread to read from the socket, and make appropriate 
      callbacks. */

   DEBUG( "sock.c: Staring ConnectionThread\n");

   tempint = threading_pthread_create( &( (*connection)->thread), 
                                       &detachedThreadAttr, 
                                       (ThreadFunc) ConnectionThreadFunc, 
                                       *connection );
   if( tempint != 0 )
   {
     error = ErrNew( ERR_SOCK, 0, ErrErrno(), "Failed to create socket "
                     "handling thread." );
     goto CONNECT_FAIL_3;
   }
   
   return NULL;
   
 CONNECT_FAIL_3:
   close( (*connection)->socket );
   
 CONNECT_FAIL_2:
   free( *connection );
   (*connection) = NULL;
   
 CONNECT_FAIL_1:
   assert( error != NULL );
   return error;
}

void sock_disconnect(SocketConnection c)
{
  /* The socket's thread will terminate itself when it reads EOF? */
  /* NO - we should terminate it. */
  close(c->socket);
  free( c->computer );
  free( c );
}

/*
   Changed 2004-08-12: port parameter changed to allow dynamically allocated 
                       ports and specified IP address.

   address must not be NULL.
   address must be either INADDR_ANY, or an IP address of the local machine.

   port must not be NULL.
   if *port is 0 a port will be dynamically assigned, and returned in *port
   otherwise the port *port will be used.

   BUG: only supports IPv4
   */
Error sock_create_server( sock_handler serverproc, struct in_addr *address, 
                          unsigned short *port,
                          sock_handler connectionHandler, void *userData,
                          Server *server )
{
  int err;
  Error error = NULL;
  
  ErrPushFunc("sock_create_server(...,port = %d)", port);
  
  assert( server != NULL );
  assert( address != NULL );
  assert( port != NULL );

  *server = malloc(sizeof(t_Server));
  /* (*server)->port = port; */
  (*server)->handler = serverproc;
  /* result->firstConn = NULL; */
  (*server)->typ = ST_SERVER;
  (*server)->connectionHandler = connectionHandler;
  (*server)->data = userData;

#ifdef SOCK_USE_THREADPOOL
  {
    pthread_attr_t attr;
    err = pthread_attr_init(&attr);
    assert(err == 0);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    error = threadPool_create(SOCK_THREADPOOL_SIZE, attr,
                              &((*server)->threadPool));
    if (error != NULL)
      goto ERR1;
  }
#endif
  
  /* Get socket */
  (*server)->socket = sock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if( (*server)->socket < 0)
    error = ErrNew( ERR_SOCK, 0, ErrSock(), "Couldn't create socket" );
  
  if( error == NULL )
  {
    /* bind socket to address */
    
    (*server)->addr.sin_family = AF_INET;
    (*server)->addr.sin_addr = *address; /* was INADDR_ANY; */
    (*server)->addr.sin_port = htons( *port );
    
    if( bind( (*server)->socket, (struct sockaddr *) &((*server)->addr),
            sizeof(struct sockaddr_in)) < 0)
      error = ErrNew( ERR_SOCK, 0, ErrSock(), "Couldn't bind socket" );
  
    if( error == NULL )
    {
      if( ((*port) == 0) || (address->s_addr == INADDR_ANY) )
      {
        int namelen;

        namelen = sizeof( (*server)->addr );

        err = getsockname( (*server)->socket, (struct sockaddr *) &((*server)->addr), &namelen );
        if( err == 0 )
        {
          *port = (*server)->addr.sin_port;
          *address = (*server)->addr.sin_addr;
        }
        else
        {
          assert( err == -1 ); /* docs say err is -1 on error */

          error = ErrNew( ERR_SOCK, 0, ErrSock(), "getsockname for new socket failed." );
        }

      }

      if( error == NULL )
        if( listen( (*server)->socket, 5) == -1 )
          error = ErrNew( ERR_SOCK, 0, ErrSock(), "Counldn't listen" );
    
      if( error == NULL )
      {
        err = threading_pthread_create( &( (*server)->thread ), &detachedThreadAttr, 
                                        (ThreadFunc) server_thread_func, *server );
        if( err != 0 )
          error = ErrNew( ERR_SOCK, 0, ErrErrno(), 
                          "Couldn't create listener thread" );
      }
    }
    
    if( error != NULL )
      close( (*server)->socket );
  } /* end of if the socket was created successfully... */

 ERR1:
  if( error != NULL )
    free( *server );
  
  ErrPopFunc();
  
  return error;
}

static void *server_thread_func( Server server )
{
#ifndef SOCK_USE_THREADPOOL
  int err;
#endif
  struct sockaddr addr;
  int addrlen = sizeof( addr );
  int new_fd;
  ServerConn serverConn;
  Error error = NULL;
  
  while( 1 )
  {
    addrlen = sizeof(addr);
    new_fd = accept( server->socket, &addr, &addrlen );
    
    if( new_fd == -1 )
    {
      Error error;
      
      error = ErrNew( ERR_SOCK, 0, ErrErrno(), 
                      "sock.c: server_thread_func: accept( %d, %p, %d ) "
                      "failed.", server->socket, &addr, addrlen );
      ErrReport( error );
      
      ErrDispose( error, TRUE );
      assert( FALSE );
    }
  
    /* create a connection object for the new server connection */
    serverConn = malloc( sizeof( t_ServerConn ) );
    assert( serverConn != NULL );
    
    serverConn->typ = ST_SERVCONN;
    serverConn->socket = new_fd;
    serverConn->data = NULL;
    serverConn->server = server;
    serverConn->handler = server->connectionHandler;
      
    assert( addrlen <= sizeof( struct sockaddr_in ) );
    
    memcpy( &(serverConn->addr), &addr, addrlen );
    serverConn->buflen = 0;
    
    server->handler( (SocketConnection) server, server->data, SOCK_CONNECTION, 
                     serverConn );
    
    /*
      threading_pthread_create moved to after the previous call. 
      
      Otherwise we could get socket input before the connection field had a 
      chance to get initalized by the application.
      
      This actually caused a SEGF when connection->data was still NULL in
      the ConnectionThreadFunc.
    */
#ifdef SOCK_USE_THREADPOOL
    error = threadPool_runThread(server->threadPool,
                                 (ThreadFunc) ConnectionThreadFunc,
                                 serverConn, &(serverConn->thread));
    assert(error == NULL);
#else
    err = threading_pthread_create( &(serverConn->thread), &(detachedThreadAttr), 
                          (ThreadFunc) ConnectionThreadFunc, serverConn );
    assert( err == 0 );
#endif
  }
  
  return NULL;
}

                                
#if 0 /* ^2 */
void sock_conn_send(SocketConnection c, char *msg)
{
  ErrPushFunc("sock_conn_send(%p, ...)", c);

  if(write(c->socket, msg, strlen(msg)) != strlen(msg))
    ERR ERR_WARN, "write failed." ENDERR;

  ErrPopFunc();
}
#endif
#if 0
void sock_destroy_server(Server s)
{
  int flags;

  /* should close all of the server's connections gracefully */

  if(s->prev == NULL)
    firstServer = s->next;
  else
    s->prev->next = s->next;

  s->handler(NULL, SOCK_CLOSED);

  /* signals must be turned off before the socket is closed, or the program
     will receive a signal after the socket has been closed, and the signal
     handler won't be able to identify what socket got the signal.
     */
#ifdef SOCK_USE_THREADPOOL
  error = threadPool_destroy(s->threadPool);
  if (error != NULL) { 
    ErrReport(error);
    ErrDispose(error);
  }
#endif

#ifdef hpux
  if( ioctl( s->socket, I_SETSIG, 0 ) != 0 )
    ERR ERR_ABORT, "Could not turn off async io for socket" ENDERR;
#else
  flags = fcntl( s->socket, F_GETFL );
  if( flags == -1 )
    ERR ERR_ABORT, "Could not get socket flags" ENDERR;

  flags &= ~FASYNC;

  if(fcntl(s->socket, F_SETFL, flags) == -1)
    ERR ERR_ABORT, "Could not set socket flags" ENDERR;
#endif
  FD_CLR(s->socket, &fdset);
  bt_remove(fd_tree, s, FALSE);

  if(close(s->socket) == -1)
    ERR ERR_WARN, "Couldn't close socket %d!?", s->socket ENDERR;

  free(s);
}
#endif
char *sock_serv_conn_adrs_c(ServerConn sc)
{
  return(inet_ntoa(sc->addr.sin_addr));
}

Error sock_send(Socket c, const char *msg)
{
  return sock_send_binary( c, msg, strlen( msg ) );
}


Error sock_send_binary(Socket c, const char *msg, size_t length)
{
  size_t count;
  Error error = NULL;
  
  /* struct strbuf cntrlbuf, databuf; */

  ErrPushFunc("sock_conn_send(...)");

  LOG_DEBUG( LOG_DEBUG_ARG, "sock_send_binary( %p, '%s', %d)\n", c, msg, length );

#ifdef WIN32
#if 1
  count = send(c->socket, msg, length, 0);
#else
  {
    /* Very urrky. Should have a cached handle in socket struct. */
    int handle = _open_osfhandle(c->socket, _O_APPEND);
    if (handle == -1) {
      DEBUG("sock_send: Disaster!\n");
      return;
    }
    else 
      DEBUG("sock_send: open_osfhandle succeded: %d\n", handle);
    count = write(handle, msg, strlen(msg));
  }
#endif
#else  /* not WIN32 */
  /* Well, send exists on Unix too - but let's continue to use write
     for now. */
  count = write(c->socket, msg, length);
#endif /* if WIN32 ... else ... */
  
  if( count == -1 )
  {
    error = ErrNew( ERR_SOCK, 0, ErrSock(), 
                    "send_binary(%d, '%s', %d) failed.", c->socket, msg, 
                    length );
  }
  else if( count != length )
    ERR ERR_WARN, "Problem writing, count=%d", count ENDERR;
#if 0
  err = fsync( c->socket );
  if( err != 0 )
  {
    error = ErrNew( ERR_UNKNOWN, 0, ErrErrno(), "Failed to fsync socket." );
    ErrReport( error );
    ErrDispose( error );
    error = NULL;  }
#endif
  ErrPopFunc();

  return error;
}

#if 0
static int socketCompare(Socket s1, Socket s2)
{
  return(s2->socket - s1->socket);
}
#endif
static void *ConnectionThreadFunc( SocketConnection connection )
{
#define _BUFSIZE 10000 /*256;*/
/*    const int bufSize = 2048; */
  Boolean done = FALSE;
  char buffer[_BUFSIZE];
#ifndef WIN32
  FILE *stream;
#endif
  int err;
  
  ErrPushFunc( "sock.c:ConnectionThreadFunc( %p )", connection );

  /* This is the main function of the thread which reads lines from the socket
     and dispatches them to the application-supplied socket handler */

#ifndef WIN32
  stream = fdopen( connection->socket, "r" );
  assert (stream != NULL);
#endif /* WIN32 */

  DEBUG("sock.c: ConnectionThreadFunc, before loop\n");
  
  while( !done ) 
  {
#ifndef WIN32
    err = file_readLine( stream, buffer, _BUFSIZE );    
#else
    err = sock_readLine( connection->socket, buffer, _BUFSIZE );
#endif

    LOG_TRACE( LOG_TRACE_ARG, "ConnectionThreadFunc: after readLine, err=%d", 
                               err );

    DEBUG( "sock.c: after file_readline\n" );
    /* if (err != 0)
       printf("Sock after readline: err %d \n", err); */
    switch( err )
    {
      case 0:
        LOG_TRACE( LOG_TRACE_ARG, "ConnectionThreadFunc: calling "
                   "connection->handler with SOCK_MESSAGE" );
        connection->handler( connection, connection->data, SOCK_MESSAGE, 
                             buffer );
        break;
        
      case ESPIPE:
      case EBADF:
        connection->handler( connection, connection->data, SOCK_DISCONNECT );
        done = TRUE;
        break;
        
      case EINTR:
        LOG_WARNING( LOG_WARNING_ARG, "ConnectionThreadFunc: read interrupted." );
        /* Just try again */
        break;

        /* This is what is returned when socket closed on windows */
      case -1:
        LOG_TRACE( LOG_TRACE_ARG, "ConnectionThreadFunc: calling handler with "
                                  "SOCK_DISCONNECT=%d.\n", SOCK_DISCONNECT );
        connection->handler( connection, connection->data, SOCK_DISCONNECT );
        done = TRUE;
        break;
        
      default:
        LOG_ERROR( LOG_ERROR_ARG, "ConnectionThreadFunc: Socket error: %s", 
                   strerror( errno ) );
        connection->handler( connection, connection->data, SOCK_DISCONNECT );
        done = TRUE;
    }
    
#ifndef WIN32
    /* If sock end of file socket is closed */
    if ((feof(stream)) && (!done)) {
      connection->handler( connection, connection->data, SOCK_DISCONNECT );
      done = TRUE;
    }
#endif

  }
  DEBUG( "sock.c:ConnectionThreadFunc:after loop.\n" );

  /* fclose( stream ); */

  ErrPopFunc();

  return NULL;
}

void sock_setUserData( Socket c, void *data )
{
  c->data = data;
}

/**
 * Error codes for: UNIX: herrno, Windows WSAGetLastError
 */
static const char *voxi_sock_error( int err )
{
#ifdef WIN32
  char *reason;

  switch (err) {
    case WSANOTINITIALISED:
      reason = "A successful WSAStartup must occur before using this function.";
      break;
    case WSAENETDOWN:
      reason = "The network subsystem has failed.";
      break;
    case WSAHOST_NOT_FOUND:
      reason = "Authoritative Answer Host not found.";
    case WSATRY_AGAIN:
      reason = "Non-Authoritative Host not found, or server failure.";
      break;
    case WSANO_RECOVERY:
      reason = "Nonrecoverable error occurred.";
      break;
    case WSANO_DATA:
      reason = "Valid name, no data record of requested type.";
      break;
    case WSAEINPROGRESS:
      reason = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
      break;
    case WSAEFAULT:
      reason = "The name parameter is not a valid part of the user address space.";
      break;
    case WSAEINTR:
      reason = "A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
      break;
    default:
      reason = "Unknown socket error %d", err;
  }

  return reason;
#else
  char buf[ 256 ];
  
  switch( err )
  {
    case HOST_NOT_FOUND:
      return "The specified host is unknown";
      
    case NO_ADDRESS:
      return "The requested name is valid, but it does not have an IP address";
      
    case NO_RECOVERY:
      return "A non-recoverable name server error occurred";
      
    case TRY_AGAIN:
      return "A temporary error occurred on an authoritative name server. "
        "Try again later.";
        
    default:
      /* This is a memory leak, but we ignore it because it is so rare. */
      snprintf( buf, sizeof(buf), "Unknown h_error code: %d", err );
      return strdup( buf );
  }
#endif
}

Boolean sock_ip_is_private( struct in_addr address )
{
#ifdef WIN32
  /* FIXME: Port me! */
  return 0;
#else
  uint32_t hip;
  uint16_t topTwo;
  
  hip = ntohl( (uint32_t) address.s_addr );
  topTwo = hip >> 16;
    
  return( (topTwo & 0xff00 == 0x0a00) || 
          (topTwo >= 0xac10 && topTwo < 0xac20) ||
          (topTwo == 0xc0a8) );
#endif /* WIN32 */
}

#ifdef WIN32 
/* reads a line from a socket, skipping lines beginning with '#' (comments) */
/* can return an errno */
int sock_readLine( int sock, char *buffer, int bufSize )
{
  char *tempPtr;

  do
  {
    tempPtr = sock_gets( buffer, bufSize, sock );

    if( tempPtr == NULL ) {
      return -1; /* errno; */
    }
  }
  while( /* !feof( file ) && */ (buffer[0] == '#') );

  /* remove any newline characters at the end of line */

  if( (tempPtr != NULL) && (strlen(buffer) > 0) )
  {
    size_t lastIndex;
    
    lastIndex = strlen( buffer ) - 1;
    
    while( (buffer[ lastIndex ] == '\n') || (buffer[ lastIndex ] == '\r') )
      lastIndex--;

    buffer[ lastIndex + 1 ] = '\0';
  }
#if 0
    /* if( !feof( file ) ) */
  buffer[ strlen( buffer ) - 1 ] = '\0';
#endif
  return 0;
}

/* Functionality similar to fgets(3) but reads from a socket instead. */
char *sock_gets(char *buf, int size, int sock)
{
  int count, rcount, err;

  for (count = 0; count < size-1; count++) 
  {
    rcount = recv(sock, &(buf[count]), 1, 0);

    if (rcount <= 0) 
    {
      err = WSAGetLastError();

      LOG_DEBUG( LOG_DEBUG_ARG, "sock_gets: rcount = %d < 0, err = %d, "
                                "WSAEINTR=%d", rcount, err, WSAEINTR );
      /*printf("RECV Error: %d \n", err);*/
      if (err == WSAEINTR)
      {
        LOG_DEBUG( LOG_DEBUG_ARG, "sock_gets: WSA Interrupted system call" );
        continue; /* I think we should try again if this happens? */
      }
      /* Something went wrong or the socket was closed. */
      /* fprintf(stderr, "sock_gets: recv failed!\n"); */
      return NULL;
    }
    if (buf[count] == '\n') {
      count++;
      break;
    }
  }
  buf[count] = '\0';
  
  /* fprintf(stderr, "sock_gets: '%s'\n", buf); */
  return buf;
}
#endif /* WIN32 */
