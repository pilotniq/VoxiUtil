/**  
  @file circularBuffer.c
  
  @author Erland Lewin <erl@voxi.com>
  
  Semantics:
  
  A circular buffer is a buffer of a limited length. If elements are written 
  (pushed) to it when it is full, the oldest element is removed from the buffer
  and destroyed (if a destroyFunc has been specified).
  
  It was initially written for the push-to-talk / speech detection code, to 
  buffer for a limited amount of time.
  
  It internally stores elements like this:
  
  Vector:
  
     0         1        2  3  4     5         6 
              tail                 head   
  element3  element4             element1  element2
*/

/*
  Copyright (C) 2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

#include <assert.h>
#include <stdlib.h>

#include <voxi/debug.h>
#include <voxi/util/err.h>
#include <voxi/util/mem.h>
#include <voxi/util/vector.h>

#include <voxi/util/circularBuffer.h>

/*
 *  Type definitions
 */
typedef struct sCircularBuffer
{
  int size;
  Vector vector;
  int headIndex;
  int tailIndex;
  int elementCount;
  DestroyElementFunc destroyFunc;
} sCircularBuffer;

/*
 * Code
 */

Error circBuf_create( int size, DestroyElementFunc destroyFunc,
                      CircularBuffer *result )
{
  Error error;
  void *nullPtr = NULL;
  int i;
  
  assert( result != NULL );
  
  error = emalloc( (void **) result, sizeof( sCircularBuffer ) );
  if( error != NULL )
  {
    error = ErrNew( ERR_UNKNOWN, 0, error, "Failed to allocate %d bytes for "
                    "the sCircularBuffer struct", sizeof( sCircularBuffer ) );
    goto CREATE_FAIL_1;
  }
  
  (*result)->size = size;
  (*result)->vector = vectorCreate( "Circular Buffer Vector", sizeof( void * ),
                                    size, 1, NULL );
  (*result)->headIndex = 0;
  (*result)->tailIndex = 1;
  (*result)->elementCount = 0;
  (*result)->destroyFunc = destroyFunc;
  
  /*
    Initialize the vector elements
  */
  for( i = 0; i < size; i++ )
    vectorSetElementAt( (*result)->vector, i, &nullPtr );
  
  assert( error == NULL );
  
  return NULL;
  
 CREATE_FAIL_1:
  assert( error != NULL );
  
  return error;
}

Error circBuf_destroy( CircularBuffer circBuf )
{
  int vectorIndex;
  
  /*
    If a destroyFunc is specified, then Destroy all elements in in the buffer. 
    Free the vector. 
    Free the buffer.
  */
  
  if( circBuf->destroyFunc != NULL )
  {
    int i;
    
    for( i = 0, vectorIndex = circBuf->headIndex; i < circBuf->elementCount; 
         i++ )
    {
      void *element;
      
      vectorGetElementAt( circBuf->vector, vectorIndex, &element );
      circBuf->destroyFunc( element );
      
      vectorIndex++;
       
      assert( vectorIndex <= circBuf->size );
       
      if( vectorIndex == circBuf->size )
        vectorIndex = 0;
    }
  }
  
  /*
    This will destroy the vector
  */
  vectorRefcountDecrease( circBuf->vector );
  
  free( circBuf );
  
  return NULL;
}

void circBuf_push( CircularBuffer circBuf, void *element )
{
  /* 
     if the buffer is full, destroy the last (tail) element.
     add the new element at the head.
  */
  if( circBuf->elementCount == circBuf->size )
  {
    /* Delete the last (tail) element if we have a destroyFunc */
    if( circBuf->destroyFunc != NULL )
    {
      void *elementToDestroy;
    
      vectorGetElementAt( circBuf->vector, circBuf->tailIndex, 
                          &elementToDestroy );
      
      DEBUG( "freeing element at %d, head=%d, tail=%d.\n", 
              circBuf->tailIndex, circBuf->headIndex, circBuf->tailIndex );
  
      circBuf->destroyFunc( elementToDestroy );
    }
    
    if( circBuf->tailIndex == (circBuf->size - 1) )
      circBuf->tailIndex = 0;
    else
      circBuf->tailIndex++;
    
    circBuf->elementCount--;
  }
      
  if( circBuf->headIndex == (circBuf->size - 1) )
    circBuf->headIndex = 0;
  else
    circBuf->headIndex++;
  
  /* Write the new element */
  vectorSetElementAt( circBuf->vector, circBuf->headIndex, &element );
  
  DEBUG( "wrote element at %d, head=%d, tail=%d.\n", circBuf->headIndex, 
          circBuf->headIndex, circBuf->tailIndex );
  
  circBuf->elementCount++;
  
  assert( circBuf->elementCount <= circBuf->size );
}

void *circBuf_popTail( CircularBuffer circBuf )
{
  void *nullPtr = NULL;
  void *element;
  
  assert( circBuf->elementCount > 0 );
  
  /* Remove and return the last (tail, oldest) element in the buffer. 
     It is not destroyed. */
  
  vectorGetElementAt( circBuf->vector, circBuf->tailIndex, &element );
  
#ifndef NDEBUG
  vectorSetElementAt( circBuf->vector, circBuf->tailIndex, &nullPtr );
#endif
  
  if( circBuf->tailIndex == (circBuf->size - 1))
    circBuf->tailIndex = 0;
  else
    circBuf->tailIndex++;
  
  circBuf->elementCount--;
  
  return element;
}

int circBuf_getElementCount( CircularBuffer circBuf )
{
  return circBuf->elementCount;
}

void circBuf_clear( CircularBuffer circBuf )
{
  void *element;
  
  while( circBuf->elementCount > 0 )
  {
    element = circBuf_popTail( circBuf );
    circBuf->destroyFunc( element );
  }
}

