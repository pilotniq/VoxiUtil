/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file collection.h
 *
 * General definitions for collections.
 *
 * Copyright (C) 1999-2002, Voxi AB
 */

#ifndef COLLECTION_H
#define COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Used to step through the elements of a collection.
 * The for each function is called for every element in the
 * collection. You can supply an arguments parameter to pass to
 * the for each function.
 */
typedef void (*ForEachFunc)( void *arguments, void *element );

/**
 * The destroy function is called to destroy each element in the collection.
 */
typedef void (*DestroyElementFunc)( void *element );

/**
 * Used when comparing elements.
 * Returns 0 if equals, -1 if data1 is greater, +1 if data2 is greater.
 */
typedef int (*CompareFunc)( void *data1, void *data2 );

#ifdef __cplusplus
}
#endif

#endif
