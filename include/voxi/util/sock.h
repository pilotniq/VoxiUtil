/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  socket-interface routines
  */

#ifndef SOCK_H
#define SOCK_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <voxi/util/err.h>
#include <voxi/types.h>

typedef struct s_Connection *SocketConnection;
typedef struct s_Server *Server;
typedef struct s_ServerConn *ServerConn;
typedef struct s_Socket *Socket;


typedef enum sock_EventEnum { SOCK_CONNECTION, SOCK_MESSAGE, SOCK_OVERFLOW,
                              SOCK_DISCONNECT, SOCK_CLOSED } sock_Event;

/*
  possible values for 'what' are:
  
  SOCK_MESSAGE: a line of text has been received. One argument follows 'what';
                a (char *), the line that was received. It is only valid until 
                the sock_handler returns.
*/
  
typedef void (*sock_handler)(SocketConnection connection, void *id, 
                             sock_Event what, ...);

EXTERN_UTIL Error sock_init();
EXTERN_UTIL Error sock_end();

EXTERN_UTIL Error sock_connect( const char *computer, unsigned short port, 
                                sock_handler handler, void *id, 
                                SocketConnection *connection );
EXTERN_UTIL void sock_disconnect(SocketConnection c);

/*
 * sending a port number 0 will dynamically allocate a port. 
 * The port number can be retrieved with sock_server_getPortNumber
 */
EXTERN_UTIL Error sock_create_server( sock_handler handler, 
                                      struct in_addr *address, 
                                      unsigned short *port, 
                                      sock_handler connectionHandler, 
                                      void *userData,
                                      Server *server );
EXTERN_UTIL void sock_destroy_server(Server s);
EXTERN_UTIL char *sock_serv_conn_adrs_c(ServerConn sc);

/*
  Note, msg must end in a newline 
*/
EXTERN_UTIL Error sock_send(Socket c, const char *msg);
/*
 *  For sending data which may contain '\0'-bytes.
 */
EXTERN_UTIL Error sock_send_binary(Socket c, const char *msg, size_t length);

EXTERN_UTIL void sock_setUserData( Socket c, void *data );

/* reads a line from a socket, skipping lines beginning with '#' (comments) */
/* can return an errno */
EXTERN_UTIL int sock_readLine( int sock, char *buffer, int bufSize );

/* Functionality similar to fgets(3) but reads from a socket instead. */
EXTERN_UTIL char *sock_gets(char *buf, int size, int sock);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

