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
#include <voxi/util/config.h>
#include <voxi/util/libcCompat.h>

class EXTERN_UTIL ByteQueue
{
private:
  char *buffer;
  int head;
  int tail;
  bool isFull;
  size_t size;
  bool endOfStream;
  bool endOfStreamReported;

	pthread_mutex_t mutex;
	pthread_cond_t condition;

public:
  ByteQueue( size_t size );
  ByteQueue::~ByteQueue();

  bool IsFull();
	bool IsEmpty();
  bool IsEndOfStream();
	//
	// WriteData returns TRUE on success, FALSE if some data was dropped
  // writeData may only be called if we are not at end of stream
	bool WriteData( const char *ptr, size_t byteCount );
  void WriteEndOfStream();
  void WriteStartOfStream();

  // read will block until there is data to return, or end of stream is 
  // signalled
  // the returned value is the number of bytes read. If this value is 
  // less than byteCount, then that means that the end of stream was reached
	size_t ReadData( char *ptr, size_t byteCount );
};

#endif