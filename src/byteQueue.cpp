//
//#include <afx.h> // junk to avoid windows include conficts.

#include <assert.h>
#include <queue>
#include <pthread.h>

#include <voxi/util/byteQueue.hpp>

#define DEBUG 1

static FILE *debugLogFile = NULL;
//
// Buffer:
//
//      HEAD      TAIL
//   0 1  2  3 4 5  6 7 8 9 10 
//        A  B C D    
// 
//     TAIL          HEAD    
//   0   1  2  3 4 5  6   7 8 9 10 
//   F                A   B C D  E
//
// read from head, write to tail
//
ByteQueue::ByteQueue( size_t size ): isFull( false ), head( 0 ), tail( 0 ), 
                                     endOfStream( true ), endOfStreamReported( false )
{
	int err;

	assert( size != 0 );

	buffer = new char[ size ];
	assert( buffer != NULL );

	this->size = size;

	assert( IsEmpty() );
	assert( !IsFull() );

	err = pthread_mutex_init( &mutex, NULL );
	assert( err == 0 );

	err = pthread_cond_init( &condition, NULL );
	assert( err == 0 );

	// create mutex & cond
#if DEBUG
	debugLogFile = fopen( "c:\\temp\\byteQueue.log", "w" );
#endif
}

bool ByteQueue::IsFull()
{
	return isFull && (head == tail); //((tail + 1) == head) || ((tail == (size - 1)) && head == 0);
}

bool ByteQueue::IsEmpty()
{
	return (head == tail) && !isFull;
}

bool ByteQueue::IsEndOfStream()
{
	return endOfStream;
}

// Returns TRUE if all data was written successfully,
// FALSE if data was dropped.
bool ByteQueue::WriteData( const char *data, size_t length )
{
	size_t bytesToCopy;
	bool noDrop = true, wasEmpty;
	int err;

  assert( !IsEndOfStream() );

	err = pthread_mutex_lock( &(mutex) );
	assert( err == 0 );

	wasEmpty = this->IsEmpty();

#ifdef DEBUG
	fprintf( debugLogFile, "Before WriteData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", this, data, (int) length, head, tail, 
		wasEmpty, this->IsFull() );
#endif

	while( length > 0 )
	{
		if( this->IsFull() )
		{
			noDrop = false;
			break; // drop
		}

		// Copy source from TAIL to (HEAD - 1 or end of buffer) whichever comes first
		if( head > tail )
			bytesToCopy = head - tail;
		else
			bytesToCopy = size - tail;

		if( bytesToCopy > length )
			bytesToCopy = length;
	
		assert( bytesToCopy > 0 );

		memcpy( &(buffer[ tail ]), data, bytesToCopy );
	
		data += bytesToCopy;
		tail += bytesToCopy;
		assert( tail <= size );
		if( tail == size )
			tail = 0;

		length -= bytesToCopy;

		if( head == tail )
			isFull = true;

		if( wasEmpty )
		{
			assert( !IsEmpty() );
			pthread_cond_broadcast( &condition );
			wasEmpty = false;
		}
	}

#if DEBUG
	fprintf( debugLogFile, "After WriteData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d, noDrop=%d.\n", this, data, (int) length, head, tail, 
		this->IsEmpty(), this->IsFull(), noDrop );
	fflush( debugLogFile );
#endif

	err = pthread_mutex_unlock( &mutex );
	assert( err == 0 );

	return noDrop;
}

void ByteQueue::WriteEndOfStream()
{
  int err;

  err = pthread_mutex_lock( &mutex );
  assert( err == 0 );

  assert( !endOfStream );

  endOfStreamReported = false;
  endOfStream = true;

	err = pthread_cond_broadcast( &condition ); // the readthread may be waiting for more data
  assert( err == 0 );

  err = pthread_mutex_unlock( &mutex );
  assert( err == 0 );
}

void ByteQueue::WriteStartOfStream()
{
  int err;

  err = pthread_mutex_lock( &mutex );
  assert( err == 0 );

  assert( endOfStream );
  assert( endOfStreamReported ); // actually, we should wait until it is reported

  endOfStream = false;

  err = pthread_mutex_unlock( &mutex );
  assert( err == 0 );
}

// Behaviour: block on read
size_t ByteQueue::ReadData( char *data, size_t readRequest )
{
	size_t bytesToCopy;
	int err;
  size_t result;
  size_t length;

  length = readRequest;

	err = pthread_mutex_lock( &mutex );
	assert( err == 0 );
	
	assert( length > 0 );

#ifdef DEBUG
	fprintf( debugLogFile, "Before ReadData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", this, data, (int) length, head, tail, 
		IsEmpty(), IsFull() );
#endif

  result = 0;

	while( length > 0 )
	{
		if( IsEmpty() )
    {
      if( IsEndOfStream() && !endOfStreamReported )
        break;
      else
      {
			  pthread_cond_wait( &condition, &mutex );

        // the condition may have been signalled from the end of stream 
        // function, in which case we should do the IsEmpty check again
        continue;
      }
    }

		if( tail <= head )
			bytesToCopy = size - head;
		else
			bytesToCopy = tail - head;

		if( bytesToCopy > length )
			bytesToCopy = length;

		assert( bytesToCopy > 0 );

		memcpy( data, &(buffer[head]), bytesToCopy );

		if( head == tail ) // then it was full, because it was not empty
		{
			assert( isFull );
			isFull = false;
		}
		data += bytesToCopy;
		head += bytesToCopy;
    result += bytesToCopy;

		assert( head <= size );
		if( head == size )
			head = 0;
		length -= bytesToCopy;
	}

#ifdef DEBUG
	fprintf( debugLogFile, "After ReadData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", this, data, (int) length, head, tail, 
		IsEmpty(), IsFull() );
	fflush( debugLogFile );
#endif

  if( result < readRequest )
  {
    assert( IsEndOfStream() );
    printf( "ByteQueue: setting eos reported to true\n" ); 
    endOfStreamReported = true;
  }

	pthread_mutex_unlock( &mutex );

  return result;
}
