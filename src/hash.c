/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
      hash.c -- General hasing routines    

      This file was part of The Great Hemul Project!
      It is now part of Voxi's Intelligent Speech Interface code.

      $Id$ 
      
      These functions are in the process of being made thread-safe with the
      help of POSIX semaphores. 
      
      The cursor things have semaphores, but we also need semaphores on
      probably each element in the HashTable, so that elements are not 
      removed while a cursor is being set on it.
      
      Bug fix 990412 in HashDestroyTable by erl.
      
      WARNING: semaphores made conditional on _POSIX_SEMAPHORES 2002-03-15.
      Will the cursor stuff still work properly if we don't have semaphores?
*/

#include <voxi/util/config.h>

#include <assert.h>
#include <ctype.h> /* for tolower */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h> /* for perror */

#if HAVE_UNISTD_H
#include <unistd.h> /* For POSIX-feature definitions on unix-like systems */
#endif

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif /* WIN32 */

#if HAVE_PTHREAD_H
/* Needed for semaphores. POSIX-feature defs are here on some platforms. */
#include <pthread.h>
#endif

#if HAVE_SEMAPHORE_H
/* POSIX-feature defs are also here on some platforms. */
#include <semaphore.h>  
#endif

#include <voxi/alwaysInclude.h>
#include <voxi/util/err.h>

#include <voxi/util/hash.h>

CVSID("$Id$");

/* --------- Definitions */

typedef struct HiPtr {
  struct HiPtr *next;
  void *data;

  /* a semaphore on the element. The element may not be removed from the 
     hashtable without first setting this semaphore.
 
     Not Implemented Yet (tm)
  */
  /* sem_t semaphore; */

  /* This value is set to non-NULL if there is a cursor pointing to this 
     element. This information is neccessary in case the element is deleted
     while the cursor is traversing the hash table. 
 
     The cursors are in a linked list.
     This list is protected by a monitor which all accesses must respect.
  */
#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS) 
  sem_t cursorListSemaphore;
#endif
  HashTableCursor firstCursor;
} *HashInfoPtr;

struct Hasht 
{
  unsigned int size;
  int elementCount;

  /* Should calculate the hashvalue for a specific data. */
  HashFuncPtr calcIndx; 
  CompFuncPtr compData;    /* Should return 0 iff data1 and data2 are equal */
  /* Should free memory pointed to by the argument. */
  DestroyFuncPtr destrData;  
  
  int debugLevel;
  
  HashInfoPtr infoArray[0];
};

struct sHashTableCursor
{
  HashTable hashTable;

  /* all cursors pointing to an HashInfoPtr are in a double-linked list for 
     that element.
  */
 
  HashTableCursor nextCursor;
  HashTableCursor prevCursor;

  int hashIndex;
  HashInfoPtr element;

} sHashTableCursor;

/* --------- static function prototypes */
static void addCursorToList( HashTableCursor cursor );
static void moveCursorToNextIndex( HashTableCursor cursor );
static void removeCursorFromList( HashTableCursor cursor );


/* --------- Help functions */

/* Returns the record containing the specified data, NULL if not found */
static HashInfoPtr FindInfo(HashTable ht, void *data, unsigned int hashval)
{ 
  HashInfoPtr tmp;

/*  #warning "ErrPushFunc disabled for debugging" */
/*    ErrPushFunc("FindInfo"); */

  for( tmp=ht->infoArray[hashval]; 
       (tmp!=NULL) && (ht->compData(data, tmp->data)); 
       tmp=tmp->next)
    ;

/*    ErrPopFunc(); */
  return(tmp);
}

static void DestroyInfo(HashTable ht, HashInfoPtr rec)
/* Frees the memory occupied by rec */
{
  int error;

  ErrPushFunc("DestroyInfo");

  /* moving the cursor to the next element is not yet implemented */
  assert( rec->firstCursor == NULL );

#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS)
  error = sem_destroy( &(rec->cursorListSemaphore) );
  if( error != 0 )
    perror( "hash.c DestroyInfo sem_destroy" );
#endif
  if( ht->destrData != NULL )
    ht->destrData(rec->data);
  
  free(rec);

  ErrPopFunc();
}

/* ----------------------- */

