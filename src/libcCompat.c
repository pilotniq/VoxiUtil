/*
  C library (in)compatibility stuff
*/

#include <stdlib.h> /* for NULL */
#include <string.h>

#include <voxi/util/libcCompat.h>

#if !defined( HAVE_STRSEP ) || !HAVE_STRSEP

char *strsep(char **stringp, const char *delim)
{
  char *token_ptr = NULL;

  /* Just a sanity check... */
  if (stringp == NULL)
    return NULL;
  
  token_ptr = *stringp;
  if (*stringp != NULL) {
    char *d = strpbrk(*stringp, delim);
    if (d == NULL) {
      *stringp = NULL;
    } else {
      *d = '\0';
      *stringp = d + 1;
    }
  }
  
  return token_ptr;
}

#endif
