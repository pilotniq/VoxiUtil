/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file threadpool.h
 *
 * A resource pool for available threads to be used in order to avoid
 * the creation of many threads, which could slow thing down.
 *
 * @author bjorn
 * 
 * Copyright (C) 1999-2002, Voxi AB
 *
 * A thread pool could be used when you need to create many threads
 * that are used only for a short period of time, after which the thread
 * automatically is returned to the pool. This will save the time
 * needed for creation of threads.
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif


typedef struct sThreadPool *ThreadPool;
typedef struct sThreadPoolThread *ThreadPoolThread;

/* the stuff below is a hack to solve recursive include problems */

#include <voxi/util/err.h>
#include <voxi/util/threading.h>

/**
 * Create a new ThreadPool with an initial size and the
 * specified attributes. All threads in the pool will
 * have the specified attributes, which can not be changed
 * after startup.
 *
 * @pre initialSize >= 0
 *
 * @param initialSize The initial number of available threads
 *    in the pool. Must be zero or greater.
 * @param attr One of the values detachedThreadAttr,
 *    realtimeDetachedThreadAttr, joinableDetachedThreadAttr and NULL.
 * NULL means joinable default priority.
 * @param res If error is NULL this will point to a valid ThreadPool object.
 *
 * @return NULL if no error occured, else a pointer to an error.
 */
Error threadPool_create(int initialSize, pthread_attr_t attr, ThreadPool *res);

/**
 * Destroys and frees a thread pool object.
 * Any attempt to call destroy, join or runThread on
 * a thread pool for which destroy has been called
 * will result in an error.
 *
 * @param threadPool The thread pool to be destroyed and freed.
 * @return NULL if successful, else a pointer to an error struct.
 */
Error threadPool_destroy(ThreadPool threadPool);

/**
 * Calls the specified function in the first available thread.
 * If no thread is available, a new one is created.
 *
 * @remarks Calling this function after threadPool_destroy has
 * been called will result in an error.
 *
 * @param threadPool The thread pool to use.
 * @param func The function to call in the new thread.
 * @param args Arguments to pass to the ThreadFunc.
 * @param newThread If error is NULL, this will point to the waken
 * thread pool thread. If the thread pool is non detached, this thread
 * should later be joinedUpon.
 *
 * @return NULL if successful, else a pointer to an error.
 */
Error threadPool_runThread(ThreadPool threadPool, ThreadFunc func, void *args,
                           ThreadPoolThread *newThread);

/**
 * Blocks until the specified thread's thread function has returned and
 * returns the value. This function must only be used for ThreadPoolThreads
 * whose ThreadPool uses non-detached (joinable) threads.
 * If the thread function is finished, this function will return immedialtly.
 *
 * @post The specified ThreadPoolThread is invalid and returned to the pool.
 *
 * @param threadToJoin The threadPoolThread to join. After the join is returned,
 * this thread pool thread will be invalid and returned to the pool.
 * @param threadFuncResult The returned value from the threadFunction.
 *
 * @return NULL if successful, else a pointer to an error.
 */
Error threadPoolThread_join(ThreadPoolThread threadToJoin, void **threadFuncResult);

#ifdef __cplusplus
}  // only need to export C interface if
              // used by C++ source code
#endif


#endif /* ifndef threadpool_h */

