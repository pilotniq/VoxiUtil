/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file bag.h
 *
 * An unordered, resizable collection of items.
 *
 * @author bjorn
 * @author ara
 * @author erl
 * @author mst
 * @author jani
 *
 * Copyright (C) 1998-2002, Voxi AB
 *
 * A bag is an unordered, resizable collection which makes no guarantee as to the
 * oreder in which the elements are accessed.
 *
 * Access to the elements is possible through 1) the iterator, 2) for each functions
 * or 3) by getting the elements array, though this is not recommended since it gives
 * no guarantee what might happen if other threads modify the array whilst reading.
 *
 * @remarks The bag is not thread safe, the bagLock and bagUnlock functions of which
 * lacking implementation for performance reason. Therefore, you manually should lock
 * and unlock the bag with your own locks.
 * @remarks NULL is NOT a valid bag.
 */

#ifndef _BAG_H
#define _BAG_H

#include <stddef.h>
#include <voxi/types.h>
#include "collection.h"
#include "err.h"

/**
 * A filter function is used when you want to step through the elements of
 * the bag, removing some of them according to the implementation of your
 * own FilterFunction. The elements for which the filter function returns
 * true are kept, the rest is removed.
 */
typedef Boolean (*FilterFunc)( void *arguments, void *element );

/**
 * A FilterFunc2 takes a pointer to the data, rather than the value itself
 * as a parameter, allowing for non-void* values. 
 */
typedef Boolean (*FilterFunc2)( void *arguments, void *dataptr );

/**
 * forEachFunc2 takes a pointer to the element as its second parameter, rather
 * than the element itself.
 */
typedef void (*ForEachFunc2)( void *arguments, void *element );

/**
 * The definition of the Bag datatype.
 */
typedef struct sBag *Bag;

/**
 * A BagIterator is used to step through each element
 * in a bag.
 */
typedef struct sBagIterator *BagIterator;

/**
 * Checks whether the specified bag is valid according to the semantics.
 * A valid bag can be used for all the functions that take a bag as an argument.
 */
Boolean bag_isValid( Bag bag );

/**
 * Create a new bag of pointers with specified initial size.
 * This is the same as calling bagCreate2(int, int, sizeof(void *))
 *
 * @param initialCapacity The initial capacity for the bag. As long as
 * the number of elements does not grow above this number,
 * no increase of size is necassary.
 * @param capacityIncrement When maximum number of elements reached,
 * grow the bag by this no of elements.
 */
Bag bagCreate( int initialCapacity, int capacityIncrement, DestroyElementFunc destroyFunc);

/**
 * The same as for bagCreate, but each element in the bag being
 * of size elementSize.
 */
Bag bagCreate2( int initialCapacity, int capacityIncrement, size_t elementSize, DestroyElementFunc destroyFunc);

/**
 * Destroys and frees the bag. If destroyFunc is not NULL, the function is called
 * for each element in the bag, with the element as an argument to the destroyFunc.
 *
 * @remarks The bag is freed by bagDestroy
 * @remarks bagDestroy will accept a NULL bag, in which case it returns immediately.
 *
 * @param bag The bag to be destoryed and freed.
 * @param destroyFunc Could be NULL. If not NULL, this function is called for each element
 * in the bag.
 */
void bagDestroy( /*@only@*/Bag bag, /*@null@*/DestroyElementFunc destroyFunc );

/**
 * Copies the specified bag to a new bag. The new bag will
 * contain the same elements as the original one.
 *
 * @return A new bag containing the same elements as the original bag.
 */
Bag bagDuplicate( const Bag original );

/**
 * Get the element size of the specified bag.
 */
size_t bagGetElementSize( const Bag bag );

/**
 * Get the size by which the bag will grow each time it is full.
 */
int bagGetCapacityIncrement( const Bag bag );

/**
 * This function is not implemented.
 * @warning This function doesn't do anything.
 */
void bagLock( Bag bag );

/**
 * This function is not implemented.
 * @warning This function doesn't do anything.
 */
void bagUnlock( Bag bag );

/**
 * Adds the pointer to the bag.
 **/
void bagAdd( Bag bag, void *obj );

/**
 * Copies the data that data points to into the bag.
 */
void bagAdd2( Bag bag, void *data );

/* bagAdd3 REMOVED AS OF 1.2 */ 

/**
 * Adds all elements of the source bag to the result bag.
 */
void bagAppend( Bag result, Bag source );

/**
 * Removes the element obj from the bag if it is found.
 *
 * @param bag The bag to remove from.
 * @param obj The element to remove
 *
 * @return True if something was removed, else false.
 */
int bagRemoveMaybe( Bag bag, void *obj );

/**
 * Same as bagRemove but takes a pointer to the data.
 */
int bagRemoveMaybe2( Bag bag, void *data );

/**
 * Does a bagRemoveMaybe, followed by an assert that
 * something really was removed. Thus the program will
 * quit if assertion is enabled.
 */
void bagRemove( Bag bag, void *obj );


/**
 * Does a bagRemoveMaybe2, followed by an assert that
 * something really was removed. Thus the program will
 * quit if assertion is enabled.
 */
void bagRemove2( Bag bag, void *data );

/**
 * Get the number of elements currently in the specified bag.
 */
int bagNoElements( const Bag bag );

/**
 * Get the internal array containing the data.
 * This requires that the bagElementSize be sizof (void *).
 *
 * @remarks This is not the recommended way of accessing the data.
 * You should use the iterator or the for each functions if possible.
 */
void **bagElements( Bag bag );

