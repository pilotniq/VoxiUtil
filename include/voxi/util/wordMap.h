/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  wordMap.h - maintains a map of word to numbers 
*/

#ifndef WORDMAP_H
#define WORDMAP_H

#include <stdio.h>
#include <voxi/util/err.h>
#include <voxi/util/libcCompat.h>

#include <voxi/util/config.h>



#ifdef __cplusplus

extern "C" {

#endif 


#define WORDMAPMASK_BYNAME 1
#define WORDMAPMASK_BYNUMBER 2

typedef enum { WORDMAP_BYNAME_NUMBER = 3, WORDMAP_BYNAME = 1,
               WORDMAP_BYNUMBER = 2} WordMapType;

typedef struct sWordMap *WordMap;
typedef struct sWordMapCursor *WordMapCursor;

/*
 *  Functions
 */
EXTERN_UTIL Error wordMap_create( const char *name, WordMapType type, 
                                  WordMap *wordMap );
EXTERN_UTIL Error wordMap_create2( const char *name, WordMapType type, 
                                   WordMap *phoneMap, int hashTableSize );

EXTERN_UTIL int wordMap_getNoElements(WordMap map);

#ifdef _FILE_IO

EXTERN_UTIL Error wordMap_save( WordMap pm, FILE *file );
EXTERN_UTIL Error wordMap_saveToStringBuffer( WordMap map, 
                                              StringBuffer *memBlock );

EXTERN_UTIL Error wordMap_load( FILE *file, WordMap *map );

#endif

EXTERN_UTIL Error wordMap_createFromString( const char *str[], 
                                            WordMap *wordMap);
  
EXTERN_UTIL Error wordMap_add( WordMap map, const char *name, int hmmNo );
EXTERN_UTIL Error wordMap_deleteByNumber( WordMap map, int number );
EXTERN_UTIL Error wordMap_deleteByName( WordMap map, const char *name );

/* returns -1 if entry not found */
EXTERN_UTIL int wordMap_findByName( WordMap pm, const char *name );
EXTERN_UTIL /*@observer@*/const char *wordMap_findByNumber( WordMap pm, 
                                                            int number );

EXTERN_UTIL WordMapType wordMap_getType( WordMap map );
EXTERN_UTIL const char *wordMap_getName( WordMap map );


/*
 *  Cursor functions for traversing an entire word map.
 */
/** 
    create a cursor for traversing a word map.
    
    Note that the cursor must either traverse the entries by name or by number;
    if traversing by name, then only one entry for each name will be returned,
    if traversing by number, then only one entry with each number will be 
    returned. This is unfortunately due to the fact that all information in the
    map is stored in hash tables.
    
    The mask argument must be either WORDMAPMASK_BYNAME or 
    WORDMAPMASK_BYNUMBER.
*/
EXTERN_UTIL Error wordMap_cursor_create( WordMap map, int mask, 
                                         WordMapCursor *cursor );
EXTERN_UTIL void wordMap_cursor_destroy( WordMapCursor cursor );

EXTERN_UTIL int wordMap_cursor_getElementNumber( WordMapCursor cursor );
EXTERN_UTIL const char *wordMap_cursor_getElementName( WordMapCursor cursor );

EXTERN_UTIL void wordMap_cursor_goFirst( WordMapCursor cursor );
EXTERN_UTIL void wordMap_cursor_goNext( WordMapCursor cursor );
EXTERN_UTIL Boolean wordMap_cursor_pastLastElement( WordMapCursor cursor );

#ifdef __cplusplus
}
#endif 

#endif
