/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  time.h
*/

#ifdef __cplusplus
extern "C" {
#endif 

#include <voxi/util/libcCompat.h>

/* get the number of ms since some unspecified time */
EXTERN_UTIL unsigned long millisec();
EXTERN_UTIL unsigned long microsec();

#ifdef __cplusplus
}
#endif

