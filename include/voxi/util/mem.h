/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file mem.h
 *
 * A Wrapper for malloc. 
 *
 * @author erl
 *
 * $Id$
 */

/*
 Since malloc2, free2 are not implemented and 
 malloch, realloch and freeh are not used in the 
 code i dont think we should add them to the public 
 documentation. -- ara 
*/

#include <voxi/types.h>
#include <voxi/util/err.h>

typedef void **Handle;
typedef enum { MEMERR_OUT } MemoryErrorType;

void *malloc2(unsigned int size);
void free2(void *ptr);

Boolean malloch(unsigned int size, Handle *result);
Boolean realloch(Handle h, int size, Boolean ignoreErr);
Boolean freeh(Handle h);

/**
 * Wrapper around POSIX-malloc.
 * The function returns a voxi-Error if the malloc failed.
 */
Error emalloc( void **result, size_t size );



