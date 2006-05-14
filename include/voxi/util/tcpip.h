/*
 * Generic TCP/IP library. Underlies sock.[ch] and tanClient.[ch]
 */

#include <voxi/util/libcCompat.h> /* for EXTERN_UTIL */

typedef enum { ERR_TCPIP_UNSPECIFIED };

typedef struct sTCPConnection *TCPConnection;
typedef int *TCPServerSocket;

EXTERN_UTIL Error tcp_connectName( const char *host, unsigned short port, 
                                   TCPConnection *connection );
/*
 *port must be either 0 for a dynamically assigned port, or else a port 
 Fix this later so that it uses somethign prettier than struct in_addr
*/
/*
EXTERN_UTIL Error tcp_server_create( struct in_addr *address,
                                     unsigned short *port, 
                                     TCPServerSocket *serverSocket );

EXTERN_UTIL Error tcp_server_accept( TCPServerSocket serverSocket, 
                              TCPConnection *connection );
EXTERN_UTIL Error tcp_server_destroy( TCPServerSocket serverSocket );
*/
EXTERN_UTIL Error tcp_disconnect( TCPConnection connection );
EXTERN_UTIL Error tcp_read( TCPConnection connection, size_t size, char *data);
EXTERN_UTIL Error tcp_write( TCPConnection connection, size_t size, 
                             char *data );