HashTable HashCreateTable(unsigned int sz, HashFuncPtr calc, 
                     CompFuncPtr comp, DestroyFuncPtr destr)
{
  HashTable ht;

  DEBUG("enter\n");

  ErrPushFunc("HashCreateTable");
  
  assert( sz != 0 );
  
  if((ht=(HashTable)malloc(sizeof(struct Hasht) + sz*sizeof(HashInfoPtr))) != NULL) {
    unsigned int i;
    ht->size = sz;
    for(i=0; i<sz; i++) ht->infoArray[i]=NULL;
    ht->calcIndx = calc;
    ht->compData = comp;
    ht->destrData = destr;
    ht->elementCount = 0;
    ht->debugLevel = 0;
  }
  
  ErrPopFunc();

  DEBUG("leave\n");

  return(ht);
}

/*
  Set the debugging level for the hash table. 0 = no debugging.
  Returns the previous debugging level 
*/
int HashSetDebug( HashTable ht, int debugLevel )
{
  int oldLevel;
  
  oldLevel = ht->debugLevel;
  
  ht->debugLevel = debugLevel;
  
  return oldLevel;
}

void HashDestroyTable(HashTable ht)
{
  unsigned int i;
  HashInfoPtr tmp, tmp2;

  ErrPushFunc("HashDestroyTable");
  for(i=0; i < ht->size; i++)
    for(tmp=ht->infoArray[i]; tmp!=NULL; ) 
    {
      /* erl 990412. DestroyInfo frees its second parameter, so we cannot reuse
         it by looking at its ->next after DestroyInfo has been called. */
      tmp2 = tmp;
      tmp = tmp->next;
      
      DestroyInfo(ht, tmp2);
    }
  
  free(ht);
  ErrPopFunc();
}

int HashGetElementCount( HashTable ht )
{
  return ht->elementCount;
}

void *HashFind(HashTable ht, void *data)
/* If a data record matching 'data' is found in ht, a pointer to it is 
   returned.  Otherwise NULL. */
{
  void *foundData;
  HashInfoPtr tmp;

  /* ErrPushFunc("HashFind"); */
  
  assert( ht != NULL && ht->size != 0 );
  
  foundData=NULL;
  if((tmp = FindInfo(ht, data, ht->calcIndx(data) % ht->size)) != NULL) {
    foundData=tmp->data;
  }
  /* ErrPopFunc(); */
  return(foundData);
}

int HashDestroy(HashTable ht, void *data)
/* Removes the record containing 'data' from ht and DESTROYS data. 
   If no such record exists, it returns 0. On success 1 is returned. */
{
  int retval=0;
  HashInfoPtr tmp;
  unsigned int hashval = ht->calcIndx(data)%ht->size; 

  ErrPushFunc("HashDestroy");
  if(ht->infoArray[hashval]!=NULL) {
    if(!(ht->compData(ht->infoArray[hashval]->data, data))) {
      HashInfoPtr toTrash;             /*First record matched -- remove it! */
      toTrash=ht->infoArray[hashval];
      ht->infoArray[hashval]=toTrash->next;
      DestroyInfo(ht, toTrash);
      ht->elementCount--;

      assert( ht->elementCount >= 0 );

      retval=1;
    }
    else {
      for( tmp = ht->infoArray[hashval];      /* Continue search for match */
           (tmp->next!=NULL) && (ht->compData(tmp->next->data, data));
           tmp = tmp->next )
        ;

      if(tmp->next!=NULL) {      /* If found, remove the matching record. */
        HashInfoPtr toTrash;
        toTrash = tmp->next;
        tmp->next = toTrash->next;
        DestroyInfo(ht, toTrash);
        ht->elementCount--;
        assert( ht->elementCount >= 0 );
        retval=1;
      }
    }
  }
  ErrPopFunc();
  return(retval);
}

int HashDelete(HashTable ht, void *data)
/* Removes the record containing 'data' from ht. If no such record exists, 
   hashDelete returns 0. On success it returns 1. DOES NOT DESTROY DATA */
{
  int retval=0;
  HashInfoPtr tmp;
  unsigned int hashval = ht->calcIndx(data)%ht->size; 

  ErrPushFunc("HashDelete");
  if(ht->infoArray[hashval]!=NULL) {
    if(!(ht->compData(ht->infoArray[hashval]->data, data))) {
      HashInfoPtr toTrash;             /*First record matched -- remove it! */
      toTrash=ht->infoArray[hashval];
      ht->infoArray[hashval]=toTrash->next;
      free(toTrash);
      ht->elementCount--;
      assert( ht->elementCount >= 0 );
      retval=1;
    }
    else {
      for( tmp = ht->infoArray[hashval];      /* Continue search for match */
           (tmp->next!=NULL) && (ht->compData(tmp->next->data, data));
           tmp = tmp->next)
        ;

      if(tmp->next!=NULL) {      /* If found, remove the matching record. */
        HashInfoPtr toTrash;
        toTrash = tmp->next;
        tmp->next = toTrash->next;
        free(toTrash);
        ht->elementCount--;
        assert( ht->elementCount >= 0 );

        retval=1;
      }
    }
  }
  ErrPopFunc();
  return(retval);
}

