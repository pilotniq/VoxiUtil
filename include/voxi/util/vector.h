/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file vector.h
 * 
 * A vector is a dynamic, ordered, one-dimensional array.
 *
 * @author bjorn
 * @author ara
 * @author erl
 * @author mst
 * @author jani
 *
 * Copyright (C) 1999-2002, Voxi AB
 */

#ifndef VECTOR_H
#define VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * Definition of the Vector datatype.
 */
typedef struct sVector *Vector;
typedef const struct sVector *ConstVector;

#include <voxi/util/collection.h>
#include <voxi/types.h>
#include <voxi/util/libcCompat.h>

#ifndef NDEBUG
/* helperfunction for assert */
EXTERN_UTIL Boolean vector_isValid( Vector vector );
#endif

/**
 * Creates a new, empty vector.
 *
 * @param name A name for this vector, could be NULL.
 * @param elementSize The size of each element in the vector.
 * Typically, this could be sizeof(void *).
 * @param initialCapacity The size to which the vector can grow
 * before it must enlarge itself.
 * @param capacityIncrement When the vector is full, it will enlarge its
 * internal array by this amount.
 * @param destroyFunc A function to call upon destruction of elements. If NULL,
 * no destroyFunc is used.
 *
 * @return A new empty vector.
 */
EXTERN_UTIL Vector vectorCreate( const char *name, size_t elementSize,
                     int initialCapacity, int capacityIncrement,
                     /*@null@*/DestroyElementFunc destroyFunc );

/**
 * Turns the debug mode on or off for the specified vector.
 */
EXTERN_UTIL void vectorDebug( ConstVector vector, Boolean state );

/**
 * If you want to track the number of references to this vector, you
 * can use the ref count.
 *
 * The vector is initially created with a reference count of 1.
 * when the reference count is decreased to 0, the vector is destroyed.
 */
EXTERN_UTIL void vectorRefcountIncrease( Vector vector );

/**
 * See documentation for vectorRefcountIncrease.
 * @see vectorRefcountIncrease
 */
EXTERN_UTIL void vectorRefcountDecrease( Vector vector );

/**
 * Grabs the lock for the specified vector for the calling thread.
 * This function will block until any other thread that might
 * have grabbed the lock releases it with vectorUnlock.
 */
EXTERN_UTIL void vectorLock( Vector vector );

/**
 * Same as vectorLock, but in debug mode.
 * @see vectorLock
 */
EXTERN_UTIL const char *vectorLockDebug( Vector vector, const char *where );

/**
 * Same as vectorLock, but in debug mode.
 * @see vectorLock
 */
EXTERN_UTIL void vectorUnlockDebug( Vector vector, const char *oldLocker );

/**
 * Releases the lock for the specified vector for the calling thread,
 * thereby making it possible for other threads to grab the lock.
 */
EXTERN_UTIL void vectorUnlock( Vector vector );

/**
 * Returns the number of element currently in the specified bag.
 */
EXTERN_UTIL int vectorGetElementCount( ConstVector vector );

/**
 * Copies the element into the end of the vector. If the vector
 * is full, its capacity is incremented.
 * @return The index at which the element was inserted.
 */
EXTERN_UTIL int /*@alt void@*/vectorAppend(Vector vector, const void *element);

/**
 * The same as for vectorAppend, but it is assumed that the element is a
 * pointer and therefore the element is stored, not copied.
 * @see vectorAppend
 */
EXTERN_UTIL int /*@alt void@*/vectorAppendPtr( Vector vector, 
                                               /*@only@*/const void *ptr );

/**
 * Removes the last element from the specified vector.
 *
 * @pre The number of elements in the vector must be > 0.
 *
 * @warning The destory function is not called here.
 * You should free the elements manually.
 */
EXTERN_UTIL void vectorRemoveLastElement( Vector vector );

/**
 * Removes all the elements from the vector.
 * @post vector->elementCount == 0
 *
 * @warning The behaviour of this function has been changed. 
 *    Previously this function would assert( FALSE ) if no destory function 
 *    was defined. Now it will call the destroy function for each element if 
 *    there is a destroy func.
 */
EXTERN_UTIL void vectorRemoveAll( Vector vector );

/**
 * Copies the element at the specified index into the memory pointed at by 
 * element.
 *
 * @pre index < vector->elementCount
 * @remarks Caller frees the element.
 * @remarks vector is not const, because the vector is locked, which modifies 
 *          the vector.
 */
EXTERN_UTIL void vectorGetElementAt( Vector vector, int index, 
                                     /*@out@*/void *element );

/**
 * Copies the element from memory to the specified index.
 *
 * @pre index < vector->elementCount
 * @remarks Caller frees the element.
 */
EXTERN_UTIL void vectorSetElementAt( Vector vector, int index, 
                                     /*@out@*/void *element );

/**
 * Creates a new vector with the same elements as in the supplied vector.
 *
 * @param name The name to give the new vector
 * @param a The source vector to copy from
 */
EXTERN_UTIL Vector vectorClone( const char *name, Vector a );

#ifdef __cplusplus
}
#endif 

#endif






