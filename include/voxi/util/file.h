/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	file.h
*/

#include <stdio.h>

#include <voxi/util/err.h>

/*
  Does a fopen, but with Voxi error handling
*/
Error file_fopen( const char *fileName, const char *mode, FILE **file );

/* reads a line, skipping lines beginning with '#' (comments) 
   returns 0 or an errno 
*/

int file_readLine( FILE *file, char *buffer, int bufSize );
Error file_readHexAsBinary( FILE *file, unsigned char *buffer, 
                            size_t byteCount );
