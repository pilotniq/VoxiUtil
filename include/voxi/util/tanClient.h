/*
 * TAN Client: NAT circumvention library (for a client)
 */

#include <voxi/util/err.h>

typedef struct sTANClient *TANClient;
typedef int TANID;

EXTERN_UTIL Error tanClient_connect( const char *hostName, unsigned short port,
                                     TANClient *tan );
EXTERN_UTIL Error tanClient_disconnect( TANClient tan );

EXTERN_UTIL Error tanClient_getMyID( TANClient tan, TANID *id );

EXTERN_UTIL Error tanClient_sendPacket( TANClient tan, TANID recipient, 
                                        char *data, size_t length );
EXTERN_UTIL Error tanClient_readPacket( TANClient tan, char **data, 
                                        size_t *size );
