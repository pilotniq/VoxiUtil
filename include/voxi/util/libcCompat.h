#ifndef LIBCCOMPAT_H
#define LIBCCOMPAT_H

/*
  C library (in)compatibility stuff
*/

/* We implement this, since it is better than strtok (doesn't modify
   the actual string data, only the pointer). See Linux man pages
   strsep(3) and strtok(3) for further info.

   The strsep in Linux (as of RedHat 6.2) behaves slightly different
   from what is said in the man pages: When no delimiters are found,
   it returns a pointer to the string while the man page sais it
   should return NULL. To mimick this (mis)behaviour, define the
   STRSEP_LINUX_BEHAVIOUR macro below when compiling win32_glue.c.
*/

#if !defined( HAVE_STRSEP) || !HAVE_STRSEP

#define STRSEP_LINUX_BEHAVIOUR
/** strsep(3) */
char *strsep(char **stringp, const char *delim);

#endif

#endif /* ifndef LIBCCOMPAT_H */
