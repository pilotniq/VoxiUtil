/*
 * config.h
 *
 * Win32 configuration for voxi util library
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifndef _REENTRANT
#define _REENTRANT
#endif

#ifndef _FILE_IO
#define _FILE_IO 1
#endif

#define HAVE___INT64 1

/*
   HAVE_PTHREAD_H is if the system has pthreads.
   _POSIX_THREADS is if we want to configure voxiUtil to use posix threads
   */
#define HAVE_PTHREAD_H 1
#define _POSIX_THREADS 1
#define HAVE_SEMAPHORE_H 1

#endif