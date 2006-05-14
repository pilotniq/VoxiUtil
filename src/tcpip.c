/*
 * tcpip.c
 *
 * Generic low-level TCP/IP Library
 */

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

#include <voxi/util/err.h>
#include <voxi/util/mem.h>
#include <voxi/util/tcpip.h>

/*
 * Type definitions
 */
typedef struct sTCPConnection
{
  struct sockaddr_in addr;
  int socket;
} sTCPConnection;

/*
 * static function prototypes
 */
static const char *voxi_sock_error( int err );

/* 
 * Start of code
 */
Error tcp_connectName( const char *hostName, unsigned short port, 
                       TCPConnection *connection )
{
  struct hostent *hostinfo;
  Error error;
  int tempint;
  
  /* preconditions */
  assert( connection != NULL );
  
  /* Look up the host name to IP address */
  hostinfo = gethostbyname( hostName );
  
  if(hostinfo == NULL) 
  {
    /* Different error handling on windows and linux */
#ifdef WIN32
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED,
                    ErrNew( ERR_UNKNOWN, 0, NULL, 
                    voxi_sock_error(WSAGetLastError())), 
                    "Failed to find the host '%s'.", computer );
#else
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, 
                    ErrNew( ERR_UNKNOWN, 0, NULL, voxi_sock_error( h_errno )), 
                    "Failed to find the host '%s'.", hostName );
#endif
    goto CONNECT_FAIL_1;
  }
  
  /* Allocate the connection */
  error = emalloc( (void **) connection, sizeof( sTCPConnection ) );
  if( error != NULL )
  {
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, error, 
                    "Failed to allocate memory for the connection" );
    goto CONNECT_FAIL_1;
  }

  bzero( (char *) &( (*connection)->addr), sizeof( (*connection)->addr ));

  (*connection)->addr.sin_family = AF_INET;
  (*connection)->addr.sin_addr =*((struct in_addr *) hostinfo->h_addr_list[0]);
  (*connection)->addr.sin_port = htons( port );

  (*connection)->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if((*connection)->socket < 0) 
  {
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, ErrErrno(), 
                    "Failed to create a TCP socket.");
    
    goto CONNECT_FAIL_2;
  }
  
  tempint = connect( (*connection)->socket, 
                     (struct sockaddr *) &(*connection)->addr,
                     sizeof(struct sockaddr_in));

  /* This is strange - it seems that the connect call completes/succeeds even
     if we get the EINTR error. */
  if(tempint != 0 && (errno != EINTR))
  {
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, ErrSock(), 
                    "Failed to connect to the server." );
  
    goto CONNECT_FAIL_3;
  }

  return NULL;
  
 CONNECT_FAIL_3:
  close( (*connection)-> socket );
  
 CONNECT_FAIL_2:
  free( *connection );
  
 CONNECT_FAIL_1:
  assert( error != NULL );
  
  return error;
}

Error tcp_disconnect( TCPConnection connection )
{
  close( connection->socket );
  free( connection );
  
  return NULL;
}

/* add timeout? 
   Only one thread may call read
 */
Error tcp_read( TCPConnection connection, size_t size, char *data)
{
  int tempint;
  int remainBufLen = size;
  char *dataPtr;
  Error error = NULL;
  
  assert( connection != NULL );
  
  dataPtr = data;
  
  do
  {
    tempint = recv( connection->socket, dataPtr, remainBufLen, 0 );
    if( tempint < 0 )
    {
      int err;
    
      assert( tempint == -1 ); /* according to docs */
#ifdef WIN32
      err = WSAGetLastError();
    
      if( errno == WSAEINTR )
        continue;
      else
        error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, NULL, 
                        "recv error: %s", voxi_sock_error( err ) );
#else
      error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, ErrErrno(), 
                      "recv error" );
#endif
    }
    else
    {
      assert( tempint > 0 );
      
      dataPtr += tempint;
      remainBufLen -= tempint;
    }
  } while( (remainBufLen > 0) && (error == NULL) );
  
  return error;
}

Error tcp_write( TCPConnection connection, size_t size, char *data )
{
  int count;
  
  count = write( connection->socket, data, size );
  assert( count == size ); /* fix later */
  
  return NULL;
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
      sprintf( buf, "Unknown h_error code: %d", err );
      return strdup( buf );
  }
#endif
}
