/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  path.c
  
  Some definitions and functions to simplify path handling.
*/

#include <stdio.h>
#include <stdlib.h>

#define DEBUG_MESSAGES 0
#include <voxi/alwaysInclude.h>
#include <voxi/util/err.h>
#include <voxi/util/vector.h>

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif

#include <voxi/util/path.h>

CVSID("$Id$");

Error path_parse(const char *path, Vector *pathVec)
{
  Error error = NULL;
  char *pathCopy, *nextDir, *nextDirCopy;

  if (path == NULL) {
    *pathVec = NULL;
    return NULL;
  }

  /* We do "double copying" of the path here - the first, temporary,
     copy is made to be able to use strsep (path_separate) without
     modifying the caller path string. The second copy for each
     element is done to be able to use the vector structure in
     conjunction with free to clean up each element separately. */
  
  if ((pathCopy = strdup(path)) == NULL) {
    error = ErrNew(ERR_ERRNO, errno, NULL,
                   "path_parse: strdup failed when creating temporary copy");
    goto ERR_BAIL_OUT;
  }

  *pathVec = vectorCreate("path vector", sizeof(char *), 1, 1,
                          (DestroyElementFunc) free);

  nextDir = pathCopy;
  while ((nextDir = path_separate(&pathCopy)) != NULL) {
    if ((nextDirCopy = strdup(nextDir)) == NULL) {
      error = ErrNew(ERR_ERRNO, errno, NULL,
                     "path_parse: strdup failed when copying path");
      goto ERR_BAIL_OUT;
    }
    DEBUG("Path: %s\n", nextDirCopy);
    vectorAppendPtr(*pathVec, nextDirCopy);
  }
  
 ERR_BAIL_OUT:
  if (pathCopy != NULL)
    free(pathCopy);
  return error;
}

char *path_separate(char **path)
{
  char delimStr[2] = {PATH_DELIM, '\0'};
  return strsep(path, delimStr);
}

char *path_concat_path(const char *path, const char *str)
{
  char *result;
  int resultLen = strlen(path) + 1 + strlen(str) + 1;
  char sep[2];
  result = (char*)malloc(resultLen);
  snprintf(result, resultLen, "%s%c%s", path, PATH_DELIM, str);
  return result;
}

char *path_concat_dir(const char *dir, const char *str)
{
  char *result;
  int resultLen = strlen(dir) + 1 + strlen(str) + 1;
  char sep[2];
  result = (char*)malloc(resultLen);
  snprintf(result, resultLen, "%s%c%s", dir, DIR_DELIM, str);
  return result;
}

FILE *path_fopen(const char *path, const char *name, const char *mode)
{
  Error err = NULL;
  Vector pathVec = NULL;
  FILE *f = NULL;
  int i;
  char *dir, *fullName;

  if ((path == NULL) || (name == NULL))
    goto ERR_RETURN;

  err = path_parse(path, &pathVec);
  if (err != NULL) {
#ifndef NDEBUG
    ErrReport(err);
#endif
    ErrDispose(err, TRUE);
    goto ERR_RETURN;
  }

  for (i = 0; (i < vectorGetElementCount(pathVec)) && (f == NULL); i++) {
    vectorGetElementAt(pathVec, i, &dir);
    fullName = path_concat_dir(dir, name);
    f = fopen(fullName, mode);
    free(fullName);
  }

  if (f != NULL)
    DEBUG("Found %s in %s\n", name, dir);
  else
    DEBUG("Could not find %s in %s\n", name, path);

 ERR_RETURN:
  if (pathVec != NULL)
    free(pathVec);
  return f;

}
