/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	bag.
*/

#include <assert.h>
#include <stdlib.h>

#include <voxi/alwaysInclude.h>

#include <voxi/util/bag.h>

#define PreCond(X)  assert(X)
#define PostCond(X) assert(X)

CVSID("$Id$");

typedef struct sBag
{
  void *cookie;
  int noElements;
  int capacity;
  int capacityIncrement;
  size_t elementSize;
	
  void *array;

  DestroyElementFunc destroyFunc;

#ifdef _POSIX_THREADS
  sVoxiMutex lock;
#endif
} sBag;

typedef struct sBagIterator
{
  int current;
  /*size_t elemSize; */
  /*int capacity; */
  Bag bag;
} sBagIterator;

Boolean bag_isValid( Bag bag ) {
  if( bag == NULL ) return FALSE;

  if( bag->cookie != bag_isValid ) return FALSE;

  if( bag->noElements < 0 ) return FALSE;
  if( bag->capacity < 0 ) return FALSE;
  if( bag->capacityIncrement < 0 ) return FALSE;
  if( bag->elementSize <= 0 ) return FALSE;
  if( bag->array == NULL ) return FALSE;

  /* plainly speculative.. pointers are usually "always" big.. */
  /* how else can i test that the pointer is valid... */
  if( bag->array < (void*)1000 ) return FALSE;
  
  return TRUE;
}

/* 
   static functions 
*/
static void ensureIncCapacity( Bag bag );

		 
/* 
   beginning of code 
*/

void bagLock( Bag bag )
{
#ifdef _POSIX_MUTEX_THREADS
  threading_mutex_lock( (VoxiMutex) &(bag->lock) );
#endif
}

void bagUnlock( Bag bag )
{
#ifdef _POSIX_MUTEX_THREADS
    threading_mutex_unlock( &(bag->lock) );
#endif
}

Bag bagCreate( int initialCapacity, 
	       int capacityIncrement,
	       DestroyElementFunc destroyFunc )
{
  /* default is a bag of pointers unless otherwise specified */
  return bagCreate2( initialCapacity, 
		     capacityIncrement, 
		     sizeof( void * ),
		     destroyFunc );
}

Bag bagCreate2( int initialCapacity, 
		int capacityIncrement, 
		size_t elementSize,
		DestroyElementFunc destroyFunc )
{
  Bag result;

  DEBUG("enter\n");
	
  assert( capacityIncrement >= 0 );
	
  result = malloc( sizeof( sBag ) );
  assert( result != NULL );
	
  result->cookie = bag_isValid;

  result->noElements = 0;
  result->capacity = initialCapacity;
  result->capacityIncrement = capacityIncrement;
	
  result->elementSize = elementSize;
	
  result->array = malloc( result->elementSize * initialCapacity );
  
  result->destroyFunc = destroyFunc;
 
  assert( result->array != NULL );

#ifdef _POSIX_THREADS
  threading_mutex_init( &(result->lock) );
#endif

  DEBUG("leave\n");

  PostCond( bag_isValid( result ) );
  return result;
}

/* creates a new bag with the same elements as the original */
Bag bagDuplicate( Bag original )
{
  Bag result;

  PreCond( original == NULL || bag_isValid( original ) );
	
  if( original == NULL )
    return NULL;

  result = malloc( sizeof( sBag ) );
  assert( result != NULL ); 
	
  bagLock( original );

  result->cookie = original->cookie;
	
  result->noElements = original->noElements; 
  result->capacity = original->capacity; 
  result->capacityIncrement = original->capacityIncrement;
	
  result->elementSize = original->elementSize; 
	
  result->array = malloc( result->elementSize * result->capacity ); 

  if (original->noElements > 0 )          
    assert( result->array != NULL ); 
	
  memcpy( result->array, original->array, 
	  result->noElements * result->elementSize );

  bagUnlock( original );

  PostCond( bag_isValid( result ) );

  return result;
}

void bagDestroy( Bag bag, DestroyElementFunc destroyFunc )
{
  int i;
  
  PreCond( bag == NULL || bag_isValid( bag ) );

  if( bag == NULL )
    return;

  bag->cookie = bagDestroy;
  
  if( destroyFunc != NULL )
    for( i = 0; i < bag->noElements; i++ )
      destroyFunc( ( (void **) bag->array)[ i ] );
  
  free( bag->array );
  
  free( bag );
}

size_t bagGetElementSize( const Bag bag )
{
  assert( bag != NULL );
  
  return bag->elementSize;
}

int bagGetCapacityIncrement( const Bag bag )
{
  assert( bag != NULL );
  
  return bag->capacityIncrement;
}


