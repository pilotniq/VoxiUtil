/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
 * Utility functions for threading 
 */

#ifndef THREADING_H
#define THREADING_H

#include <voxi/util/config.h>

#ifdef HAVE_UNISTD_H
/* The POSIX-feature definitions are here on unix-like systems. */
#include <unistd.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

/* libcCompat defines EXTERN macro */
#include <voxi/util/libcCompat.h>

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif /* WIN32 */

#include <voxi/types.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif


/* This type is defined as the type passed as the thirt parameter to 
   pthread_crete. */
typedef void * (*ThreadFunc)(void *);

typedef struct
{
  pthread_mutex_t mutex;
  pthread_t thread;
  pid_t pid;  /* the number used by gdb to identify threads */
  /* 
     this semaphore has value 1 when no-one is accessing the mutex data,
     0 otherwise.
     
     rules:
     
     one may only try to lock the lock while holding the semaphore if the 
     count is 0.
     
  */
  sem_t semaphore;
  int count;
  const char *lastLockFrom;
  Boolean debug;
} sVoxiMutex, *VoxiMutex;

/* 
   The detachedThreadAttr variable must not be accessed until the 
   threading_init call has been made.
*/

EXTERN_UTIL pthread_attr_t detachedThreadAttr, realtimeDetachedThreadAttr,
  joinableRealtimeThreadAttr, detachedLowPrioThreadAttr;

/*
  The threading modules can be init'ed by several modules simultaneously 
  without any problems - it keeps track of how many inits and shutdowns have
  been made
*/
EXTERN_UTIL void threading_init();
EXTERN_UTIL void threading_shutdown();

EXTERN_UTIL void threading_mutex_init( VoxiMutex mutex );
EXTERN_UTIL void threading_mutex_lock( VoxiMutex mutex );
EXTERN_UTIL void threading_mutex_setDebug( VoxiMutex mutex, Boolean debug );
/*
  Debugging version of mutex_lock.
  
  Pass a string describing why/where the lock is being locked. 
  
  The function will return the description of the previous locker of the lock.
*/
EXTERN_UTIL const char *threading_mutex_lock_debug( VoxiMutex mutex, const char *where );
/*
  Debugging version of mutex_unlock.
  
  Pass the string returned by threading_mutex_lock_debug as the oldWhere 
  parameter.
*/
EXTERN_UTIL void threading_mutex_unlock_debug( VoxiMutex mutex, const char *oldWhere );
EXTERN_UTIL void threading_mutex_unlock( VoxiMutex mutex );
EXTERN_UTIL void threading_mutex_destroy( VoxiMutex mutex );

EXTERN_UTIL void threading_cond_wait( pthread_cond_t *condition, VoxiMutex mutex );
EXTERN_UTIL Boolean threading_cond_timedwait( pthread_cond_t *condition, 
                                              VoxiMutex mutex, 
                                              unsigned long usec );

/* use this instead of pthread_create! */
EXTERN_UTIL int threading_pthread_create( pthread_t * thread, 
                                          pthread_attr_t * attr,
                                          ThreadFunc start_routine, 
                                          void * arg);

#ifdef __cplusplus
}
#endif

#endif
