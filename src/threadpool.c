/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
 * A thread pool.
 */

#include "config.h"

#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif

#define DEBUG_MESSAGES 0

#include <voxi/alwaysInclude.h>
#include <voxi/util/threadpool.h>

CVSID("$Id$");


/* Static function prototypes */

/*
 * The main loop waiting for the thread to become in use.
 */
static void * threadPoolThread_mainLoop(ThreadPoolThread self);

/* Check if thread attr is detached */
static Boolean threadPool_checkDetached(pthread_attr_t attr);

/* Creates a new theadPoolThread, does not add it to any list */
static Error threadPoolThread_create(ThreadPool threadPool, ThreadPoolThread *res);

/* Destroys and frees a thread pool thread, assuming  the start cond already signaled */
static Error threadPoolThread_destroy(ThreadPoolThread thread);

/* Adds a thread to the available threads' list.
 * NOTE: YOU ARE RESPONSIBLE FOR LOCKING YOURSELF */
static void threadPool_makeAvailable(ThreadPoolThread thread);

/* Internal data */

/* Possibilities for the joined state of the non-detached ThreadPoolThreds */
typedef enum {JOIN_STATE_RUNNING_UNJOINED,
              JOIN_STATE_RUNNING_JOINED,
              JOIN_STATE_FINISHED_UNJOINED,
              JOIN_STATE_FINISHED_JOINED,
              JOIN_STATE_COMPLETE} JoinState;

typedef struct sThreadPool {

  /* The pthread attributes for this thread as a bitmap */
  pthread_attr_t attribute;

  sVoxiMutex threadListMutex;
  
  /* A linked list with currently available threads */
  ThreadPoolThread firstAvailableThread;
  
  /* A linked list with threads in use */
  ThreadPoolThread firstThreadInUse;
  
  /* Set to true when destroy is called */
  Boolean isShuttingDown;

  Boolean isDetached;
  
} sThreadPool;

typedef struct sThreadPoolThread {

  /* The real thread */
  pthread_t thread;

  /* Next element in the linked list, NULL if this is last */
  ThreadPoolThread next;
  ThreadPoolThread prev;

  /* The thread pool in which this thread is stored */
  ThreadPool myPool;

  /* The client's function with data */
  ThreadFunc threadFunc;
  void *threadFuncArgs;
  void *threadFuncResult;

  /* Conditions for the startup of the thread */
  pthread_cond_t startCondition;
  sVoxiMutex startConditionMutex;

  /* When thread is joined upon */
  pthread_cond_t joinStateCondition;
  sVoxiMutex joinStateMutex;

  /* For detached ThreadPoolThreads only */
  JoinState state;  
} sThreadPoolThread;


