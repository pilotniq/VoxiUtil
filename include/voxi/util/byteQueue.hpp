//
// A Queue to isolate threads piping data from one to the other
//
// Policies: 
//   read on empty: block
//   write on full: drop
//
#ifndef BYTE_QUEUE_HPP
#define BYTE_QUEUE_HPP

// #include <queue>
#include <pthread.h>

class ByteQueue
{
private:
	char *buffer;
	int head;
	int tail;
	bool isFull;
	size_t size;

	pthread_mutex_t mutex;
	pthread_cond_t condition;

public:
	ByteQueue( size_t size );

	bool IsFull();
	bool IsEmpty();
	//
	// WriteData returns TRUE on success, FALSE if some data was dropped
	bool WriteData( const char *ptr, size_t byteCount );
	void ReadData( char *ptr, size_t byteCount );
};

#endif