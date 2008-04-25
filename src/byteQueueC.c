#include <assert.h>
#include <pthread.h>
#include <stdio.h> /* For debugging */

#include <voxi/util/byteQueue.h>
#include <voxi/util/err.h>
#include <voxi/util/logging.h>
#include <voxi/util/mem.h>

LOG_MODULE_DECL( "ByteQueue", LOGLEVEL_WARNING);

#define DEBUG 0

/* 
   Type definitions 
*/
typedef struct sByteQueue
{
  char *buffer;
  int head;
  int tail;
  Boolean isFull;
  size_t size;
  Boolean endOfStream;
  Boolean endOfStreamReported;
  
  pthread_mutex_t mutex;
  pthread_cond_t condition;
} sByteQueue;

static FILE *debugLogFile = NULL;
/*
 * Buffer:
 *
 *      HEAD      TAIL
 *   0 1  2  3 4 5  6 7 8 9 10 
 *        A  B C D    
 * 
 *     TAIL          HEAD    
 *   0   1  2  3 4 5  6   7 8 9 10 
 *   F                A   B C D  E
 *
 * read from head, write to tail
 */

Error byteQueue_create( size_t size, ByteQueue *byteQueue )
{
  Error error;
	int err;
  
	assert( size != 0 );
  
  error = emalloc( (void **) byteQueue, sizeof( sByteQueue ) );
  if( error != NULL )
    goto CREATE_ERROR_1;
  
  (*byteQueue)->isFull = FALSE;
  (*byteQueue)->head = 0;
  (*byteQueue)->tail = 0;
  (*byteQueue)->endOfStream = TRUE;
  (*byteQueue)->endOfStreamReported = FALSE;
	(*byteQueue)->size = size;
  
  error = emalloc( (void **) &((*byteQueue)->buffer), size );
  if( error != NULL )
    goto CREATE_ERROR_2;
  
	err = pthread_mutex_init( &((*byteQueue)->mutex), NULL );
	assert( err == 0 );

	err = pthread_cond_init( &((*byteQueue)->condition), NULL );
	assert( err == 0 );

  LOG_DEBUG( LOG_DEBUG_ARG, "Created byteQueue %p", byteQueue );

#if DEBUG
	debugLogFile = fopen( "c:\\temp\\byteQueue.log", "w" );
#endif
  
  return NULL;
  
 CREATE_ERROR_2:
  free( *byteQueue );
  
 CREATE_ERROR_1:
  assert( error != NULL );
  
  return error;
}

void byteQueue_destroy( ByteQueue byteQueue )
{
  free( byteQueue->buffer );
  pthread_mutex_destroy( &(byteQueue->mutex) );
  pthread_cond_destroy( &(byteQueue->condition) );
  
  free( byteQueue );
}

Boolean byteQueue_isFull( ByteQueue bq )
{
	return bq->isFull && (bq->head == bq->tail);
}

Boolean byteQueue_isEmpty( ByteQueue bq )
{
	return (bq->head == bq->tail) && !bq->isFull;
}

Boolean byteQueue_isEndOfStream( ByteQueue bq )
{
	return bq->endOfStream;
}

// Returns TRUE if all data was written successfully,
// FALSE if data was dropped.
Boolean byteQueue_writeData( ByteQueue bq, const char *data, size_t length )
{
	size_t bytesToCopy;
	Boolean noDrop = TRUE, wasEmpty;
	int err;

  LOG_DEBUG( LOG_DEBUG_ARG, "%p->WriteData( %p, %d )", bq, data, length );

  assert( !byteQueue_isEndOfStream(bq) );

	err = pthread_mutex_lock( &(bq->mutex) );
	assert( err == 0 );

	wasEmpty = byteQueue_isEmpty( bq );

#if DEBUG
	fprintf( debugLogFile, "Before WriteData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", this, data, (int) length, head, tail, 
		wasEmpty, this->IsFull() );
#endif

	while( length > 0 )
	{
		if( byteQueue_isFull( bq ) )
		{
			noDrop = FALSE;
			break; /* drop */
		}

		/* Copy source from TAIL to (HEAD - 1 or end of buffer) whichever comes 
       first */
		if( bq->head > bq->tail )
			bytesToCopy = bq->head - bq->tail;
		else
			bytesToCopy = bq->size - bq->tail;

		if( bytesToCopy > length )
			bytesToCopy = length;
	
		assert( bytesToCopy > 0 );

		memcpy( &(bq->buffer[ bq->tail ]), data, bytesToCopy );
	
		data += bytesToCopy;
		bq->tail += bytesToCopy;
		assert( bq->tail <= bq->size );
		if( bq->tail == bq->size )
			bq->tail = 0;

		length -= bytesToCopy;

		if( bq->head == bq->tail )
			bq->isFull = TRUE;

		if( wasEmpty )
		{
			assert( !byteQueue_isEmpty( bq ) );
			pthread_cond_broadcast( &(bq->condition) );
			wasEmpty = FALSE;
		}
	}

#if DEBUG
	fprintf( debugLogFile, "After WriteData( %p, %p, %d ): head=%d, tail=%d, "
           "isEmpty=%d, isFull=%d, noDrop=%d.\n", bq, data, (int) length, 
           bq->head, bq->tail, byteQueue_isEmpty( bq ), byteQueue_isFull( bq ),
           noDrop );
	fflush( debugLogFile );
#endif

	err = pthread_mutex_unlock( &(bq->mutex) );
	assert( err == 0 );

	return noDrop;
}

