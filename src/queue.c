/*
   queue.c
   */

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>

#include <voxi/util/mem.h>
#include <voxi/util/queue.h>

typedef struct sQueueEntry *QueueEntry;

typedef struct sQueueEntry
{
  QueueEntry newer;
  void *userData;
} sQueueEntry;

typedef struct sQueue
{
  pthread_mutex_t mutex;
  sem_t semaphore;

  QueueEntry oldest;
  QueueEntry newest;
} sQueue;

/*
 * static functions
 */
static QueueEntry doPop( Queue queue );

/*
 * Start of code
 */
Error queue_create( Queue *result )
{
  Error error;
  int err;

  error = emalloc( result, sizeof( sQueue ) );
  if( error != NULL )
    return error;

  err = sem_init( &((*result)->semaphore), 0, 0 );
  assert( err == 0 );

  (*result)->mutex = PTHREAD_MUTEX_INITIALIZER;
  (*result)->oldest = NULL;
  (*result)->newest = NULL;

  return NULL;
}

Error queue_destroy( Queue queue )
{
  int err;

  err = pthread_mutex_lock( &(queue->mutex) );
  assert( err == 0 );

  if( queue->newest != NULL )
    return ErrNew( ERR_QUEUE, ERR_QUEUE_NOT_EMPTY, NULL, "The queue was not "
                   "empty, which it must be in order to be destroyed" );

  assert( queue->oldest == NULL );

  err = sem_destroy( &(queue->semaphore) ) ;
  assert( err == 0 );

  err = pthread_mutex_unlock( &(queue->mutex) );
  assert( err == 0 );

  err = pthread_mutex_destroy( &(queue->mutex) ) ;
  assert( err == 0 );

  free( queue );

  return NULL;
}

Error queue_push( Queue queue, void *userElement )
{
  int err;
  Error error;
  QueueEntry element;

  error = emalloc( (void **) &element, sizeof( sQueueEntry ) );
  assert( error == NULL );

  element->userData = userElement;

  err = pthread_mutex_lock( &(queue->mutex) );
  assert( err == 0 );

  element->newer = queue->newest;

  if( queue->oldest == NULL )
  {
    assert( queue->newest == NULL );
    
    queue->oldest = element;
  }

  queue->newest = element;

  err = pthread_mutex_unlock( &(queue->mutex) );
  assert( err == 0 );

  err = sem_post( &(queue->semaphore) );
  assert( err == 0 );

  return NULL;
}

/* Returns TRUE if the queue contains elements */
Boolean queue_poll( const Queue queue )
{
  return (queue->newest != NULL);  
}

Error queue_pop( Queue queue, void **result )
{
  int err;
  QueueEntry element;

  err = sem_wait( &(queue->semaphore) );

  element = doPop( queue );

  *result = element->userData;

  free( element );

  return NULL;
}

static QueueEntry doPop( Queue queue )
{
  int err;
  QueueEntry element;

  err = pthread_mutex_lock( &(queue->mutex) );
  assert( err == 0 );

  assert( queue->oldest != NULL );
  assert( queue->newest != NULL );

  element = queue->oldest;
  queue->oldest = element->newer;

  if( queue->oldest == NULL )
  {
    assert( queue->newest == element );
    queue->newest = NULL;
  }

  err = pthread_mutex_unlock( &(queue->mutex) );
  assert( err == 0 );

  return element;
}
/*
  if timeoutTime is NULL, then no timeout is used (the function will wait 
  forever.
  */
Error queue_waitFor( Queue queue, const struct timespec *timeoutTime, void **result )
{
  int err;
  QueueEntry element;

  err = sem_timedwait( &(queue->semaphore), timeoutTime );
  if( err != 0 )
  {
    Error error;

    assert( err == -1 );

    if( errno == ETIMEDOUT )
      error = ErrNew( ERR_QUEUE, ERR_QUEUE_TIMEDOUT, NULL, "Wait timed out" );
    else
      error = ErrNew( ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, ErrErrno(), "sem_timedwait failed" );

    return error;
  }

  element = doPop( queue );

  assert( result != NULL );

  *result = element->userData;

  free( element );

  return NULL;
}