int HashAdd(HashTable ht, void *data)
/* Adds data to the specified hashtable. If an equivalent record (according to
   ht->compData() ) exists, no adding is done. Returns 1 on success, 0 on 
   failure. */
{
  int retval = 1;
  unsigned int hashval = ht->calcIndx(data)%ht->size;
  HashInfoPtr newInfo;

  ErrPushFunc("HashAdd");
  if(FindInfo(ht, data, hashval))
    retval=0;
  else {
    if( (newInfo = (HashInfoPtr)malloc(sizeof(struct HiPtr))) != NULL ) {
      int error;

      newInfo->data = data;
      newInfo->next = ht->infoArray[hashval];
      newInfo->firstCursor = NULL;
#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS)
      error = sem_init( &(newInfo->cursorListSemaphore), 0, 1 );
      assert( error == 0 );
#endif
      /* I believe this is an atomic action and therefore needs not be 
         protected by a semaphore to be thread-safe.
     
         Can this be guaranteed? What does the pthreads package say about
         atomicity? Anyway, the elementCount needs to be semaphored methinks.
 
         /Erl 981015
      */
      ht->infoArray[hashval] = newInfo;
      ht->elementCount++;
    }
    else
      retval = 0;
  }
  ErrPopFunc();
  return(retval);
}

/*
  Cursor functions.

  added by erl 981015
*/

HashTableCursor HashCursorCreate( HashTable ht )
{
  HashTableCursor result;

  if( ht->debugLevel > 1 )
    fprintf( stderr, "hash.c: HashCursorCreate( %p ): entering\n", ht );

  result = malloc( sizeof( sHashTableCursor ) );
  assert( result != NULL );

  result->hashTable = ht;
  result->prevCursor = NULL;
  result->nextCursor = NULL;
  result->element = NULL;

  HashCursorGoFirst( result );

  if( ht->debugLevel > 0 )
    fprintf( stderr, "hash.c: HashCursorCreate( %p ) = %p\n", ht, result );
  
  return result;
}

void HashCursorDestroy( HashTableCursor cursor )
{
  if( cursor->hashTable->debugLevel > 0 )
    fprintf( stderr, "hash.c: HashCursorDestroy( %p )\n", cursor );
  
  /* if the cursor points at an element, then remove it from the cursor
     list of that element */

  if( cursor->element != NULL )
    removeCursorFromList( cursor );

  free( cursor );
}

/* Move the cursor to the first element in the hashtable (according to some
   undefined, arbitrary order).
*/
void HashCursorGoFirst( HashTableCursor cursor )
{
  if( cursor->hashTable->debugLevel > 0 )
    fprintf( stderr, "hash.c: HashCursorGoFirst( %p ): entry\n", cursor );
  
  /* 
     if the cursor is pointing at an element, then remove it from the cursor 
     list of that element.
  */
  if( cursor->element != NULL )
  {
    removeCursorFromList( cursor );
    cursor->element = NULL;
  }
  
  cursor->hashIndex = -1;
   
  moveCursorToNextIndex( cursor );
   
  if( cursor->hashTable->debugLevel > 1 )
    fprintf( stderr, "hash.c: HashCursorGoFirst( %p ): exit\n", cursor );
}

/* Move the cursor to the next element in the hash table */
void HashCursorGoNext( HashTableCursor cursor )
{
  if( cursor->element->next == NULL )
  {
    /* find the next hash index with an entry */
    removeCursorFromList( cursor );
    
    cursor->element = NULL;
    
    moveCursorToNextIndex( cursor );
  }
  else
  {
    removeCursorFromList( cursor );
    
    cursor->element = cursor->element->next;
    
    addCursorToList( cursor );
  }
}

/* return TRUE if the cursor has passed the last element */
Boolean HashCursorPastLastElement( HashTableCursor cursor )
{
  return (cursor->hashIndex >= (long) cursor->hashTable->size);
}

void *HashCursorGetElement( HashTableCursor cursor )
{
  assert( cursor->element != NULL );
  
  return cursor->element->data;
}

