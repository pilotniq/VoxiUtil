/*
   queue.h

  Note: This implementation assumes that all reads from the queue are done by 
        one thread.

  (C) Copyright 2004 Icepeak AB
  (C) Copyright 2004 Voxi AB

  Erland Lewin
 */

#ifndef VOXI_UTIL_QUEUE_H
#define VOXI_UTIL_QUEUE_H

#include <pthread.h> /* the struct timespec is defined here methinks. */

#include <voxi/util/err.h>

typedef struct sQueue *Queue;

enum { ERR_QUEUE_UNSPECIFIED, ERR_QUEUE_TIMEDOUT, ERR_QUEUE_NOT_EMPTY };

EXTERN_UTIL Error queue_create( Queue *result );

/* 
   A queue must be empty before it is destroyed 
*/
EXTERN_UTIL Error queue_destroy( Queue queue );

EXTERN_UTIL Error queue_push( Queue queue, void *element );

/* Returns TRUE if the queue contains elements */
EXTERN_UTIL Boolean queue_poll( const Queue queue );
EXTERN_UTIL Error queue_pop( Queue, void **result );
/*
  if timeoutTime is NULL, then no timeout is used (the function will wait 
  forever.
  */
EXTERN_UTIL Error queue_waitFor( Queue, const struct timespec *timeoutTime, void **result );

#endif
