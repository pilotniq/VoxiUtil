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
#include <voxi/util/libcCompat.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  Does a fopen, but with Voxi error handling
*/
EXTERN_UTIL Error file_fopen( const char *fileName, const char *mode, FILE **file );

/* reads a line, skipping lines beginning with '#' (comments) 
   returns 0 or an errno 
*/

EXTERN_UTIL int file_readLine( FILE *file, char *buffer, int bufSize );
EXTERN_UTIL Error file_readHexAsBinary( FILE *file, unsigned char *buffer, 
                            size_t byteCount );

#ifdef __cplusplus
}
#endif

