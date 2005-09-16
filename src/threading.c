/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
 * Utility functions for threading 
 */

#include "config.h"

#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <voxi/alwaysInclude.h>

#include <voxi/util/threading.h>

CVSID("$Id$");

/*
  macros
*/

/*
 * Exported data
 */

/*
  Thread attributes for threads created in the detached state
  (Who cannot be joined, and whose data is automatically freed when the
  thread terminates)
 */
pthread_attr_t detachedThreadAttr, realtimeDetachedThreadAttr, 
  joinableRealtimeThreadAttr, detachedLowPrioThreadAttr;

/*
 * static data 
 */
static int inits = 0;

/*
  The threading modules can be init'ed by several modules simultaneously 
  without any problems - it keeps track of how many inits and shutdowns have
  been made.
*/
Error threading_init()
{
  Error error = NULL;
  
  /* The inits variable should really be protected by a semaphore */
  if( inits == 0 )
  {
    int err;
    struct sched_param sched_param;
    
    /* init the detachedThreadAttr static variable */
    err = pthread_attr_init( &detachedThreadAttr );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_init failed." );
      goto ERR_RETURN;
    }

    err = pthread_attr_setdetachstate( &detachedThreadAttr, 
                                       PTHREAD_CREATE_DETACHED );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setdetachstate failed." );
      goto ERR_RETURN;
    }
    
    /* init the realtime detachedThreadAttr static variable */
    err = pthread_attr_init( &realtimeDetachedThreadAttr );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_init failed." );
      goto ERR_RETURN;
    }

    err = pthread_attr_setdetachstate( &realtimeDetachedThreadAttr, 
                                       PTHREAD_CREATE_DETACHED );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setdetachstate failed." );
      goto ERR_RETURN;
    }

#ifndef PTHREADS_WIN32    
    err = pthread_attr_setschedpolicy( &realtimeDetachedThreadAttr,
                                       SCHED_FIFO );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setdetachstate failed." );
      goto ERR_RETURN;
    }
    sched_param.sched_priority = 1;
#else  /* PTHREADS_WIN32 */
    sched_param.sched_priority = 2;   /* Win32: THREAD_PRIORITY_HIGHEST */
#endif /* PTHREADS_WIN32 */
    
    err = pthread_attr_setschedparam( &realtimeDetachedThreadAttr, 
                                      &sched_param );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setschedparam failed." );
      goto ERR_RETURN;
    }
    
    /* init the joinableRealtimeThreadAttr static variable */
    err = pthread_attr_init( &joinableRealtimeThreadAttr );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_init failed." );
      goto ERR_RETURN;
    }
    
    err = pthread_attr_setdetachstate( &joinableRealtimeThreadAttr, 
                                       PTHREAD_CREATE_JOINABLE );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setdetachstate failed." );
      goto ERR_RETURN;
    }
    
#ifndef PTHREADS_WIN32    
    err = pthread_attr_setschedpolicy( &joinableRealtimeThreadAttr,
                                       SCHED_FIFO );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setschedpolicy failed." );
      goto ERR_RETURN;
    }
    sched_param.sched_priority = 1;
#else  /* PTHREADS_WIN32 */
    sched_param.sched_priority = 2; /* Win32: THREAD_PRIORITY_HIGHEST */
#endif /* PTHREADS_WIN32 */
    
    err = pthread_attr_setschedparam( &joinableRealtimeThreadAttr, 
                                      &sched_param );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setschedparam failed." );
      goto ERR_RETURN;
    }

    /* init the detachedLowPrioThreadAttr static variable */
    err = pthread_attr_init( &detachedLowPrioThreadAttr );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_init failed." );
      goto ERR_RETURN;
    }

    err = pthread_attr_setdetachstate( &detachedLowPrioThreadAttr, 
                                       PTHREAD_CREATE_DETACHED );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setdetachstate failed." );
      goto ERR_RETURN;
    }

#ifndef PTHREADS_WIN32
    err = pthread_attr_setschedpolicy( &detachedLowPrioThreadAttr,
                                       SCHED_OTHER );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setschedpolicy failed." );
      goto ERR_RETURN;
    }
#else  /* PTHREADS_WIN32 */
    sched_param.sched_priority = -2; /* THREAD_PRIORITY_LOWEST */
    err = pthread_attr_setschedparam( &detachedLowPrioThreadAttr, 
                                      &sched_param );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_setschedparam failed." );
      goto ERR_RETURN;
    }
#endif /* PTHREADS_WIN32 */
  }

  inits++;
  
 ERR_RETURN:
  return error;
}

