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
static void validate( Queue queue );

/*
 * Start of code
 */
Error queue_create( Queue *result )
{
  Error error = NULL;
  int err;

  error = emalloc( (void **) result, sizeof( sQueue ) );
  if( error != NULL )
    return error;

  err = sem_init( &((*result)->semaphore), 0, 0 );
  if (err != 0) {
    return ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL, "sem_init failed");
  }

  /* (*result)->mutex = PTHREAD_MUTEX_INITIALIZER; */
  err = pthread_mutex_init( &( (*result)->mutex), NULL );
  if (err != 0) {
    return ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL, "pthread_mutex_init failed");
  }
  
  (*result)->oldest = NULL;
  (*result)->newest = NULL;

  validate( *result );

  return NULL;
}

Error queue_destroy( Queue queue )
{
  Error error = NULL;
  int err;

  err = pthread_mutex_lock( &(queue->mutex) );
  if (err != 0) {
    return ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                  "queue_destroy: pthread_mutex_lock failed");
  }

  validate( queue );

  if( queue->newest != NULL )
    return ErrNew( ERR_QUEUE, ERR_QUEUE_NOT_EMPTY, NULL, "The queue was not "
                   "empty, which it must be in order to be destroyed" );

  assert( queue->oldest == NULL );

  err = sem_destroy( &(queue->semaphore) ) ;
  if (err != 0) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                   "queue_destroy: sem_destroy failed");
    pthread_mutex_unlock( &(queue->mutex) );
    /* Shouldn't really matter if the unlock fails in this stage...
       Of course, if the Error API would support it we could add
       a reason to the Error we are already emitting. */
    goto ERR_RETURN;
  }

  err = pthread_mutex_unlock( &(queue->mutex) );
  if (err != 0) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                   "queue_destroy: pthread_mutex_unlock failed");
    goto ERR_RETURN;
  }

  err = pthread_mutex_destroy( &(queue->mutex) ) ;
  if (err != 0) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                   "queue_destroy: pthread_mutex_destroy failed");
    goto ERR_RETURN;
  }

  free( queue );

 ERR_RETURN:
  return error;
}

Error queue_push( Queue queue, void *userElement )
{
  int err;
  Error error = NULL;
  QueueEntry element;

  error = emalloc( (void **) &element, sizeof( sQueueEntry ) );
  if (error != NULL) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, error,
                   "queue_push: emalloc failed");
    goto ERR_RETURN;
  }

  element->userData = userElement;

  err = pthread_mutex_lock( &(queue->mutex) );
  if (err != 0) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                   "queue_push: pthread_mutex_lock failed");
    goto ERR_RETURN;
  }

  validate( queue );

  element->newer = NULL;
  
  if( queue->newest == NULL ) {
    assert( queue->oldest == NULL );

    queue->oldest = element;
  }
  else {
    queue->newest->newer = element;
  }

  queue->newest = element;

  validate( queue );

  err = pthread_mutex_unlock( &(queue->mutex) );
  if (err != 0) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                   "queue_push: pthread_mutex_unlock failed");
    goto ERR_RETURN;
  }

  err = sem_post( &(queue->semaphore) );
  if (err != 0) {
    error = ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                   "queue_push: sem_post failed");
    goto ERR_RETURN;
  }

 ERR_RETURN:
  return error;
}

/* Returns TRUE if the queue contains elements */
Boolean queue_poll( const Queue queue )
{
  Boolean retval = FALSE;
  pthread_mutex_lock(&(queue->mutex));
  retval = (queue->newest != NULL);
  pthread_mutex_unlock(&(queue->mutex));
  return retval;  
}

Error queue_pop( Queue queue, void **result )
{
  int err;
  QueueEntry element;

  err = threading_sem_wait( &(queue->semaphore) );
  assert( err == 0 );
  
  element = doPop( queue );
  if (element == NULL) {
    return ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                  "queue_pop: doPop failed");
  }

  *result = element->userData;

  free( element );

  return NULL;
}

static QueueEntry doPop( Queue queue )
{
  int err;
  QueueEntry element = NULL;

  err = pthread_mutex_lock( &(queue->mutex) );
  if (err != 0) {
    goto ERR_RETURN;
  }

  validate( queue );

  assert( queue->oldest != NULL );
  assert( queue->newest != NULL );

  element = queue->oldest;
  queue->oldest = element->newer;

  if( queue->oldest == NULL )
  {
    assert( queue->newest == element );
    queue->newest = NULL;
  }

  validate( queue );

  pthread_mutex_unlock( &(queue->mutex) );

 ERR_RETURN:
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

    if( errno == ETIMEDOUT )
      error = ErrNew( ERR_QUEUE, ERR_QUEUE_TIMEDOUT, NULL, "Wait timed out" );
    else {
      Error err2 = ErrErrno();
      error = ErrNew( ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, err2, "sem_timedwait failed" );
    }

    return error;
  }

  element = doPop( queue );
  if (element == NULL) {
    return ErrNew(ERR_QUEUE, ERR_QUEUE_UNSPECIFIED, NULL,
                  "queue_waitFor: doPop failed");
  }

  assert( result != NULL );

  *result = element->userData;

  free( element );

  return NULL;
}

/* Should be called while the queue is locked */
static void validate( Queue queue )
{
  if( queue->oldest == NULL )
    assert( queue->newest == NULL );
  else
  {
    QueueEntry element;

    for( element = queue->oldest; element->newer != NULL; element = element->newer )
      ;
    
    assert( queue->newest == element );
    assert( element->newer == NULL );
  }
}