Error threadPool_create(int initialSize, pthread_attr_t attr, ThreadPool *res) {
  Error error = NULL;
  int i;
  ThreadPool pool;
  DEBUG("threadPool create enter\n");
  
  assert(initialSize >= 0);

  /* Allocate the thread pool */
  pool = (ThreadPool) malloc(sizeof(sThreadPool));
  assert(pool != NULL);

  /* The attribute should not be joinable since we join the
   * actal threads manually */
  pool->isDetached = threadPool_checkDetached(attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  DEBUG("threadPool create isDetached: %d\n", pool->isDetached);
  
  /* Set the thread pool attributes */
  pool->attribute            = attr;
  pool->isShuttingDown       = FALSE;
  pool->firstThreadInUse     = NULL;
  pool->firstAvailableThread = NULL;

  /* Init the mutex */
  threading_mutex_init(&(pool->threadListMutex));
  
  /* Create the threads */
  for (i = 0; i < initialSize; i++) {
    ThreadPoolThread resource;

    error = threadPoolThread_create(pool, &resource);
    if (error != NULL)
      goto ERR1;
    
    threadPool_makeAvailable(resource);
  }

  *res = pool;

 ERR1:
  DEBUG("threadPool create leaving\n");
  return error;
}


Error threadPool_destroy(ThreadPool threadPool) {
  Error error = NULL;
  ThreadPoolThread theThread = NULL;
  int err = 0;

  DEBUG("ThreadPool_destroy enter\n");
  assert(threadPool != NULL);

  threading_mutex_lock(&(threadPool->threadListMutex));
  
  threadPool->isShuttingDown = TRUE;
  
  /* Destroy all available threads */
  while ((theThread = threadPool->firstAvailableThread) != NULL) {
    Error tempErr;
    ThreadPoolThread next;

    DEBUG("ThreadPool_destroy waking available thread\n");
    err = pthread_cond_signal(&(theThread->startCondition));
    assert(err == 0);

    next    = theThread->next;
    tempErr = threadPoolThread_destroy(theThread); /* Also frees it */
    threadPool->firstAvailableThread = next;
    
    if (tempErr != NULL)
      error = tempErr;
  }
  
   /* Destroy all threads in use */
  while ((theThread = threadPool->firstThreadInUse) != NULL) {
    Error tempErr;
    ThreadPoolThread next;

    DEBUG("ThreadPool_destroy destroying in use thread\n");
    next    = theThread->next;
    tempErr = threadPoolThread_destroy(theThread);
    threadPool->firstThreadInUse = next;
    
    if (tempErr != NULL)
      error = tempErr;
  }
  
  threading_mutex_unlock(&(threadPool->threadListMutex));
  
  /* Now clean up the thread pool's attributes */
  err = pthread_attr_destroy(&(threadPool->attribute));
  assert(err == 0);

  threading_mutex_destroy(&(threadPool->threadListMutex));

  free(threadPool);

  DEBUG("ThreadPool_destroy leaving\n");
  return error;
}

Error threadPool_runThread(ThreadPool threadPool, ThreadFunc func, void *args,
                           ThreadPoolThread *newThread) {
  Error error = NULL;
  ThreadPoolThread theThread;
  ThreadPoolThread tempElement;

  DEBUG("threadPool_runThread, enter\n");
  
  assert(func != NULL);
  assert(threadPool != NULL);

  threading_mutex_lock(&(threadPool->threadListMutex));
  
  /* Check if shutting down */
  if (threadPool->isShuttingDown) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "Cannot run thread, since pool is shutting down.");
    
    threading_mutex_unlock(&(threadPool->threadListMutex));
    goto ERR1;
  }
  
  /* Grab first in available list */

  if (threadPool->firstAvailableThread == NULL) {
    DEBUG("threadPool_runThread, No threads available, creating new\n");
    error = threadPoolThread_create(threadPool, &theThread);
  }
  else {
    DEBUG("threadPool_runThread, found thread available\n");
    theThread = threadPool->firstAvailableThread;
    threadPool->firstAvailableThread = theThread->next;
    if (threadPool->firstAvailableThread != NULL)
      threadPool->firstAvailableThread->prev = NULL;
  }

  if (error != NULL) {
    threading_mutex_unlock(&(threadPool->threadListMutex));
    goto ERR1;
  }
  
  /* Add to in use list */
  tempElement = threadPool->firstThreadInUse;
  if (tempElement != NULL)
    tempElement->prev = theThread;
  theThread->next = tempElement;
  theThread->prev = NULL;
  threadPool->firstThreadInUse = theThread;
  
  threading_mutex_unlock(&(threadPool->threadListMutex));
  
  /* Set the function to be called and signal to wake thread */
  threading_mutex_lock(&(theThread->startConditionMutex));
  theThread->threadFunc = func;
  theThread->threadFuncArgs = args;
  DEBUG("threadPool_runThread before signal startCondition\n");
  pthread_cond_signal(&(theThread->startCondition));
  threading_mutex_unlock(&(theThread->startConditionMutex));

  *newThread = theThread;
  
 ERR1:
  DEBUG("threadPool_runThread, leaving\n");
  return error;
}