Error threading_shutdown()
{
  Error error = NULL;
  assert( inits > 0 );

  inits--;

  if( inits == 0 )
  {
    int err;

    /* Destroy the detachedThreadAttr */
    err = pthread_attr_destroy( &detachedThreadAttr );
    if (err != 0) {
      error = ErrNew(ERR_THREADING, 0, NULL,
                     "pthread_attr_destroy failed." );
    }
  }

  return error;
}

Error threading_mutex_init( VoxiMutex mutex )
{
  Error error = NULL;
  int err;
  
  err = sem_init( &(mutex->semaphore), 0, 1 );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_init failed." );
    goto ERR_RETURN;
  }
  
  err = pthread_mutex_init( &(mutex->mutex), NULL );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "pthread_mutex_init failed." );
    goto ERR_RETURN;
  }

#ifndef NDEBUG
  /* I dont set it to 0 since i want to know that I was the one that set 
     these values (when i'm debugging) */
  mutex->pid = 10;
#else
  mutex->pid = 0;
#endif

  mutex->lastLockFrom = NULL;
  mutex->count = 0;
  mutex->debug = FALSE;

 ERR_RETURN:
  return error;
}

void threading_mutex_lock( VoxiMutex mutex )
{
  threading_mutex_lock_debug( mutex, NULL );
}

/* FIXME: Change the API of this function to return Error info, since there
   are several points in it where runtime errors may occur (now marked as
   commented-out assertions on return values from various functions.
   NB: This should of course not affect assertions that document the
   intended algorithmic flow of the implementation itself. -mst */
const char *threading_mutex_lock_debug( VoxiMutex mutex, const char *where )
{
  int err, tempInt;
  const char *oldWhere;

  DEBUG( "threading_mutex_lock: sem_wait" );
  
  if( mutex->debug )
    fprintf( stderr, "threading_mutex_lock_debug( %p, %s ), pid %d: "
             "waiting.\n",
             mutex, (where == NULL) ? "NULL" : where, getpid() );
  
  err = sem_wait( &(mutex->semaphore) );
  /* assert( err == 0 ); */

  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  /* assert( err == 0 ); */
  assert( tempInt == 0 );

  if( mutex->debug )
    fprintf( stderr, "threading_mutex_lock_debug( %p, %s ), pid %d: "
             "got semaphore, count=%d.\n",
             mutex, (where == NULL) ? "NULL" : where, getpid(), mutex->count );
  {
    pthread_t mythread = pthread_self();
    
    if( (mutex->count > 0) && pthread_equal( mutex->thread, mythread) )
    {
      if( mutex->debug )
        fprintf( stderr, "threading_mutex_lock_debug( %p, %s ), pid %d: got "
                 "recursive lock.\n", mutex, (where == NULL) ? "NULL" : where,
                 getpid() );
  
      mutex->count++;
    }
    else
    {
      DEBUG( "threading_mutex_lock: wait sem_post" );

      err = sem_post( &(mutex->semaphore) );
      /* assert( err == 0 ); */
      
#ifndef PTHREADS_WIN32      
      if( mutex->debug )
        fprintf( stderr, "threading_mutex_lock_debug( %p, %s ), pid %d: "
                 "before second wait, status=?\n", mutex, /* status was %ld */
                 (where == NULL) ? "NULL" : where, getpid()
                 /* mutex->mutex.__m_lock.__status */);
#endif /* PTHREADS_WIN32 */
      
      err = pthread_mutex_lock( &(mutex->mutex) );
      /* assert( err == 0 ); */
      /* assert( mutex->mutex.__m_lock.__status > 0 ); */
      
#ifndef PTHREADS_WIN32      
      if( mutex->debug )
        fprintf( stderr, "threading_mutex_lock_debug( %p, %s ), pid %d, "
                 "thread %ld: after second wait, locked mutex_t %p, "
                 "status = ?, " /* status was %ld */
                 "waiting for semaphore.\n", 
                 mutex, (where == NULL) ? "NULL" : where,
                 getpid(), (long int) pthread_self(), &(mutex->mutex)
                 /* mutex->mutex.__m_lock.__status */);
#endif /* PTHREADS_WIN32 */

      DEBUG( "threading_mutex_lock: wait sem_wait" );

      err = sem_wait( &(mutex->semaphore) );
      /* assert( err == 0 ); */

      err = sem_getvalue( &(mutex->semaphore), &tempInt );
      /* assert( err == 0 ); */
      assert( tempInt == 0 );

      if( mutex->debug )
        fprintf( stderr, "threading_mutex_lock_debug( %p, %s ), pid %d: "
                 "second gotit.\n", mutex, (where == NULL) ? "NULL" : where,
                 getpid() );

      if( mutex->count != 0 )
        fprintf( stderr, "ERROR: threading_mutex_lock_debug( %p, %s ), "
                 "pid %d: mutex count was %d!\n", mutex, 
                 (where == NULL) ? "NULL" : where,
                 getpid(), mutex->count );

      assert( mutex->count == 0 );
      mutex->count = 1;
      mutex->thread = pthread_self();
      mutex->pid = getpid();
    }  
    oldWhere = mutex->lastLockFrom;
    mutex->lastLockFrom = where;
  }
  DEBUG( "threading_mutex_lock: sem_post" );
  
  err = sem_post( &(mutex->semaphore) );
  /* assert( err == 0 ); */
  
  DEBUG("threading_mutex_lock_debug( %p, %s ) = %s\n", mutex,
        (where == NULL) ? "NULL" : where,
        (oldWhere == NULL) ? "NULL" : oldWhere);

  return oldWhere;
}

