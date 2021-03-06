/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  time.c
*/

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif

#include <voxi/alwaysInclude.h>

#include <voxi/util/time.h>

CVSID("$Id$");

unsigned long millisec()
{
  struct timeval tempTime;
  
#ifndef NDEBUG
  debug=0;
#endif
  
  gettimeofday( &tempTime, NULL );
  
  return (unsigned long) rint(tempTime.tv_sec * 1e3 + tempTime.tv_usec / 1000 );
}

unsigned long microsec()
{
  struct timeval tempTime;

#ifndef NDEBUG
  debug=0;
#endif
  
  gettimeofday( &tempTime, NULL );
  
  return (tempTime.tv_sec * 1000000 + tempTime.tv_usec ) & 0xffffffff;
}
