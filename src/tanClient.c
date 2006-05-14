/*
 *  TAN Client implementation
 */
#include <assert.h>
#include <stdint.h>

#include <voxi/util/err.h>
#include <voxi/util/mem.h>
#include <voxi/util/tcpip.h>

#include <voxi/util/tanClient.h>

/* 
   Constants
*/
#define CURRENT_PROTOCOL_VERSION 0

typedef struct sTANClient
{
  TCPConnection connection;
  unsigned int protocolVersion;
  unsigned int id;
} sTANClient;

Error tanClient_connect( const char *hostName, unsigned short port,
                         TANClient *tan )
{
  Error error;
  uint32_t versionPacket = htonl( CURRENT_PROTOCOL_VERSION );
    
  error = emalloc( (void **) tan, sizeof( sTANClient ) );
  if( error != NULL )
    goto CONNECT_FAIL_1;

  error = tcp_connectName( hostName, port, &((*tan)->connection) );
  if( error != NULL )
    goto CONNECT_FAIL_2;

  /* Do version negotiation */

  /* send my protocol version number */
  error = tcp_write( (*tan)->connection, 4, (char *) &versionPacket );
  if( error != NULL )
  {
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, error, 
                    "Failed to send my protocol version number" );
    goto CONNECT_FAIL_3;
  }

  /* read the server's protocol version number */
  /* we should add a timeout here */
  error = tcp_read( (*tan)->connection, 4, (char *) &versionPacket );
  if( error != NULL )
  {
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, error, 
                    "Failed to read server protocol version number" );
    goto CONNECT_FAIL_3;
  }
  
  if( versionPacket < CURRENT_PROTOCOL_VERSION )
    (*tan)->protocolVersion = versionPacket;
  else
    (*tan)->protocolVersion = CURRENT_PROTOCOL_VERSION;
  
  /* Get my id number */
  error = tcp_read( (*tan)->connection, 4, (char *) &((*tan)->id) );
  if( error != NULL )
  {
    error = ErrNew( ERR_TCPIP, ERR_TCPIP_UNSPECIFIED, error, 
                    "Failed to read my id number" );
    goto CONNECT_FAIL_3;
  }
  
  (*tan)->id = ntohl( (*tan)->id );
  
  return NULL;
      
 CONNECT_FAIL_3:
  tcp_disconnect( (*tan)->connection );
  
 CONNECT_FAIL_2:
  free( *tan );
  
 CONNECT_FAIL_1:
  assert( error != NULL );
  
  return error;
}

Error tanClient_disconnect( TANClient tan )
{
  Error error;
  
  error = tcp_disconnect( tan->connection );
  if( error != NULL )
    ErrDispose( error, TRUE ); /* ignore */
  
  free( tan );
  
  return NULL;
}


Error tanClient_getMyID( TANClient tan, TANID *id )
{
  *id = tan->id;
  
  return NULL;
}

