/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  BitFippling - lets fippel some bits!

  31/1-2002 - jani
  Includes some useful functions that are quite low level.
  Like changing endianess, writing integers in binary etc. 
 
 */

#ifndef BITFIPPLING_H
#define BITFIPPLING_H

#include "config.h"


#if HAVE_LONG_LONG
#define VOXI_LONG_LONG long long
#elif HAVE___INT64
#define VOXI_LONG_LONG __int64
#else
#error "Need a 'long long' datatype or equivalent."
#endif

/*
  Debug functions that print out integers as binary, decimal and hex.
 */
void printbinary(VOXI_LONG_LONG value);

void printbinarylong(long int value);


/*
  Change endianess of a long int
 */
long int drokk(long int i);


/*
  Change endianess of a short
 */
short tanj(short i);


/*
  Change endianess of a float
 */
float smeg(float f);

#endif
