//
#include <afx.h> // junk to avoid windows include conficts.

#include <assert.h>
#include <queue>
#include <pthread.h>

#include "byteQueue.hpp"

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
ByteQueue::ByteQueue( size_t size ): isFull( false ), head( 0 ), tail( 0 )
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

// Returns TRUE if all data was written successfully,
// FALSE if data was dropped.
bool ByteQueue::WriteData( const char *data, size_t length )
{
	size_t bytesToCopy;
	bool noDrop = true, wasEmpty;
	int err;

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
			pthread_cond_signal( &condition );
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

// Behaviour: block on read
void ByteQueue::ReadData( char *data, size_t length )
{
	size_t bytesToCopy;
	int err;

	err = pthread_mutex_lock( &mutex );
	assert( err == 0 );
	
	assert( length > 0 );

#ifdef DEBUG
	fprintf( debugLogFile, "Before ReadData( %p, %p, %d ): head=%d, tail=%d, "
		"isEmpty=%d, isFull=%d.\n", this, data, (int) length, head, tail, 
		IsEmpty(), IsFull() );
#endif

	while( length > 0 )
	{
		if( IsEmpty() )
			pthread_cond_wait( &condition, &mutex );

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

	pthread_mutex_unlock( &mutex );
}
