/*
 A Queue to isolate threads piping data from one to the other

 Policies: 
   read on empty: block
   write on full: drop
*/
#ifndef BYTE_QUEUE_H
#define BYTE_QUEUE_H

#include <voxi/util/err.h>
#include <voxi/util/libcCompat.h>
#include <voxi/types.h>

typedef struct sByteQueue *ByteQueue;

Error byteQueue_create( size_t size, ByteQueue *byteQueue );
void byteQueue_destroy( ByteQueue );

Boolean byteQueue_isFull( ByteQueue );
Boolean byteQueue_isEmpty( ByteQueue );
Boolean byteQueue_isEndOfStream( ByteQueue );

/*
  WriteData returns TRUE on success, FALSE if some data was dropped
  
  writeData may only be called if we are not at end of stream
*/
Boolean byteQueue_writeData( ByteQueue bq, const char *ptr, size_t byteCount );
void byteQueue_writeEndOfStream( ByteQueue bq );
void byteQueue_writeStartOfStream( ByteQueue bq );

/*
  read will block until there is data to return, or end of stream is 
  signalled
  the returned value is the number of bytes read. If this value is 
  less than byteCount, then that means that the end of stream was reached
*/
size_t byteQueue_readData( ByteQueue bq, char *ptr, size_t byteCount );

#endif