void threading_mutex_unlock( VoxiMutex mutex )
{
  threading_mutex_unlock_debug( mutex, NULL );
}

/* FIXME: Change the API of this function to return Error info, since there
   are several points in it where runtime errors may occur (now marked as
   commented-out assertions on runtime return values from various functions).
   NB: This should of course not affect assertions that document the
   intended algorithmic flow of the implementation itself. -mst */
void threading_mutex_unlock_debug( VoxiMutex mutex, const char *oldWhere )
{
  int err;
  int tempInt = 0;
  
  DEBUG( "threading_mutex_unlock: sem_wait" );
  
  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  /* assert( err == 0 ); */
  
  if( mutex->debug )
    fprintf( stderr, "threading_mutex_unlock_debug( %p, %s ), pid %d: "
             "waiting for access, sem %p value=%d.\n", mutex, 
             (oldWhere == NULL) ? "NULL" : oldWhere, getpid(), 
             &(mutex->semaphore), tempInt );
  
  /* wait for the semaphore to have a non-zero value, then decrease it */
  err = sem_wait( &(mutex->semaphore) );
  /* assert( err == 0 ); */
  
  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  /* assert( err == 0 ); */
  assert( tempInt == 0 );
  
  assert( pthread_equal( mutex->thread, pthread_self() ) );
  assert( mutex->count > 0 );
  
  mutex->lastLockFrom = oldWhere;
  mutex->count--;
  
  if( mutex->debug )
    fprintf( stderr, "threading_mutex_unlock_debug( %p, %s ), pid %d: "
             "new count=%d\n", mutex, (oldWhere == NULL) ? "NULL" : oldWhere,
             getpid(), mutex->count );
  
  if( mutex->count == 0 )
  {
    /* I dont set it to 0 since i want to know that */
    /* I was the one that set these values (when i'm debugging) */
    mutex->pid = 11;     

    err = pthread_mutex_unlock( &(mutex->mutex) );
    /* assert( err == 0 ); */
    
    if( mutex->debug )
      fprintf( stderr, "threading_mutex_unlock_debug( %p, %s ), pid %d: "
               "pthread_mutex_unlock( %p )\n", mutex, 
               (oldWhere == NULL) ? "NULL" : oldWhere,
               getpid(), &(mutex->mutex) );

    assert( oldWhere == NULL );
  }
  
  DEBUG( "threading_mutex_unlock: sem_post" );
  
  err = sem_post( &(mutex->semaphore) );
  /* assert( err == 0 ); */

#ifndef NDEBUG
  if( mutex->debug || debug )
#else
  if( mutex->debug )
#endif /* NDEBUG */
    fprintf( stderr, "threading_mutex_unlock_debug( %p, %s ), pid %d: "
             "leaving.\n", mutex, (oldWhere == NULL) ? "NULL" : oldWhere,
             getpid() );
}

