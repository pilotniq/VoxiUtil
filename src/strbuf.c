/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
 * strbuf.c
 *
 * Functions for dynamically resized, safe string buffers.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <voxi/alwaysInclude.h>
#include <voxi/util/err.h>

#include <voxi/util/strbuf.h>

CVSID("$Id$");

/* 
   The following structure has the following invariant:
   
   cursor - string = bufferSize - bufferRemaining
   
   The StringBuffer code has been modified to be able to store binary 
   information, which may contain '\0' bytes.
*/
typedef struct sStringBuffer 
{
  char *string;
  char *cursor; /* points to the '\0' at the end of the string */
  
  size_t bufferSize;
  size_t bufferRemaining;
} sStringBuffer;

/*
 * static function prototypes
 */
static Error strbuf_enlarge( StringBuffer strbuf, size_t minFreeSpace );

/*
  Length bytes are reserved for the string, plus one for the terminating \0. 
  The buffer will grow automatically when neccessary.
*/
Error strbuf_create( size_t length, StringBuffer *strbuf )
{
  DEBUG("enter\n");

  (*strbuf) = malloc( sizeof( sStringBuffer ) );
  assert( *strbuf != NULL );
  
  (*strbuf)->string = malloc( length + 1 );
  assert( (*strbuf)->string != NULL );
  
  (*strbuf)->string[0] = '\0';
  
  (*strbuf)->cursor = (*strbuf)->string;
  (*strbuf)->bufferSize = length;
  (*strbuf)->bufferRemaining = length;
  
  assert( ((*strbuf)->cursor - (*strbuf)->string) == 
          ((*strbuf)->bufferSize - (*strbuf)->bufferRemaining ) );

  DEBUG("leave\n");

  return NULL;
}

void strbuf_destroy( StringBuffer strbuf )
{
  free( strbuf->string );
  free( strbuf );
}

char *strbuf_destroyKeepString( StringBuffer strbuf, size_t *length )
{
  char *result;
  
  result = strbuf->string;
  if( length != NULL )
    *length = (strbuf->cursor - strbuf->string);
  
  free( strbuf );
  
  return result;
}


Error strbuf_sprintf( StringBuffer strbuf, const char *format, ... )
{
  va_list args;
  Error result;
  
  va_start( args, format );
  
  result = strbuf_sprintfQuoteSubstrings2( strbuf, NULL, format, args );
  
  va_end( args );
  
  return result;
}

Error strbuf_sprintfQuoteSubstrings( StringBuffer strbuf, 
                                     const char *quoteString,
                                     const char *format, ... )
{
  va_list args;
  Error result;
  
  va_start( args, format );
  
  result = strbuf_sprintfQuoteSubstrings2( strbuf, quoteString, format, 
                                            args );
  
  va_end( args );
  
  return result;
}

Error strbuf_sprintfQuoteSubstrings2( StringBuffer strbuf, 
                                      const char *quoteString,
                                      const char *format, va_list args )
{
  Error error = NULL;
  char buffer[ 16 ];
  int tempInt;
  size_t bufLen;
  const char *tempString;
  
  for( ; (*format != '\0') && (error == NULL); format++ )
  {
    switch( *format )
    {
      case '%':
        format++;
        switch( *format )
        {
          case 'd':
            tempInt = va_arg( args, int );
            
            sprintf( buffer, "%d", tempInt );
            
            /* resize the strbuf if neccessary */
            bufLen = strlen( buffer );
            
            error = strbuf_enlarge( strbuf, bufLen );
            
            if( error == NULL )
            {
              strcpy( strbuf->cursor, buffer );
              strbuf->cursor += bufLen;
              strbuf->bufferRemaining -= bufLen;
            }
            break;
            
          case 's':
            tempString = va_arg( args, const char * );
            
            if( quoteString == NULL )
            {
              /* resize the strbuf if neccessary */
              bufLen = strlen( tempString );
              
              error = strbuf_enlarge( strbuf, bufLen );
              
              if( error == NULL )
              {
                strcpy( strbuf->cursor, tempString );
                strbuf->cursor += bufLen;
                strbuf->bufferRemaining -= bufLen;
              }
            }
            else 
            {
              const char *inCharPtr;
              size_t remainToCopy;
              
              remainToCopy = strlen( tempString );
              
              strbuf_enlarge( strbuf, remainToCopy );
              
              for( inCharPtr = tempString; 
                   (*inCharPtr != '\0') && (error == NULL); 
                   inCharPtr++, remainToCopy-- )
              {
                if( (*inCharPtr == '%') || 
                    (strchr( quoteString, *inCharPtr ) != NULL ) )
                {
                  /* Quote the character */
                  if( strbuf->bufferRemaining < (remainToCopy + 2) )
                    error = strbuf_enlarge( strbuf, remainToCopy + 2 );
                  
                  sprintf( strbuf->cursor, "%%%2xc", *inCharPtr );
                  strbuf->cursor+=3;
                  strbuf->bufferRemaining-=3;
                }
                else
                {
                  *(strbuf->cursor) = *inCharPtr;
                  (strbuf->cursor)++;
                  strbuf->bufferRemaining --;
                }
              } /* end of for each input character, copy or quote it. */
            }
            
            break;
            
          default:
            error = ErrNew( ERR_STRBUF, STRBUF_ERR_UNKNOWN_FORMAT, NULL,
                            "Unknown format character %%%c.", *format );
        }
        break;
#if 0        
      case '\\':
        format++;
        switch( *format )
        {
          default:
            error = ErrNew( ERR_STRBUF, STRBUF_ERR_UNKNOWN_ESCAPE, NULL,
                            "Unknown escaped character \\%c.", *format );
        }
        break;
#endif
      default:
        if( strbuf->bufferRemaining == 0 )
          error = strbuf_enlarge( strbuf, 1 );
        
        assert( strbuf->bufferRemaining > 0 );
        
        if( error == NULL )
          *(strbuf->cursor) = *format;
        
        strbuf->cursor++;
        strbuf->bufferRemaining--;
        break;
    }
  }
  
  *(strbuf->cursor) = '\0';
  
  assert( (strbuf->cursor - strbuf->string) == 
          (strbuf->bufferSize - strbuf->bufferRemaining ) );
  
  return error;
}

