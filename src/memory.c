/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  memory.c
  
  Voxi Memory Manager
*/

#include <voxi/util/config.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h> /* for POSIX-feature definitions on unix-like systems */
#endif

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif /* WIN32 */

#include <voxi/alwaysInclude.h>
#include <voxi/types.h>

#if HAVE_PTHREAD_H
/* Threaing is used if and only if we have pthreads. */
#include <voxi/util/threading.h>
#endif

#include <voxi/util/memory.h>

CVSID("$Id$");

#define BLOCK_HEADER( manager, index ) ((BlockHeader) ((manager)->data + (index) * (manager)->blockSize))

typedef struct sMemoryManager
{
  char *name;
  int capacity;
  size_t elementSize;
  size_t blockSize;
  int firstFree;
#ifdef _POSIX_THREADS
  sVoxiMutex mutex;
#endif
  char *data;
} sMemoryManager;

typedef struct sBlockHeader
{
  MemoryManager manager;
  Boolean isFree;
  
  union
  {
    int nextFree; /* if it is free */
    
    struct
    {
      int index;    /* if it is allocated */
    } used;
  } data;
  const char *allocatedFrom;
} sBlockHeader, *BlockHeader;

MemoryManager memManager_create( size_t elementSize, const char *name, 
                                 int capacity )
{
  MemoryManager result;
  char *ptr;
  int i;
  
  DEBUG("enter\n");

  assert( capacity > 0 );
  assert( elementSize > 0 );
  
  result = malloc( sizeof( sMemoryManager ) );
  result->elementSize = elementSize;
  result->blockSize = elementSize + sizeof( sBlockHeader );
  result->name = (name == NULL) ? NULL : strdup( name );
  result->capacity = capacity;
  result->firstFree = 0;
#ifdef _POSIX_THREADS
  threading_mutex_init( &(result->mutex) );
#endif
  result->data = malloc( result->blockSize * result->capacity );
  assert( result->data != NULL );
  
  for( i = 0, ptr = result->data; i < result->capacity; 
       i++, ptr += (elementSize + sizeof( sBlockHeader ) ) )
  {
    ((BlockHeader) ptr)->manager = result;
    ((BlockHeader) ptr)->isFree = TRUE;
    ((BlockHeader) ptr)->allocatedFrom = NULL;
    ((BlockHeader) ptr)->data.nextFree = (i == (result->capacity - 1)) ? -1 : i + 1;
  }
  
  DEBUG("leave\n");

  return result;
}

/*
  NOTE: the where string will not be duped - the pointer must not be free'd by the caller
  and must be valid until the block is freed
*/
void *memManager_allocate( MemoryManager manager, const char *where )
{
  int index;
  BlockHeader header;
  
#ifdef _POSIX_THREADS
  threading_mutex_lock( &(manager->mutex) );
#endif
  index = manager->firstFree;
  if( index != -1 )
  {
    header = BLOCK_HEADER( manager, index );
    manager->firstFree = header->data.nextFree;
  }
  
  assert( index < manager->capacity );
  
  if( index == -1 )
  {
    fprintf( stderr, "ERROR: memory manager '%s' full.\n", manager->name );
    fprintf( stderr, "dumping:.\n" );
    for( index = 0; index < manager->capacity; index++ )
    {
      BlockHeader header;
      
      header = BLOCK_HEADER( manager, index );
      
      assert( header->manager == manager );
      assert( !header->isFree );
      assert( header->data.used.index == index );
      
      fprintf( stderr, "%d: %s\n", header->data.used.index, 
               header->allocatedFrom );
    }
    
    return NULL;
  }
#ifdef _POSIX_THREADS  
  threading_mutex_unlock( &(manager->mutex) );
#endif
  if( header->manager != manager )
  {
    fprintf( stderr, "ERROR: memory.c: header manager=%p, manager=%p, index=%d.\n",
             header->manager, manager, index );
    assert( FALSE );
  }
  
  assert( header->isFree );
  
  header->isFree = FALSE;
  header->data.used.index = index;
  header->allocatedFrom = where;
#if 0
  fprintf( stderr, "Memory manager %s: allocated block %d from %s = %p\n", manager->name,
           index, where, ((char *) header) + sizeof( sBlockHeader ) );
#endif
  return ((char *) header) + sizeof( sBlockHeader );
}

void memManager_validate( void *block )
{
  BlockHeader header;
  MemoryManager manager;
  int index;
  
  header = (BlockHeader) (((char *) block) - sizeof( sBlockHeader ));
  assert( !header->isFree );
  
  manager = header->manager;
  if( manager == NULL )
  {
    fprintf( stderr, "ERROR: memManager_validate( %p ): manager == NULL\n", block );
    return;
  }
  
  index = header->data.used.index;
  assert( index < manager->capacity );
  
  assert( !header->isFree );
  
  /* check that the next block wasn't overwritten */
  if( index < (manager->capacity - 1) )
  {
    header = BLOCK_HEADER( manager, index + 1 );
    if( header->manager != manager )
    {
      fprintf( stderr, "ERROR: Freed block %d seems to have overwritten its successor.\n",
               index );
      header->manager = manager;
    }
  }
#if 0
  fprintf( stderr, "Memory manager %s: freed block %d.\n", manager->name,
           index );
#endif
}

void memManager_free( void *block )
{
  BlockHeader header;
  MemoryManager manager;
  int index;
  
  header = (BlockHeader) (((char *) block) - sizeof( sBlockHeader ));
#if 0
  assert( !header->isFree );
#else
  if( header->isFree )
  {
    assert( header->manager != NULL );
    
    fprintf( stderr, "ERROR: memory manager %s: block %p (alloc'd from %s) free'd "
             "twice?\n", header->manager->name, block, 
             (header->allocatedFrom == NULL) ? "NULL" : header->allocatedFrom );
    return;
  }
#endif
  
  manager = header->manager;
  if( manager == NULL )
  {
    fprintf( stderr, "ERROR: memManager_free( %p ): manager == NULL\n", block );
    return;
  }
  
  index = header->data.used.index;
  assert( index < manager->capacity );
  
  header->isFree = TRUE;
  
#ifdef _POSIX_THREADS
  /* This might be optimized so that frees and locks can be done most often 
     without clashing locks, but that would probably not be worth the effort 
  */
  threading_mutex_lock( &(manager->mutex) );
#endif
  header->data.nextFree = manager->firstFree;
  manager->firstFree = index;
  
#ifdef _POSIX_THREADS
  threading_mutex_unlock( &(manager->mutex) );
#endif
  /* check that the next block wasn't overwritten */
  if( index < (manager->capacity - 1) )
  {
    header = BLOCK_HEADER( manager, index + 1 );
    if( header->manager != manager )
    {
      fprintf( stderr, "ERROR: Freed block %d seems to have overwritten its successor.\n",
               index );
      header->manager = manager;
    }
  }
#if 0
  fprintf( stderr, "Memory manager %s: freed block %d.\n", manager->name,
           index );
#endif
  
  
}
