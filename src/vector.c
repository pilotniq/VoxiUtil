/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * vector.c
 *
 * A vector is a dynamic one-dimensional array.
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for POSIX-feature definitions */
#endif

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif /* WIN32 */

#include <voxi/alwaysInclude.h>
#include <voxi/util/collection.h>
#ifdef _POSIX_THREADS
#include <voxi/util/threading.h>
#endif
#include <voxi/util/vector.h>

CVSID("$Id$");

typedef struct sVector
{
  size_t elementSize;

  int capacity;
  int capacityIncrement;
  int elementCount;
  
  int refCount;
  DestroyElementFunc destroyFunc;
  
  char *name;
#ifdef _POSIX_THREADS
  sVoxiMutex lock;
  sVoxiMutex refCountLock;
#endif
  char *data;

#ifndef NDEBUG
  void *cookie;
#endif

} sVector;

/*
 *  static functions
 */
static void increaseCapacity( Vector vector );
static void vectorDestroy( Vector vector );
     
/*
 * code
 */

#ifndef NDEBUG
Boolean vector_isValid( Vector vector ) {
  assert( vector !=  NULL );
  assert( vector->elementSize > 0 );
  assert( vector->capacity >= 0 ); 
  assert( vector->capacityIncrement > 0 );
  assert( vector->elementCount >= 0);
  assert( vector->refCount > 0 );
  /* FIXME: destroyFunc, name, lock, refCountLock */
  assert( vector->data != NULL );

  assert( vector->cookie == vector_isValid );

  assert( vector->capacity >= vector->elementCount );

  return TRUE;
}
#endif 


Vector vectorCreate( const char *name, size_t elementSize,
                     int initialCapacity, int capacityIncrement,
                     DestroyElementFunc destroyFunc )
{
  Error error = NULL;
  Vector result = NULL;

  DEBUG("enter\n");
  
  result = malloc( sizeof( sVector ) );
  if (result == NULL) {
    goto ERR_RETURN;
  }
  
  if( name == NULL )
    result->name = NULL;
  else {
    result->name = strdup( name );
    if (result->name == NULL) {
      goto ERR_RETURN;
    }
  }
  
  result->elementSize = elementSize;
  result->capacity = initialCapacity;
  result->capacityIncrement = capacityIncrement;
  result->destroyFunc = destroyFunc;
  result->refCount = 1;
  
  result->data = malloc( elementSize * initialCapacity );
  if (result->data == NULL) {
    goto ERR_RETURN;
  }
  result->elementCount = 0;
#ifdef _POSIX_THREADS
  error = threading_mutex_init( &(result->lock) );
  if (error != NULL) {
    ErrDispose(error, TRUE);
    goto ERR_RETURN;
  }
  error = threading_mutex_init( &(result->refCountLock) );
  if (error != NULL) {
    ErrDispose(error, TRUE);
    goto ERR_RETURN;
  }
#endif
#ifndef NDEBUG
  result->cookie = vector_isValid;
#endif

  DEBUG("leave\n");

  return result;
  
 ERR_RETURN:
  if (result) {
    if (result->name)
      free(result->name);
    if (result->data)
      free(result->data);
    free(result);
    result = NULL;
  }
  return NULL;
}

Vector vectorClone( const char *name, Vector a ) 
{
  Vector b;

  assert(vector_isValid( a ) );

  b = vectorCreate( name, 
                    a->elementSize, 
                    a->elementCount+a->capacityIncrement,
                    a->capacityIncrement,
                    a->destroyFunc);

  memcpy( b->data, a->data, a->elementSize * a->elementCount );
  b->elementCount = a->elementCount;
  
  assert(vector_isValid( b ));

  return b;
}

void vectorRefcountIncrease( Vector vector )
{
#ifdef _POSIX_THREADS
  threading_mutex_lock( (VoxiMutex) &(vector->refCountLock) );
#endif
  vector->refCount++;
#ifdef _POSIX_THREADS
  threading_mutex_unlock( (VoxiMutex) &(vector->refCountLock) );
#endif
}

void vectorRefcountDecrease( Vector vector )
{
#ifdef _POSIX_THREADS
  threading_mutex_lock( (VoxiMutex) &(vector->refCountLock) );
#endif
  assert( vector->refCount > 0 );
  
  vector->refCount--;
  
  if( vector->refCount == 0 )
  {
    /* assert( vector is not locked ) */
    vectorDestroy( vector );
  }
#ifdef _POSIX_THREADS
  else
    threading_mutex_unlock( &(vector->refCountLock) );
#endif
}
  
static void vectorDestroy( Vector vector )
{
  int i;
  char *ptr;
  
  /* assert refCountLock is locked */
#ifdef _POSIX_THREADS
  threading_mutex_unlock( &(vector->refCountLock) );
#endif
  if( vector->destroyFunc != NULL )
    for( i = 0, ptr = vector->data; i < vector->elementCount; 
         i++, ptr += vector->elementSize )
      vector->destroyFunc( ptr );
  
  if( vector->name != NULL )
    free( vector->name );
  
  free( vector->data );
#ifdef _POSIX_THREADS
  threading_mutex_destroy( &(vector->lock) );
  threading_mutex_destroy( &(vector->refCountLock) );
#endif
#ifndef NDEBUG
  vector->cookie = vectorDestroy;
#endif
  
  free( vector );
}

