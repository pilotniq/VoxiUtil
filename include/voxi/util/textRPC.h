/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  textRPC.h
  
  Implements a Remote Procedure Call (RPC) protocol using text-based 
  messages over TCP.
    On top of this protocol may be layered application bindings to functions
  etcetera.

  Depends on the sock.h layer, which should be properly initialized.
*/

#include <voxi/util/err.h>
#ifdef __cplusplus
extern "C" {  /* only need to export C interface if used by C++ source code */
#endif


enum { TEXTRPC_ERR_REMOTE_EXCEPTION = 1 };

typedef struct sTextRPCServer *TextRPCServer;
typedef struct sTextRPCConnection *TextRPCConnection;

typedef Error (*TRPC_FunctionDispatchFunc)( void *applicationServerData, 
                                            char *callText, char **result );

typedef Error (*TRPC_FunctionConnectionOpenedFunc)( TextRPCConnection connection );
typedef Error (*TRPC_FunctionConnectionClosedFunc)( TextRPCConnection connection );
                                           
EXTERN_UTIL
Error textRPC_server_create( TRPC_FunctionDispatchFunc dispatchFunc,
                             TRPC_FunctionConnectionOpenedFunc connectionOpenedFunc,
                             TRPC_FunctionConnectionClosedFunc connectionClosedFunc,
                             void *applicationServerData, unsigned short port, 
                             TextRPCServer *server );

EXTERN_UTIL
Error textRPC_client_create( TRPC_FunctionDispatchFunc dispatchFunc, 
                             const char *host, unsigned short port, 
                             void *applicationClientData,
                             TextRPCConnection *connection );

EXTERN_UTIL
void textRPC_server_destroy( TextRPCServer server );

/**
 * Destroys a text rpc connection and frees it
 */
EXTERN_UTIL
Error textRPCConnection_destroy( TextRPCConnection connection );

EXTERN_UTIL
Error textRPC_call( TextRPCConnection connection, const char *text, char **result );

#ifdef __cplusplus
}  /* only need to export C interface if used by C++ source code */
#endif