Error threading_cond_wait( pthread_cond_t *condition, VoxiMutex mutex )
{
  Error error = NULL;
  int oldCount, tempInt;
  int err;
  const char *oldLastLockFrom;
  
  err = sem_wait( &(mutex->semaphore) );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_wait failed." );
    goto ERR_RETURN;
  }
  
  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_getvalue failed." );
    goto ERR_RETURN;
  }
  assert( tempInt == 0 );

  /* Is the below really a requirement?
     Is it not allowed to pass on a locked mutex to a new
     thread to "finish the job"? -mst */
  /* assert( mutex->thread == pthread_self() ); */
  
  assert( mutex->count > 0 );
  
  /* release the VoxiMutex, saving the old state */

  /* I dont set it to 0 since i want to know that I was the one that set 
     these values (when i'm debugging) */
  mutex->pid = 12;
  
  oldLastLockFrom = mutex->lastLockFrom;
  mutex->lastLockFrom = NULL;
  
  oldCount = mutex->count;
  mutex->count = 0;
  
  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: oldCount=%d\n",
             condition, mutex, getpid(), oldCount );
  
  /* Hmm, is it dangerous to sem-post here, before the wait */
  err = sem_post( &(mutex->semaphore) );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_post failed." );
    goto ERR_RETURN;
  }
  
  err = pthread_cond_wait( condition, &(mutex->mutex) );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "pthread_cond_wait failed." );
    goto ERR_RETURN;
  }

  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: after "
             "pthread_cond_wait\n",
             condition, mutex, getpid() );
  
#if 0
  /* release the mutex, and get the semaphore, because that's the order it 
     must be done in to prevent deadlock. */
  
  err = pthread_mutex_unlock( &(mutex->mutex) );
  
  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: reaquiring\n",
             condition, mutex, getpid() );
#endif
  err = sem_wait( &(mutex->semaphore) );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_wait failed." );
    goto ERR_RETURN;
  }
  
  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_getvalue failed." );
    goto ERR_RETURN;
  }
  assert( tempInt == 0 );

  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: got semaphore\n",
             condition, mutex, getpid() );

  assert( mutex->count == 0 );
  mutex->count = oldCount;
  mutex->lastLockFrom = oldLastLockFrom;
  mutex->thread = pthread_self();
  mutex->pid = getpid();

  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: leaving "
             "(count=%d)\n", condition, mutex, getpid(), mutex->count );
  
  err = sem_post( &(mutex->semaphore) );
  if (err != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "sem_post failed." );
    goto ERR_RETURN;
  }

 ERR_RETURN:
  return error;
}

void threading_mutex_destroy( VoxiMutex mutex )
{
  int err;
  
  assert( mutex->count == 0 );

  if( mutex->debug )
    fprintf( stderr, "threading_mutex_destroy( %p ), pid %d.\n",
             mutex, getpid() );
  
  err = sem_destroy( &(mutex->semaphore) );
  if( err != 0 )
    perror( "threading_mutex_destroy - sem_destroy" );
  
  err = pthread_mutex_destroy( &(mutex->mutex) );
  if( err != 0 )
    perror( "threading_mutex_destroy - pthread_mutex_destroy" );
}

Boolean threading_cond_timedwait( pthread_cond_t *condition, VoxiMutex mutex, 
                                  unsigned long usec )
{
  struct timespec wakeuptime;
  struct timeval now;
 /* Fix timespec struct */
  gettimeofday(&now, NULL);
  wakeuptime.tv_sec = now.tv_sec + (usec / 1000000);
  wakeuptime.tv_nsec = (now.tv_usec + (usec % 1000000)) * 1000;

  return threading_cond_absolute_timedwait(condition, mutex, &wakeuptime);
}

/* FIXME: Change the API of this function to return Error info, since there
   are several points in it where runtime errors may occur (now marked as
   commented-out assertions on runtime return values from various functions).
   NB: This should of course not affect assertions that document the
   intended algorithmic flow of the implementation itself. -mst */
Boolean threading_cond_absolute_timedwait( pthread_cond_t *condition, 
                                           VoxiMutex mutex, 
                                           struct timespec *wakeuptime ) 
{
  int oldCount, tempInt;
  int err;
  const char *oldLastLockFrom;

  Boolean timedout;
  
  err = sem_wait( &(mutex->semaphore) );
  assert( err == 0 );
  
  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  /* assert( err == 0 ); */
  assert( tempInt == 0 );

  assert( pthread_equal( mutex->thread, pthread_self() ) );
  assert( mutex->count > 0 );
  
  /* release the VoxiMutex, saving the old state */

  /* I dont set it to 0 since i want to know that I was the one that 
     set these values (when i'm debugging) */
  mutex->pid = 13;
  
  oldLastLockFrom = mutex->lastLockFrom;
  mutex->lastLockFrom = NULL;
  
  oldCount = mutex->count;
  mutex->count = 0;
  
  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: oldCount=%d\n",
             condition, mutex, getpid(), oldCount );
  
  /* Hmm, is it dangerous to sem-post here, before the wait */
  err = sem_post( &(mutex->semaphore) );
  /* assert( err == 0 ); */

  /* Do the wait */
  err = pthread_cond_timedwait( condition, &(mutex->mutex), wakeuptime );
  /* assert( err == 0 ); */
  timedout = (err == ETIMEDOUT);

  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: after "
             "pthread_cond_wait\n",
             condition, mutex, getpid() );
  