void bagAdd( Bag bag, void *obj )
{
  assert( bag != NULL );
  assert( bag->elementSize == sizeof( void * ) );
    
  PreCond( bag_isValid( bag ) );

  ensureIncCapacity( bag );
    
  ((void **) bag->array)[ bag->noElements ] = obj;
  bag->noElements++;

  PostCond( bag_isValid( bag ) );
}

void bagAdd2( Bag bag, void *data )
{
  assert( bag != NULL );

  PreCond( bag_isValid( bag ) );
    
  ensureIncCapacity( bag );
    
  memcpy( ((char *) bag->array) + bag->noElements * bag->elementSize, data, 
	  bag->elementSize );
  bag->noElements++;

  PostCond( bag_isValid( bag ) );
}


/* make sure one more element will fit in the array, if it will not, then
   enlarge the array */
static void ensureIncCapacity( Bag bag )
{
  assert( bag != NULL );
  
  if( bag->noElements == bag->capacity )
    {
      if( bag->capacityIncrement == 0 )
	bag->capacity = (bag->capacity == 0) ? 1 : (bag->capacity * 2); /* erl */
      else
	bag->capacity += bag->capacityIncrement; /* mst & dad */
		
      assert( bag->capacity != 0 );
		
      bag->array = realloc( bag->array, bag->capacity * bag->elementSize );
      assert( bag->array != NULL );
    }
}

void bagRemove( Bag bag, void *obj )
{
  int res;
	
  res = bagRemoveMaybe( bag, obj );
	
  assert( res == 1 );
}

void bagRemove2( Bag bag, void *data )
{
  int res;
	
  res = bagRemoveMaybe2( bag, data );
	
  assert( res == 1 );
}

int bagRemoveMaybe( Bag bag, void *obj )
{
  int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return 0;
  
  assert( bag->elementSize == sizeof( void * ) );
	
  for( i = 0; i < bag->noElements; i++ )
    if( ((void **) bag->array)[ i ] == obj )
      {
	((void **) bag->array)[ i ] = 
	  ((void **) bag->array)[ bag->noElements - 1 ];
	bag->noElements--;
	return 1;
      }
	
  return 0;
}



int bagRemoveMaybe2( Bag bag, void *data )
{
  int i;
  char *ptr;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return 0;
  
  for( i = 0, ptr = (char *) bag->array; i < bag->noElements; 
       i++, ptr += bag->elementSize )
    if( memcmp( data, ptr, bag->elementSize ) == 0 )
      {
	memcpy( ptr, 
		((char *) bag->array) + (bag->noElements - 1) * bag->elementSize,
		bag->elementSize );
	bag->noElements--;
	return 1;
      }
	
  return 0;
}

int bagNoElements( Bag bag )
{
  PreCond( bag == NULL || bag_isValid( bag ) );

  if( bag == NULL )
    return 0;
  
  return bag->noElements;
}

void **bagElements( Bag bag )
{
  PreCond( bag == NULL || bag_isValid( bag ) );

  if( bag == NULL )
    return NULL;
  
  assert( bag->elementSize == sizeof( void * ) );

  return bag->array;
}

void *bagElements2( Bag bag )
{
  PreCond( bag == NULL || bag_isValid( bag ) );

  if( bag == NULL )
    return NULL;
  else
    return bag->array;
}

Boolean bagContains( Bag bag, void *element )
{
  int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return FALSE;
	
  assert( bag->elementSize == sizeof( void * ) );
	
  for( i = 0; i < bag->noElements; i++ )
    if( ((void **) bag->array)[ i ] == element )
      return TRUE;
	
  return FALSE;
}

Boolean bagContainsCompare( Bag bag, void *element,
                            CompareFunc compareFunction )
{
	int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return FALSE;
	
  assert( bag->elementSize == sizeof( void * ) );
	
	for( i = 0; i < bag->noElements; i++ )
		if( compareFunction(((void **) bag->array)[ i ], element) == 0 )
			return TRUE;
	
	return FALSE;
}

Boolean bagContains2( Bag bag, void *element )
{
  int i;
  char *ptr;
  Boolean contains = FALSE;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag != NULL )
	
    for( i = 0, ptr = (char *) bag->array; i < bag->noElements; 
         i++, ptr += bag->elementSize )
      if( memcmp( ptr, element, bag->elementSize ) == 0 )
        contains = TRUE;

  PostCond( bag == NULL || bag_isValid( bag ) );
	
  return contains;
}

Bag bagFilter( Bag bag, FilterFunc filterFunc, void *filterArgs )
{
  Bag result;
  int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return NULL;
  
  result = bagCreate( 10, 10, bag->destroyFunc );
	
  assert( bag->elementSize == sizeof( void * ) );
	
  /* should protect this section with a semaphore */
  for( i = 0; i < bag->noElements; i++ )
    if( filterFunc( filterArgs, ((void **) bag->array)[ i ] ) )
      bagAdd( result, ((void **) bag->array)[i] );
	
  return result;
}

