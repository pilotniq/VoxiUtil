/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  win32_glue.c
  
  Miscellaneous Unix/Linux functions needed by Voxi implemented using
  the Win32 API.

  $Id$
*/

#ifdef WIN32

#include <windows.h>
#include <io.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include <voxi/alwaysInclude.h>

#include <voxi/util/win32_glue.h>

CVSID("$Id$");

double rint(double x)
{
  return floor(x + 0.5);
}

int gettimeofday(struct timeval *tv, void *tz)
{
  struct _timeb tbuf;
  _ftime(&tbuf);
  tv->tv_sec = tbuf.time;
  tv->tv_usec = tbuf.millitm * 1000;
  return 0;
}

void sleep(unsigned int sec)
{
  Sleep(sec * 1000);
}

void usleep(unsigned long usec)
{
  Sleep(usec / 1000);
}


/* Memory-mapped files. This is ALMOST like mmap, but takes a Win32 file
   handle instead of an osf handle (which are tricky to pass between
   DLL:s. */
void *mmap_win32(void *start, size_t length, int prot,
                 int flags, HANDLE hFile, off_t offset)
{
  HANDLE hMapping;
  LPVOID vMapping;
  DWORD flProtect = PAGE_READONLY, mProtect = FILE_MAP_READ;

  if (hFile == -1) {
    fprintf(stderr, "win32_glue.c: mmap: Bad file handle\n");
    return MAP_FAILED;
  }
  
  if (prot & PROT_NONE) {
    fprintf(stderr, "win32_glue.c: mmap: PROT_NONE not supported.\n");
    return MAP_FAILED;
  }
  if (flags & MAP_FIXED) {
    fprintf(stderr, "win32_glue.c: mmap: MAP_FIXED not supported.\n");
    return MAP_FAILED;
  }

  /* Win32 protection type */
  if (prot & PROT_READ) {
    flProtect = PAGE_READONLY;
    mProtect = FILE_MAP_READ;
  }
  if (prot & PROT_WRITE) {
    flProtect = PAGE_READWRITE;
    mProtect = FILE_MAP_WRITE;
  }
  if (flags & MAP_PRIVATE) {
    flProtect = PAGE_WRITECOPY;
    mProtect = FILE_MAP_COPY;
  }

  /* Win32 section attributes */
  if (prot & PROT_EXEC)
    flProtect |= SEC_IMAGE;

  hMapping = CreateFileMapping(hFile, NULL, flProtect, 0, 0, NULL);
  if (!hMapping) {
    fprintf(stderr, "win32_glue.c: mmap: CreateFileMapping: error %d.\n",
            GetLastError());
    return MAP_FAILED;
  }

  vMapping = MapViewOfFile(hMapping, mProtect, 0, 0, 0);
  if (!vMapping) {
    fprintf(stderr, "win32_glue.c: mmap: MapViewOfFile: error %d.\n",
            GetLastError());
    return MAP_FAILED;
  }
  
  return vMapping;
}

int munmap(void *start, size_t length)
{
  /* NOTE: We should probably also do a CloseHandle() on the mapping
     created in mmap. */
  if (UnmapViewOfFile(start) == 0)
    return -1;
  return 0;
}

#endif /* WIN32 */