/**
 * Get the internal array containing the data.
 *
 * @remarks This is not the recommended way of accessing the data.
 * You should use the iterator or the for each functions if possible.
 */ 
void *bagElements2( Bag bag );

/**
 * Checks whether the specified element is in the bag.
 */ 
Boolean bagContains( Bag bag, void *element );

/**
 * Checks whether the specified element is in the bag,
 * using the specified comare function.
 */
/**
 * Checks whether the specified element is in the bag.
 */ 
Boolean bagContainsCompare( Bag bag, void *element,
                            CompareFunc compareFunction);

/**
 * Same as bagContains but the element should be a pointer to
 * the data, rather than the data itself.
 */
Boolean bagContains2( Bag bag, void *element );

/**
 * Keeps bag contents for which the filterFunc returns TRUE.
 * @param bag The bag to step through.
 * @param filterFunc The function to call, that determines what
 * elements should be removed or kept.
 * @param filterArgs An argument to pass to the filterFunc.
 *
 * @return The bag with the elements removed.
 */
Bag bagFilter( Bag bag, FilterFunc filterFunc, /*@null@*/void *filterArgs );

/**
 * The same as bagFilter, but takes a FilterFunc2 for a filter function.
 * Does not create a new bag, simply removes the element from the existing bag.
 *
 * @warning The use of this function is currently not recommended, since
 * there is no destory function for the elements being removed.
 */
void bagFilter2( Bag bag, FilterFunc2 filterFunc, void *filterArgs );

/**
 * Steps through the elements of the bag, calling the specified ForEachFunc for every
 * element in the bag.
 *
 * @param bag The bag to step through.
 * @param forEachFunc The function to call for every element in the bag.
 * @param args An argument to pass to the ForEachFunc.
 */
void bagForEach( Bag bag, ForEachFunc forEachFunc, void *args );

/**
 * Same as bagForEach, the elements not nesassarily being sizeof (void *). 
 */
void bagForEach2( Bag bag, ForEachFunc2 forEachFunc, void *args );

/**
 * Calls the for each function with the elements for
 * which the filter function returns true.
 */
void bagFilterForEach( Bag bag, FilterFunc filterFunc, void *filterArgs, ForEachFunc forEachFunc, void *args );

/**
 * calls the filterfunc for the instances of the bag until it returns TRUE,
 * at which point the function returns TRUE.
 *   If the filterFunc does not return TRUE for any element of the bag, 
 * then return FALSE.
 *   if element is not NULL, it will be filled in with the element which
 * caused the function to return TRUE.
 *
 * The noCopy-Version returns a pointer to the object within the bag-s memory.
 *   -- a bit dangerous, but needed when changing attribute-values.  - ara 
 *
 * @warning The use of these functions should be avoided. Use the iterator instead.
 */
Boolean bagUntil( Bag bag, FilterFunc filterFunc, void *filterArgs, 
                  void **element );

/**
 * See documentation for bagUntil.
 * @see bagUntil
 */
Boolean bagUntil2( Bag bag, FilterFunc2 filterFunc, void *filterArgs,
                   void *element );

/**
 * See documentation for bagUntil.
 * @see bagUntil
 */
Boolean bagUntil2NoCopy( Bag bag, FilterFunc2 filterFunc, void *funcArgs, void **element );

/**
 * Get a random element from the specified bag.
 * @return A random element if the size of the bag
 * is > 0, else NULL is returned.
 */
void *bagGetRandomElement( const Bag bag );
/**
 * Same as bagGetRandomElement but copies the data into the ptr.
 * @see bagGetRandomElement
 */
void bagGetRandomElement2( const Bag bag, void *ptr );

/* 
   What are the sematics for a bagIterator? What happens if elements are 
   added/removed while the bag is being iterated over by another thread?
   
   /erl
*/

/**
 * Creates a new iterator for the specified bag.
 * This is the preferred way of stepping through the elements
 * of a bag.
 * @return A newly alocated bag iterator with position
 * on the first element of the bag.
 */
BagIterator bagIteratorCreate2(const Bag bag);

/**
 * Same as bagIteratorCreate2 but this functions returns an error.
 * @see bagGetTheIterator
 */
Error bagIteratorCreate(const Bag bag, BagIterator *bi);

/**
 * Get the next element of the iterator.
 * This function should only be called if you have checked that there
 * are more elements with bagIteratorHasNext.
 *
 * @see bagIteratorHasNext
 */
void *bagIteratorNext(BagIterator bi);

/**
 * Same as bagIteratorNext, but relies on caller to have allocated space for the ptr,
 * into which the data is copied.
 *
 * @see bagIteratorNext
 */
Error bagIteratorGetNext(BagIterator bi, /*@out@*/void *ptr);

/**
 * Checks whether there are more elements available for
 * the specified bag iterator.
 * This function should always be called before the bagIteratorNext or
 * bagIteratorGetNext is called.
 */
Boolean bagIteratorHasNext(BagIterator bi);

/**
 * Destorys the bagIterator
 * @see bagIteratorCreate
 */
Error bagIteratorDestroy(BagIterator bi);

/**
 * Create a new bag that is the "cross product" of bag1 and bag2,
 * where bag1 and bag2 must be bags of strings.
 *
 * The resulting bag is a bag of strings with bagNoElements(bag1) *
 * bagNoElements(bag2) elements. The resulting elements are defined by
 * concatenating each element from bag1 wich each element from bag2
 * once, putting a space in between.
 */
Error bagCrossProd(Bag bag1, Bag bag2, Bag *result);
     
#endif /* _BAG_H */