const char *strbuf_getString( StringBuffer strbuf )
{
  return strbuf->string;
}

char *strbuf_getStringCopy( StringBuffer strbuf )
{
  return strdup( strbuf->string );
}

Error strbuf_appendBinary( StringBuffer strbuf, size_t length, 
                           const void *pointer )
{
  Error error;
  
  error = strbuf_enlarge( strbuf, length );
  if( error != NULL )
    return error;
  
  assert( strbuf->bufferRemaining >= length );
  
  memcpy( strbuf->cursor, pointer, length );
  strbuf->cursor += length;
  strbuf->bufferRemaining -= length;
  
  return NULL;
}

static Error strbuf_enlarge( StringBuffer strbuf, size_t minFreeSpace )
{
  Error error = NULL;
  
  if (strbuf->bufferRemaining <= minFreeSpace) {
    char *newPtr;
    size_t cursorOffset, newSize;
    size_t sizeIncrease;

    sizeIncrease = strbuf->bufferSize;
    if( sizeIncrease + strbuf->bufferRemaining < minFreeSpace )
      sizeIncrease = minFreeSpace;
    
    newSize = strbuf->bufferSize + sizeIncrease;
    
    newPtr = realloc( strbuf->string, newSize );
    
    if( newPtr == NULL )
      error = ErrNew( ERR_STRBUF, STRBUF_ERR_ENLARGE_FAILED, ErrErrno(),
                      "Failed to enlarge string buffer to %d bytes.",
                      newSize );
    
    if( error == NULL )
      {
        cursorOffset = strbuf->cursor - strbuf->string;
        
        strbuf->string = newPtr;
        strbuf->cursor = newPtr + cursorOffset;
        
        strbuf->bufferSize = newSize;
        strbuf->bufferRemaining += sizeIncrease;
      }
    
    assert( (strbuf->cursor - strbuf->string) == 
            (strbuf->bufferSize - strbuf->bufferRemaining ) );
  }
  
  return error;
}

Error strbuf_stringList_append( StringBuffer strbuf, const char *string )
{
  return strbuf_appendBinary( strbuf, strlen( string ) + 1, string );
}

Error strbuf_stringList_append2( StringBuffer strbuf, const char *format,
                                 ... )
{
  va_list args;
  Error error = NULL;
  char buf[ 1024 ];
  
  va_start( args, format );
  
  vsprintf( buf, format, args );
  
  error = strbuf_appendBinary( strbuf, strlen( buf ) + 1, buf );
  
  va_end( args );
  
  return error;
}


/* returns NULL if no more strings */
Error strbuf_stringList_popString( StringBuffer strbuf, int *cursor, 
                                   const char **string )
{
  *string = &(strbuf->string[ *cursor ]);
  
  if( *string == strbuf->cursor )
    *string = NULL; /* at last string */
  
  cursor += (strlen( *string ) + 1);
  
  return NULL;
}