Error threadPoolThread_join(ThreadPoolThread thread, void **threadFuncResult) {
  Error error = NULL;
  DEBUG("threadPool join enter.\n");
  
  if (thread->myPool->isDetached) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "You cannot call join on detached thread pools");
    goto ERR1;
  }
  
  /* Check if thread is running, if not wait until finished */
  threading_mutex_lock(&(thread->joinStateMutex));
  DEBUG("threadPool join state=%d\n", thread->state);
  
  if ((thread->state == JOIN_STATE_RUNNING_JOINED) ||
      (thread->state == JOIN_STATE_FINISHED_JOINED)) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "Attempt to call join on already joined thread");
    threading_mutex_unlock(&(thread->joinStateMutex));
    goto ERR1;
  }

  if (thread->state == JOIN_STATE_RUNNING_UNJOINED) {
    thread->state = JOIN_STATE_RUNNING_JOINED;
    DEBUG("threadPool join state befor wait for finished\n");
    threading_cond_wait(&(thread->joinStateCondition),
                        &(thread->joinStateMutex));
    assert(thread->state == JOIN_STATE_FINISHED_JOINED);
  }
    
  /* Now that thread is finished, get the return value and signal complete */
  DEBUG("threadPool join before signal complete\n");
  *threadFuncResult = thread->threadFuncResult;
  thread->state     = JOIN_STATE_COMPLETE;
  pthread_cond_signal(&(thread->joinStateCondition));
  threading_mutex_unlock(&(thread->joinStateMutex));
  
 ERR1:
  DEBUG("threadPool join leaving\n");
  return error;
}

/*
 * Loops until the pool is shut down.
 * The start condition could be signaled, either by the thread
 * coming in to use or the pool being shut down.
 */
static void * threadPoolThread_mainLoop(ThreadPoolThread self) {
  int err = 0;
  DEBUG("threadPool mainLoop, enter\n");
  
  do {
    /* Wait until either shutting down or thread in use */
    threading_mutex_lock(&(self->startConditionMutex));
    
    if (self->threadFunc == NULL) {
      DEBUG("threadPool mainLoop, before startCondition wait\n");
      threading_cond_wait(&(self->startCondition),
                          &(self->startConditionMutex));
    }

    DEBUG("threadPool mainLoop, thread woke up\n");
    threading_mutex_unlock(&(self->startConditionMutex));
    
    if (self->myPool->isShuttingDown)
      break;

    /* Check that state is valid */
    assert((self->state == JOIN_STATE_RUNNING_UNJOINED) ||
           (self->state == JOIN_STATE_RUNNING_JOINED));
    
    /* Call the thread function */
    DEBUG("threadPool mainLoop calling thread function.\n");
    self->threadFuncResult = self->threadFunc(self->threadFuncArgs);

    /* If not detached we should join the thread */
    if (!(self->myPool->isDetached)) {
      DEBUG("threadPool mainLoop enter is detached block\n");
      threading_mutex_lock(&(self->joinStateMutex));

      /* Change the join state to FINISHED */      
      if (self->state == JOIN_STATE_RUNNING_JOINED)
        self->state = JOIN_STATE_FINISHED_JOINED;
      else if (self->state == JOIN_STATE_RUNNING_UNJOINED)
        self->state = JOIN_STATE_FINISHED_UNJOINED;

      DEBUG("threadPool mainLoop state: %d, before signal joinstate\n", self->state);
      err = pthread_cond_signal(&(self->joinStateCondition));
      assert(err == 0);
      
      /* Now wait until the join is complete, note that mutex already locked above */
      if (self->state != JOIN_STATE_COMPLETE) {
        DEBUG("threadPool mainLoop before wait for completion\n");
        threading_cond_wait(&(self->joinStateCondition),
                            &(self->joinStateMutex));
      }

      /* Finished, do some cleanup */
      self->state = JOIN_STATE_RUNNING_UNJOINED;
      threading_mutex_unlock(&(self->joinStateMutex));
    }

    /* Clean up, remove from inUse and restore to available */
    self->threadFunc = NULL;

    /* If shutting down, no need for this since thread will be destroyed */
    threading_mutex_lock(&(self->myPool->threadListMutex));
    if (!(self->myPool->isShuttingDown)) {
      ThreadPool thePool = self->myPool;
      
      /* Remove from in use list */
      assert(thePool->firstThreadInUse != NULL);

      if (thePool->firstThreadInUse == self)
        thePool->firstThreadInUse = self->next;
      if (self->prev != NULL)
        self->prev->next = self->next;
      if (self->next != NULL)
        self->next->prev = self->prev;

      /* Insert into available list */
      threadPool_makeAvailable(self);
    }
    threading_mutex_unlock(&(self->myPool->threadListMutex));
    
  } while (!(self->myPool->isShuttingDown));
  DEBUG("threadPool mainLoop leaving - shutting down\n");

  return NULL;
}

