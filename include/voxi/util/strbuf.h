/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
 * string.h
 *
 * Voxi's string functions.
 *
 * Note that string buffers are not multithreading-safe; if multiple threads
 * operate on the same string buffer simultaneously, then an external 
 * mechanism must be used to prevent garbage.
 */

#ifndef VOXI_UTIL_STRING_H
#define VOXI_UTIL_STRING_H

#include <stdarg.h>
#include <stddef.h> 

enum { STRBUF_ERR_OTHER, STRBUF_ERR_UNKNOWN_FORMAT, 
       STRBUF_ERR_ENLARGE_FAILED };

typedef struct sStringBuffer *StringBuffer;

#include <voxi/util/err.h>

Error strbuf_create( size_t length, StringBuffer *result );
void strbuf_destroy( StringBuffer strbuf );
char *strbuf_destroyKeepString( StringBuffer strbuf, size_t *length );

Error strbuf_sprintf( StringBuffer strbuf, const char *format, ... );

/*
 * This function takes any of the characters in the quoteChars string which
 * appear in any %s arguments and quote them by inserting a '%' character 
 * and the hex code of the characters.
 */
Error strbuf_sprintfQuoteSubstrings( StringBuffer strbuf, 
                                     const char *quoteChars,
                                     const char *format, 
                                     ... );
Error strbuf_sprintfQuoteSubstrings2( StringBuffer strbuf, 
                                      const char *quoteString,
                                      const char *format, va_list args );

Error strbuf_appendBinary( StringBuffer strbuf, size_t length, 
                           const void *pointer );

const char *strbuf_getString( StringBuffer strbuf );
char *strbuf_getStringCopy( StringBuffer strbuf );

#endif
