/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file voxi/types.h
 *
 * some basic type definitions
 */

#ifndef VOXI_TYPES_H
#define VOXI_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * the boolean value false
 */
#ifndef FALSE
#define FALSE 0
#endif

/**
 * the boolean value true
 */
#ifndef TRUE
#define TRUE !FALSE
#endif


/* Note: These should be somewhere else, but there is no good place at
   the moment. */

#ifndef MAX
/** max */
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
/** min */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/**
 * The boolean type
 */
typedef int Boolean;

#ifdef __cplusplus
}
#endif 

#endif