/* Checks if an attr is detached */
static Boolean threadPool_checkDetached(pthread_attr_t attr) {
  int val;
  int err = 0;
  err = pthread_attr_getdetachstate(&attr, &val);
  assert(err == 0);
  return (val == PTHREAD_CREATE_DETACHED); 
}

static Error threadPoolThread_create(ThreadPool pool, ThreadPoolThread *res) {
  Error error = NULL;
  int err1, err2, err3 = 0;
  
  ThreadPoolThread resource =
    (ThreadPoolThread) malloc(sizeof(sThreadPoolThread));
  
  /* Set the attributes of the threadPoolThread */
  resource->myPool = pool;
  resource->next   = NULL;
  resource->prev   = NULL;
  resource->state  = JOIN_STATE_RUNNING_UNJOINED;
  
  resource->threadFunc       = NULL;
  resource->threadFuncArgs   = NULL;
  resource->threadFuncResult = NULL;

  threading_mutex_init(&(resource->startConditionMutex));
  threading_mutex_init(&(resource->joinStateMutex));
  err1 = pthread_cond_init(&(resource->startCondition), NULL); 
  err2 = pthread_cond_init(&(resource->joinStateCondition), NULL);
  
  err3 = threading_pthread_create(&(resource->thread), &(pool->attribute),
                                  (ThreadFunc) threadPoolThread_mainLoop,
                                  resource);
  
  if ((err1 | err2 | err3) != 0) {
    error = ErrNew(ERR_THREADING, 0, NULL,
                   "ThreadPool_create failed: Couldn't init new thread.");
    free(resource);
    goto ERR1;
  }

  *res = resource;
  
 ERR1:
  return error;
}

static void threadPool_makeAvailable(ThreadPoolThread resource) {
  ThreadPool pool = resource->myPool;
  ThreadPoolThread tempElement;
  
  tempElement = pool->firstAvailableThread;
  if (tempElement != NULL)
    tempElement->prev = resource;
  resource->next = tempElement;
  pool->firstAvailableThread = resource;
  resource->prev = NULL;
}

static Error threadPoolThread_destroy(ThreadPoolThread thread) {
  Error error = NULL;
  void *threadFuncErr;
  int err;
  
  /* This finishes and frees the thread, the start cond should be signaled when
   * this point is reached */
  err = pthread_join(thread->thread, &threadFuncErr);
  assert(err == 0);

  thread->next   = NULL;
  thread->prev   = NULL;
  thread->myPool = NULL;

  thread->threadFunc       = NULL;
  thread->threadFuncArgs    = NULL;
  thread->threadFuncResult = NULL;

  err = pthread_cond_destroy(&(thread->startCondition));
  assert(err == 0);
  threading_mutex_destroy(&(thread->startConditionMutex));

  err = pthread_cond_destroy(&(thread->joinStateCondition));
  assert(err == 0);
  threading_mutex_destroy(&(thread->joinStateMutex));

  free(thread);

  /* Check if we got an error from the mainLoop */
  if (threadFuncErr != NULL)
    error = (Error) threadFuncErr;
  
  return error;
}