int HashString( const char *string )
{
  int hashValue;

  /* 
     be a bit tolerant 
  */
  if( string == NULL )
    return 0;
  
  /* do it any old way now, we can implement something theoretically 
     superior later if we need it 
  */
  for( hashValue = 0; (*string != '\0'); string++ )
    hashValue = (hashValue << 4) ^ (*string);
  
  return hashValue;
}

int HashLowercaseString( const char *string )
{
  int hashValue;

  if( string == NULL )
    return 0;
  
  /* do it any old way now, we can implement something theoretically 
     superior later if we need it 
  */
  for( hashValue = 0; (*string != '\0'); string++ )
    hashValue = (hashValue << 4) ^ (tolower(*string));

  return hashValue;
}

static void removeCursorFromList( HashTableCursor cursor )
{
  int error;

  if( cursor->hashTable->debugLevel > 1 )
    fprintf( stderr, "hash.c: removeCursorFromList( %p ): entry\n", cursor );

  assert( cursor->element != NULL );
#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS)
  sem_wait( &(cursor->element->cursorListSemaphore) );
#endif
  if( cursor->hashTable->debugLevel > 2 )
    fprintf( stderr, "hash.c: removeCursorFromList( %p ): prevCursor=%p, "
             "nextCursor = %p\n", 
             cursor, cursor->prevCursor, cursor->nextCursor );

  if( cursor->prevCursor != NULL )
  {
    cursor->prevCursor->nextCursor = cursor->nextCursor;
    cursor->prevCursor = NULL;
  }
  else
    cursor->element->firstCursor = cursor->nextCursor;

  if( cursor->nextCursor != NULL )
  {
    cursor->nextCursor->prevCursor = cursor->prevCursor;
    cursor->nextCursor = NULL;
  }
#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS)
  error = sem_post( &(cursor->element->cursorListSemaphore) );
  assert( error == 0 );
#endif
  
  if( cursor->hashTable->debugLevel > 1 )
    fprintf( stderr, "hash.c: removeCursorFromList( %p ): entry\n", cursor );
}

static void addCursorToList( HashTableCursor cursor )
{
  int error;

  assert( cursor->element != NULL );

  if( cursor->hashTable->debugLevel > 1 )
    fprintf( stderr, "hash.c: addCursorToList( %p ): entry\n", cursor );
#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS)
  sem_wait( &(cursor->element->cursorListSemaphore) );
#endif
  /* add the cursor to the list of cursors that point at this element */
  cursor->nextCursor = cursor->element->firstCursor;
  cursor->prevCursor = NULL;
  cursor->element->firstCursor = cursor;
  if( cursor->nextCursor != NULL )
    cursor->nextCursor->prevCursor = cursor;
  
  if( cursor->hashTable->debugLevel > 2 )
    fprintf( stderr, "hash.c: addCursorToList( %p ): nextCursor=%p, "
             "prevCursor=%p\n", cursor, cursor->nextCursor, 
             cursor->prevCursor );
#if defined (_POSIX_SEMAPHORES) && defined (_POSIX_THREADS)
  error = sem_post( &(cursor->element->cursorListSemaphore) );
  assert( error == 0 );
#endif
  if( cursor->hashTable->debugLevel > 1 )
    fprintf( stderr, "hash.c: addCursorToList( %p ): exit\n", cursor );
}

void *HashGetArbitraryElement( HashTable hashTable )
{
  unsigned int hashIndex;

  assert( hashTable->elementCount > 0 );

  for( hashIndex = 0; (hashIndex < hashTable->size) && 
       (hashTable->infoArray[ hashIndex ] == NULL); 
       hashIndex++ )
    ;

  assert( hashIndex < hashTable->size );

  return hashTable->infoArray[ hashIndex ]->data;
}

/* 
   moves the cursor to the next hash index, and positions it at the first
   element of that index. 
 
   Assumes the cursor is not pointing at an element.
*/
static void moveCursorToNextIndex( HashTableCursor cursor )
{
  assert( cursor->element == NULL );
  
  for( cursor->hashIndex++; 
       (cursor->hashIndex < ((long) cursor->hashTable->size)) && 
         (cursor->hashTable->infoArray[ cursor->hashIndex ] == NULL); 
       cursor->hashIndex++ )
    ;

  if( cursor->hashIndex < ((long) cursor->hashTable->size) )
  {
    cursor->element = cursor->hashTable->infoArray[ cursor->hashIndex ];
  
    assert( cursor->element != NULL );

    addCursorToList( cursor );
  }
  else
    cursor->element = NULL;
}