int vectorGetElementCount( ConstVector vector )
{
  return vector->elementCount;
}

int vectorAppend( Vector vector, const void *element )
{
  int index;
  
#ifdef _POSIX_THREADS
  const char *prevLocker;
  
  prevLocker = vectorLockDebug( vector, "vectorAppend" );
#endif
  if( vector->elementCount == vector->capacity )
    increaseCapacity( vector );
  
  assert( vector->capacity > vector->elementCount );
  
  index = vector->elementCount;
  
  memcpy( vector->data + index * vector->elementSize, element, 
          vector->elementSize );
  
  vector->elementCount++;
#ifdef _POSIX_THREADS
  vectorUnlockDebug( vector, prevLocker );
#endif
  return index;
}

int vectorAppendPtr( Vector vector, const void *ptr )
{
  int index;
#ifdef _POSIX_THREADS
  const char *prevLocker;
  
  prevLocker = vectorLockDebug( vector, "vectorAppendPtr" );
#endif
  assert( vector->elementSize == sizeof( void *) );
  
  if( vector->elementCount == vector->capacity )
    increaseCapacity( vector );
  
  assert( vector->capacity > vector->elementCount );
  
  index = vector->elementCount;
  
  *((void **) (vector->data + index * vector->elementSize)) = (void *) ptr;
  
  vector->elementCount++;
#ifdef _POSIX_THREADS
  vectorUnlockDebug( vector, prevLocker );
#endif
  return index;
}


void vectorRemoveLastElement( Vector vector )
{
  if( vector->elementCount != 0 )
    vector->elementCount--;
  else
    assert( FALSE );
}

void vectorRemoveAll( Vector vector )
{
  if( vector->destroyFunc != NULL )
  {
    int i;
    char *ptr;

    for( i = 0, ptr = vector->data; i < vector->elementCount; 
         i++, ptr += vector->elementSize )
      vector->destroyFunc( ptr );
  }
  
  vector->elementCount = 0;

  return;
}

void vectorGetElementAt( Vector vector, int index, void *element )
{
#ifdef _POSIX_THREADS
  const char *prevLocker;
  
  prevLocker = vectorLockDebug( vector, "vectorGetElementAt" );
#endif
  assert( index < vector->elementCount );
  assert( index >= 0 );
  
  memcpy( element, vector->data + index * vector->elementSize, 
          vector->elementSize );
#ifdef _POSIX_THREADS
  vectorUnlockDebug( vector, prevLocker );
#endif
}

/*
 * Can only set elements from 0...vectorGetElementCount(), that is at most 
 * create the new element.
 */
void vectorSetElementAt( Vector vector, int index, /*@out@*/void *element )
{
#ifdef _POSIX_THREADS
  const char *prevLocker;
  
  prevLocker = vectorLockDebug( vector, "vectorSetElementAt" );
#endif

  assert( index <= vector->elementCount );
  
  if( index == vector->elementCount )
  {
    /* enlarge vector if neccessary */
    if( index >= vector->capacity )
      increaseCapacity( vector );
    
    vector->elementCount++;
  }
  
  memcpy( vector->data + index * vector->elementSize, element,
          vector->elementSize );
  
#ifdef _POSIX_THREADS
  vectorUnlockDebug( vector, prevLocker );
#endif
}


/* Should be modified to throw an error when there is no thread support 
 
   Returns the old locker, which must be passed to vectorUnlockDebug
*/
const char *vectorLockDebug( Vector vector, const char *where )
{
#ifdef _POSIX_THREADS
  /* cast away the const */
  return threading_mutex_lock_debug( (VoxiMutex) &(vector->lock), where );
#else
  return NULL;
#endif
}

/* Should be modified to throw an error when there is no thread support */
void vectorLock( Vector vector )
{
#ifdef _POSIX_THREADS
  threading_mutex_lock( &(vector->lock) );
#endif
  /* vectorLockDebug( vector, "vectorLock" ); */
}

/* Should be modified to throw an error when there is no thread support */
void vectorUnlock( Vector vector )
{
#ifdef _POSIX_THREADS
  threading_mutex_unlock( (VoxiMutex) &(vector->lock) );
#endif
}

void vectorUnlockDebug( Vector vector, const char *prevLocker )
{
#ifdef _POSIX_THREADS
  threading_mutex_unlock_debug( (VoxiMutex) &(vector->lock), prevLocker );
#endif
}

void vectorDebug( ConstVector vector, Boolean state )
{
#ifdef _POSIX_THREADS
  threading_mutex_setDebug( &( ((Vector) vector)->lock), state );
#endif
}

static void increaseCapacity( Vector vector )
{
#ifdef _POSIX_THREADS
  vectorLockDebug( vector, "increaseCapacity" );
#endif
  assert( vector->capacityIncrement > 0 );
  
  vector->capacity += vector->capacityIncrement;
  
  vector->data = realloc( vector->data, 
                          vector->capacity * vector->elementSize );
  assert( vector->data != NULL );
#ifdef _POSIX_THREADS
  vectorUnlock( vector );
#endif
}
