/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	file.c
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <voxi/alwaysInclude.h>
#include <voxi/util/err.h>
#include <voxi/util/file.h>

CVSID("$Id$");

/* reads a line, skipping lines beginning with '#' (comments) */
/* can return an errno */
int file_readLine( FILE *file, char *buffer, int bufSize )
{
  char *tempPtr;
  
  DEBUG("enter\n");

	do
	{
		tempPtr = fgets( buffer, bufSize, file );
    if( tempPtr == NULL )
      return errno;
	}
	while( !feof( file ) && (buffer[0] == '#') );
	
	/* remove any newline characters at the end of line */
  
  if( tempPtr != NULL )
  {
    size_t lastIndex;
    
    lastIndex = strlen( buffer ) - 1;
    
    while( (buffer[ lastIndex ] == '\n') || (buffer[ lastIndex ] == '\r') )
      lastIndex--;

    buffer[ lastIndex + 1 ] = '\0';
  }
#if 0
    /* if( !feof( file ) ) */
  buffer[ strlen( buffer ) - 1 ] = '\0';
#endif

  DEBUG("leave\n");

  return 0;
}

/* Reads byteCountes bytes formatted as hexadecimal from the file.
   
   The function will read twice as many characters from the file as byteCount
   (because each byte becomes two characters).
   
*/
   
Error file_readHexAsBinary( FILE *file, unsigned char *buffer, 
                            size_t byteCount )
{
  char buf[ 3 ];
  size_t readCount;
  
  buf[ 2 ] = '\0';
  
  for( ; byteCount > 0; )
  {
    unsigned long byteValue;
    
    /* read one byte at a time */
    readCount = fread( buf, 2, 1, file );
    assert( readCount == 1 );
    
    byteValue = strtoul( buf, NULL, 16 );
    assert( byteValue < 256 );
    
    *buffer = (unsigned char) byteValue;
    buffer++;
    byteCount--;
  }
  
  return NULL;
}

Error file_fopen( const char *fileName, const char *mode, FILE **file )
{
  Error error;
  
  (*file) = fopen( fileName, mode );
  if( (*file) == NULL )
    error = ErrNew( ERR_UNKNOWN, 0, ErrErrno(), 
                    "fopen( \"%s\", \"%s\" ) failed.", fileName, mode );
  else
    error = NULL;
  
  return error;
}