#if 0
  /* release the mutex, and get the semaphore, because that's the order it 
     must be done in to prevent deadlock. */
  
  err = pthread_mutex_unlock( &(mutex->mutex) );
  
  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: reaquiring\n",
             condition, mutex, getpid() );
#endif
  err = sem_wait( &(mutex->semaphore) );
  /* assert( err == 0 ); */

    
  err = sem_getvalue( &(mutex->semaphore), &tempInt );
  /* assert( err == 0 ); */
  assert( tempInt == 0 );

  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: got semaphore\n",
             condition, mutex, getpid() );

  assert( mutex->count == 0 );
  mutex->count = oldCount;
  mutex->lastLockFrom = oldLastLockFrom;
  mutex->thread = pthread_self();
  mutex->pid = getpid();

  if( mutex->debug )
    fprintf( stderr, "threading_cond_wait( %p, %p ), pid %d: leaving "
             "(count=%d)\n", condition, mutex, getpid(), mutex->count );
  
  err = sem_post( &(mutex->semaphore) );
  /* assert( err == 0 ); */

  return timedout;
}

void threading_mutex_setDebug( VoxiMutex mutex, Boolean debug )
{
  if( debug || (mutex->debug) )
    fprintf( stderr, "threading_mutex_setDebug( %p, %s ), pid %d, "
             "thread %ld.\n", mutex, debug ? "TRUE" : "FALSE", getpid(), 
             pthread_self() );

  mutex->debug = debug;
}

#ifdef PROFILING
/* 
 * pthread_create wrapper for gprof compatibility
 *
 * from http://sam.zoy.org/doc/programming/gprof.html
 *
 * needed headers: <pthread.h>
 *                 <sys/time.h>
 */

typedef struct wrapper_s
{
  void * (*start_routine)(void *);
  void * arg;

  pthread_mutex_t lock;
  pthread_cond_t  wait;

  struct itimerval itimer;

} wrapper_t;


/* The wrapper function in charge for setting the itimer value */
static void * wrapper_routine(void * data)
{
  /* Put user data in thread-local variables */
  void * (*start_routine)(void *) = ((wrapper_t*)data)->start_routine;
  void * arg = ((wrapper_t*)data)->arg;

  /* Set the profile timer value */
  setitimer(ITIMER_PROF, &((wrapper_t*)data)->itimer, NULL);

  /* Tell the calling thread that we don't need its data anymore */
  pthread_mutex_lock(&((wrapper_t*)data)->lock);
  pthread_cond_signal(&((wrapper_t*)data)->wait);
  pthread_mutex_unlock(&((wrapper_t*)data)->lock);

  /* Call the real function */
  return start_routine(arg);
}

#endif


/* Same prototype as pthread_create; use some #define magic to
 * transparently replace it in other files */
int threading_pthread_create(pthread_t * thread, pthread_attr_t * attr,
                         void * (*start_routine)(void *), void * arg)
{
  /*
  static unsigned long tot = 0;
  static int numCalls = 0;
  unsigned long start, stop;
  */
  int i_return;
  
#ifdef PROFILING
  wrapper_t wrapper_data;

  /* Initialize the wrapper structure */
  wrapper_data.start_routine = start_routine;
  wrapper_data.arg = arg;
  getitimer(ITIMER_PROF, &wrapper_data.itimer);
  pthread_cond_init(&wrapper_data.wait, NULL);
  pthread_mutex_init(&wrapper_data.lock, NULL);
  pthread_mutex_lock(&wrapper_data.lock);

  /* Alter user-passed data so that we call the wrapper instead
   * of the real function */
  arg = &wrapper_data;
  start_routine = wrapper_routine;
#endif

  /* The real pthread_create call */
  /* start = millisec(); */
  i_return = pthread_create(thread, attr, start_routine,
                            arg);
  /*stop = millisec();
  tot += stop - start;
  numCalls++;
  printf("Call to pthread_create took %lu ms, numCalls=%d avg time=%f ms\n", stop - start, numCalls, (float)((float)tot)/numCalls);
  */
#ifdef PROFILING
  /* If the thread was successfully spawned, wait for the data
   * to be released */
  if(i_return == 0)
    {
      pthread_cond_wait(&wrapper_data.wait, &wrapper_data.lock);
    }

  pthread_mutex_unlock(&wrapper_data.lock);
  pthread_mutex_destroy(&wrapper_data.lock);
  pthread_cond_destroy(&wrapper_data.wait);
#endif

  return i_return;
}