void bagFilter2( Bag bag, FilterFunc2 filterFunc, void *filterArgs )
{
  int i;
  char *ptr, *lastPtr;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return;
  
  bagLock( bag );
	
  /* it would be more efficient to loop through the bag backwards. In the 
     case that the bag is completely emptied, no memcpy's would be neccessary.
  */
  for( i = 0, ptr = (char *) bag->array, 
	 lastPtr = ptr + (bag->noElements - 1) * bag->elementSize; 
       i < bag->noElements; 
       )
    if( filterFunc( filterArgs, ptr ) )
      {
	i++;
	ptr += bag->elementSize;
      }
    else
      {
	memcpy( ptr, lastPtr, bag->elementSize );
	bag->noElements--;
	lastPtr -= bag->elementSize;
      }
	
  bagUnlock( bag ); 
}

void bagForEach( Bag bag, ForEachFunc forEachFunc, void *args )
{
  int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return;
  
  assert( bag->elementSize == sizeof( void * ) );
	
  /* the locks are to ensure that other threads don't add/remove elements from 
     the bag while we iterate over it. This could otherwise cause chaos.
  */
  bagLock( bag );
  
  /* should protect this section with a semaphore? */
  for( i = 0; i < bag->noElements; i++ )
    forEachFunc( args, ((void **) bag->array)[ i ] );
  
  bagUnlock( bag );
}

void bagForEach2( Bag bag, ForEachFunc2 forEachFunc, void *args )
{
  int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
	
  if( bag == NULL )
    return;
  
  bagLock( bag );
	
  /* should protect this section with a semaphore? */
  for( i = 0; i < bag->noElements; i++ )
    forEachFunc( args, ((char *) bag->array) + i * bag->elementSize );
	
  bagUnlock( bag );
}


void bagFilterForEach( Bag bag, FilterFunc filterFunc, void *filterArgs, 
                       ForEachFunc forEachFunc, void *args )
{
  int i;
	
  PreCond( bag == NULL || bag_isValid( bag ) );

  if( bag == NULL )
    return;
  
  assert( bag->elementSize == sizeof( void * ) );
	
  /* should protect this section with a semaphore? */
  for( i = 0; i < bag->noElements; i++ )
    if( filterFunc( filterArgs, ((void **) bag->array)[ i ] ) )
      forEachFunc( args, ((void **) bag->array)[ i ] );
}


Boolean bagUntil( Bag bag, FilterFunc filterFunc, void *funcArgs, 
                  void **element )
{
  int i;
  Boolean done = FALSE;

  PreCond( bag == NULL || bag_isValid( bag ) );

  if( bag == NULL )
    return FALSE;
  
  assert( bag->elementSize == sizeof( void * ) );
	
  /* should protect this section with a semaphore? */
  for( i = 0; !done && (i < bag->noElements); i++ )
    done = filterFunc( funcArgs, ((void **) bag->array)[ i ]);
  
  if( done && (element != NULL ) )
    {
      assert( i > 0 );
    
      *element = ((void **) bag->array)[ i - 1 ];
    }
  
  return done;
}

Boolean bagUntil2( Bag bag, FilterFunc2 filterFunc, void *funcArgs, 
                   void *element )
{
  int i;
  Boolean done = FALSE;

  PreCond( bag == NULL || bag_isValid( bag ) );
  
  if( bag == NULL )
    return FALSE;
  
  /* should protect this section with a semaphore? */
  for( i = 0; !done && (i < bag->noElements); i++ )
    done = filterFunc( funcArgs, ((char *) bag->array) + i * bag->elementSize);
  
  if( done && (element != NULL ) )
    {
      assert( i > 0 );
    
      memcpy( element, ((char *) bag->array) + (i - 1) * bag->elementSize,
	      bag->elementSize );
    }

  return done;
}

Boolean bagUntil2NoCopy( Bag bag, FilterFunc2 filterFunc, void *funcArgs, 
                         void **element )
{
  int i;

  PreCond( bag == NULL || bag_isValid( bag ) );
  
  if( bag == NULL ) return FALSE;
  
  /* should protect this section with a semaphore? */
  for( i = 0;  i < bag->noElements; i++ ) {
    void *ptr = ((char *) bag->array) + i * bag->elementSize;
    if( filterFunc( funcArgs, ptr ) ) {
      if (element != NULL) *element = ptr;
      return TRUE;
    }    
  }
  if (element != NULL) *element = NULL;
  return FALSE;
}



/* add all elements of the source bag to the result bag */
void bagAppend( Bag result, Bag source )
{
  PreCond( source == NULL || bag_isValid( source ) );

  if( source == NULL )
    return;
  
  assert( result != NULL );
  assert( result->elementSize == source->elementSize );
	
  bagForEach2( source, (ForEachFunc2) bagAdd2, result );
}

