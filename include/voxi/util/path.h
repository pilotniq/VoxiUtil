/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Some definitions and functions to simplify path handling.

  $Id$
*/

#ifndef PATH_H
#define PATH_H

#include <voxi/util/err.h>
#include <voxi/util/vector.h>

#ifdef WIN32
#  include "win32_glue.h"
#  define PATH_DELIM ';'
#  define DIR_DELIM '\\'
#else  /* Linux/Unix */
#  define PATH_DELIM ':'
#  define DIR_DELIM '/'
#endif

/* A specialized version of strsep(3): Returns the next directory in
   the given path as a null-terminated string. Will modify the path
   string. See further the strsep(3) man page. */
char *path_separate(char **path);

/* Parses a path according to the path policy of the system being used.
   The directories listed in the path will be returned in pathVec,
   as a new vector of strings. (These strings will be freed when the
   vector is destroyed). */
Error path_parse(const char *path, Vector *pathVec);

/* Concatenates str to path with an OS-dependent path delimiter in
   between: ':' on Unix, ';' on Win32. Parameters are untouched, the
   result is returned and must be freed by the caller. */
char *path_concat_path(const char *path, const char *str);

/* Concatenates str to dir with an OS-dependent directory delimiter in
   between: '/' on Unix, '\' on Win32. Parameters are untouched, the
   result is returned and must be freed by the caller. */
char *path_concat_dir(const char *dir, const char *str);

/* Searches for a file with the given name in the given path, and
   opens the first one found, using fopen, with the given mode. The
   directories in the path are searched from left to right. Returns
   NULL if no such file was found */
FILE *path_fopen(const char *path, const char *name, const char *mode);

#endif /* PATH_H */
