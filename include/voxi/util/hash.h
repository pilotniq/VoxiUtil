/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
   General hashing routines.     /Mårten Stenius
   
   Migrated from The Great Hemul Project to erlib to Voxi, becoming a true
   part of Voxi's ISI on February 1, 2000.
   
   $Id$
*/

#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <voxi/util/libcCompat.h>

typedef int (*HashFuncPtr)(void *data);
#ifndef _COMPFUNCPTR
#  define _COMPFUNCPTR
   typedef int (*CompFuncPtr)(void *data1, void *data2);
#endif
typedef void (*DestroyFuncPtr)(void *data);

typedef struct Hasht *HashTable;
typedef struct sHashTableCursor *HashTableCursor;

#include <voxi/types.h>

/* Returns a hash table of specified size. For a closer explanation of the
   function arguments, see the declaration of HashTable in hash.c.
   If the creation of the table fails, the return value is NULL. */

EXTERN_UTIL HashTable HashCreateTable( unsigned int size, HashFuncPtr calc,
                                       CompFuncPtr comp, DestroyFuncPtr destr);

/*
  Set the debugging level for the hash table. 0 = no debugging.
  Returns the previous debugging level.
  
  Debug levels: 
    0 = no debugging output
    1 = exported functions called and their results
    2 = function entry and exits, internal (static) function entries and exits
    3 = function internal details 
    
*/
EXTERN_UTIL int HashSetDebug( HashTable ht, int debugLevel );

/* Frees all memory occupied by the hash table ht and its records. */
EXTERN_UTIL void HashDestroyTable(HashTable ht);

/* return the number of elements in the hash table */
EXTERN_UTIL int HashGetElementCount( HashTable ht );

/* If a data record matching 'data' is found in ht, HashFind returns a pointer
   to it -- if no match can be found, it returns NULL. */
EXTERN_UTIL /*@observer@*/void *HashFind(HashTable ht, void *data);

/* Removes the record containing 'data' from ht. If no such record exists,
   it returns 0. On success 1 is returned. */
EXTERN_UTIL int HashDelete(HashTable ht, void *data);

/* Removes the record containing 'data' from ht and destroys data.
   If no such record exists, it returns 0. On success 1 is returned. */
EXTERN_UTIL int HashDestroy(HashTable ht, void *data);

/* Adds data to the specified hashtable. If an equivalent record (according to
   ht->compData() ) exists, no adding is done.
   Returns 1 on success, 0 on failure. */
EXTERN_UTIL int HashAdd(HashTable ht, /*@only@*/void *data);
/*perhaps @keep@ would be sufficient*/

/*
   functions for traversing (enumerating) the entries in the hashtable added
   by erl 981015.

   cursors are not expected to be used by more than one thread at a time.
*/
/*
   create a cursor for traversing the hash table 
*/
EXTERN_UTIL HashTableCursor HashCursorCreate( HashTable ht );

EXTERN_UTIL void HashCursorDestroy( /*@special@*/HashTableCursor cursor )/*@releases cursor@*/;

/* return the element that the cursor is pointing to */
EXTERN_UTIL /*@observer@*/void *HashCursorGetElement( HashTableCursor cursor );

/* Move the cursor to the first element in the hashtable (according to some
   undefined, arbitrary order).
*/
EXTERN_UTIL void HashCursorGoFirst( HashTableCursor );

/* Move the cursor to the next element in the hash table */
EXTERN_UTIL void HashCursorGoNext( HashTableCursor );

/* return TRUE if the cursor has passed the last element */
EXTERN_UTIL Boolean HashCursorPastLastElement( HashTableCursor );

/*
 * Utility hashing funcions 
 */

/* calculates a hash value for a string */
EXTERN_UTIL int HashString( const char *string );
EXTERN_UTIL int HashLowercaseString( const char *string );

#ifdef __cplusplus
}
#endif


#endif