void *bagGetRandomElement( Bag bag )
{
  int elementNumber;
  
  assert( bag != NULL );
  assert( bag->elementSize == sizeof( void * ) );
  
  if( bag->noElements == 0 )
    return NULL;
  
  elementNumber = rand() % (bag->noElements);
  
  assert( elementNumber >= 0 );
  assert( elementNumber < bag->noElements );
  
  return ((void **) bag->array)[ elementNumber ];
}

void bagGetRandomElement2( const Bag bag, void *ptr )
{
  int elementNumber;
  
  assert( bag != NULL );
  
  if( bag->noElements == 0 )
    assert( FALSE );
  
  elementNumber = rand() % (bag->noElements);
  
  assert( elementNumber >= 0 );
  assert( elementNumber < bag->noElements );
  
  memcpy( ptr, ( (char *) bag->array) + elementNumber * bag->elementSize,
          bag->elementSize );
}

/*
 *  These functions are for those who are too lazy to use the
 *  fe_mumbo() functions that swap their arguments and are 
 *  impossible to read.
 *
 *  Do not use bagElementGet. 
 */

/* Cheapo version without the error 
   was bagGetTheIterator */
BagIterator bagIteratorCreate2(const Bag bag)
{
  BagIterator bi = malloc(sizeof(sBagIterator));

  PreCond( bag_isValid( bag ) );
  
  bi->current = 0;
  bi->bag = bag;
  
  return bi;
}

/* was bagIteratorGet */
Error bagIteratorCreate(const Bag bag, 
			BagIterator *bi)
{
  PreCond( bag_isValid( bag ) );

  *bi = malloc(sizeof(sBagIterator));
  
  (*bi)->current = 0;
  (*bi)->bag = bag;
    
  return NULL;
}

/* Cheapo version without the error */
void *bagIteratorNext(BagIterator bi)
{
  void *ptr;
  int i = bi->current;

  PreCond( bi != NULL );
  PreCond( bag_isValid( bi->bag ) );

  assert(bi->bag != NULL);
  
  ptr = malloc((bi->bag)->elementSize);

  memcpy( ptr, ( (char *) (bi->bag)->array) + i * (bi->bag)->elementSize,
          (bi->bag)->elementSize );
  
  bi->current++;
  
  return ptr;
}

/* relies on the caller to have allocated space for the element */
Error bagIteratorGetNext(BagIterator bi, void *ptr)
{
  int i = bi->current;

  PreCond( bi != NULL );
  PreCond( bag_isValid( bi->bag ) );

  assert( (bi->bag != NULL) );

  memcpy( ptr, ( (char *) (bi->bag)->array) + i * (bi->bag)->elementSize,
          (bi->bag)->elementSize );
    
  bi->current++;

  return NULL;
}

Boolean bagIteratorHasNext(BagIterator bi)
{
  PreCond( bi != NULL );
  PreCond( bag_isValid( bi->bag ) );

  assert(bi->bag != NULL);

  if (bi->current >= (bi->bag)->noElements || bi->current < 0)
    return FALSE;
  
  return TRUE;

}

/**
 * Destroys the bagIterator
 */
Error bagIteratorDestroy(BagIterator bi)
{
  if (bi == NULL)
    return NULL;

  free(bi);

  return NULL;
}

/**
 * Create a new bag that is the "cross product" of bag1 and bag2.  The
 * bags bag1 and bag2 must be bags of strings.
 * (Moved here from speech/compileGrammar.c).
 */
Error bagCrossProd( Bag bag1, Bag bag2, Bag *result )
{
  Error error;
  BagIterator bi;
  Bag bag;

  assert(result != NULL);

  bag = bagCreate(10,10,NULL);
  
  if( bagNoElements( bag1 ) == 0 ) {
    bagAppend( bag, bag2 );
    return bag;
  }

  for(bi = bagIteratorCreate2(bag1);  bagIteratorHasNext(bi);)  {
    char *str1;
    BagIterator bi2;
    if ((error = bagIteratorGetNext(bi, &str1)) != NULL)
      goto RETURN_ERROR;

    for(bi2 = bagIteratorCreate2(bag2);  bagIteratorHasNext(bi2);)  {
      char *str2;
      char buf[512];
      if ((bagIteratorGetNext(bi2, &str2)) != NULL)
        goto RETURN_ERROR;
    
      sprintf(buf,"%s %s", str1, str2);
      
      bagAdd(bag,strdup(buf));
    }
    free(bi2);
  }
  free(bi);

  *result = bag;
  return NULL;

 RETURN_ERROR:
  if (bag != NULL)
    bagDestroy(bag, free);
  return error;
}