void byteQueue_writeEndOfStream( ByteQueue bq )
{
  int err;

  LOG_DEBUG( LOG_DEBUG_ARG, "%p->WriteEndOfStream()", (void *) bq );

  err = pthread_mutex_lock( &(bq->mutex) );
  assert( err == 0 );

  if( bq->endOfStream )
  {
    // LOG_ERROR( LOG_ERROR_ARG, "ByteQueue: WriteEndOfStream when already at end of stream." );

    goto ALREADY_AT_END;
  }
  assert( !bq->endOfStream );

  bq->endOfStreamReported = FALSE;
  bq->endOfStream = TRUE;
  
  /* the readthread may be waiting for more data */
	err = pthread_cond_broadcast( &(bq->condition) ); 
  assert( err == 0 );

ALREADY_AT_END:
  err = pthread_mutex_unlock( &(bq->mutex) );
  assert( err == 0 );
}

void byteQueue_writeStartOfStream( ByteQueue bq )
{
  int err;

  LOG_DEBUG( LOG_DEBUG_ARG, "%p->WriteStartOfStream()", (void *) bq );

  err = pthread_mutex_lock( &(bq->mutex) );
  assert( err == 0 );

  assert( bq->endOfStream );

  // If the end of stream has not been reported, block until it has
  while( !bq->endOfStreamReported )
  {
    err = pthread_cond_wait( &(bq->condition), &(bq->mutex) );
    assert( err == 0 );
  }
  
  bq->endOfStream = FALSE;

  err = pthread_mutex_unlock( &(bq->mutex) );
  assert( err == 0 );
}

// Behaviour: block on read
size_t ByteQueue_readData( ByteQueue bq, char *data, size_t readRequest )
{
	size_t bytesToCopy;
	int err;
  size_t result;
  size_t length;

  LOG_DEBUG( LOG_DEBUG_ARG, "%p->ReadData( %p, %d )", bq, data, 
             readRequest );

  length = readRequest;

	err = pthread_mutex_lock( &(bq->mutex) );
	assert( err == 0 );
	
	assert( length > 0 );

#if DEBUG
	fprintf( debugLogFile, "Before ReadData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", bq, data, (int) length, bq->head, bq->tail, 
		byteQueue_isEmpty( bq ), byteQueue_isFull( bq ) );
#endif

  result = 0;

	while( length > 0 )
	{
		if( byteQueue_isEmpty( bq ) )
    {
      LOG_TRACE( LOG_TRACE_ARG, "%p->ReadData: IsEmpty, eos=%d, "
                 "eosReported=%d", bq, byteQueue_isEndOfStream( bq ), 
                 bq->endOfStreamReported );

      if( byteQueue_isEndOfStream( bq ) && 
          !bq->endOfStreamReported )
        break;
      else
      {
        LOG_TRACE( LOG_TRACE_ARG, "%p->ReadData: waiting for data", bq );

			  pthread_cond_wait( &(bq->condition), &(bq->mutex) );

        LOG_TRACE( LOG_TRACE_ARG, "%p->ReadData: got condition change", bq );

        // the condition may have been signalled from the end of stream 
        // function, in which case we should do the IsEmpty check again
        continue;
      }
    }

		if( bq->tail <= bq->head )
			bytesToCopy = bq->size - bq->head;
		else
			bytesToCopy = bq->tail - bq->head;

		if( bytesToCopy > length )
			bytesToCopy = length;

		assert( bytesToCopy > 0 );

		memcpy( data, &(bq->buffer[ bq->head ]), bytesToCopy );

		if( bq->head == bq->tail ) // then it was full, because it was not empty
		{
			assert( bq->isFull );
			bq->isFull = FALSE;
		}
		data += bytesToCopy;
		bq->head += bytesToCopy;
    result += bytesToCopy;

		assert( bq->head <= bq->size );
		if( bq->head == bq->size )
			bq->head = 0;
		length -= bytesToCopy;
	}

#if DEBUG
	fprintf( debugLogFile, "After ReadData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", bq, data, (int) length, bq->head, bq->tail, 
		byteQueue_isEmpty( bq ), byteQueue_IsFull( bq ) );
	fflush( debugLogFile );
#endif

  if( result < readRequest )
  {
    assert( byteQueue_isEndOfStream( bq ) );
    bq->endOfStreamReported = TRUE;

    // broadcast because eos is now reported
    pthread_cond_broadcast( &(bq->condition) );
  }

	pthread_mutex_unlock( &(bq->mutex) );

  return result;
}
