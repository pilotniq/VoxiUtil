/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Erl's handle handling functions

  $Id$
  */

#include <assert.h>
#include <stdlib.h>

#include <voxi/alwaysInclude.h>
#include <voxi/types.h>
#include <voxi/util/err.h>
#include <voxi/util/mem.h>

CVSID("$Id$");

#if 0
/* replacements for standard malloc */

static int allocated = 0;

void *malloc2(unsigned int size)
{
  allocated+=size;
  return(malloc(size));
}

void free2(void *ptr)
{
  allocated
}
#endif

Boolean malloch(unsigned int size, Handle *result)
{

  DEBUG("enter\n");

  *result  = malloc(sizeof(void *));
  if(*result == NULL)
  {
    ERR ERR_ABORT, "malloch: Error when mallocating memory for handle" ENDERR;
    return(FALSE);
  }
  else
  {
    **result = malloc(size);
    if(**result == NULL)
    {
      ERR ERR_ABORT, "malloch: Error when allocating memory block" ENDERR;

      free(*result);
      return(FALSE);
    }
    else  return(TRUE);
  }

}

/*
  realloch:

  Reallocates a handle.
  If it fails, it returns false, but the handle is still valid, although the
  old size.
  */
/*------------------------------------------------*/
Boolean realloch(Handle h, int size, Boolean ignoreErr)
/*------------------------------------------------*/
{
  Handle temp;

  temp = realloc(*h, size);
  if(temp == NULL)
  {
    if(!ignoreErr)
      ERR ERR_ABORT, "realloch: realloc failed - requested size = %d\n", size 
        ENDERR;

    return(FALSE);
  }
  else
  {
    *h = temp;
    return(TRUE);
  }
}

Boolean freeh(Handle h)
{
  free(*h);
  free(h);

  return(TRUE);
}

Error emalloc( void **result, size_t size )
{
  *result = malloc( size );
  if( *result == NULL )
    return ErrNew( ERR_MEMORY, MEMERR_OUT, NULL, "Out of memory when "
                   "allocating %d bytes", size );
  else
    return NULL;
}

Error estrdup( const char *originalString, char **newString )
{
  assert( newString != NULL );
  assert( originalString != NULL );

  *newString = strdup( originalString );
  
  if( *newString == NULL )
    return ErrNew( ERR_MEMORY, MEMERR_OUT, NULL, "Out of memory when "
                   "allocating copying a string (%d bytes)", 
                   strlen( originalString ) + 1 );
  else
    return NULL;
}
