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

#include <stdio.h>
#include <voxi/util/bitFippling.h>


/*
  Debug functions that print out integers as binary, decimal and hex.
 */
#ifdef WIN32
void printbinary(__int64 value) {
int i;

  for (i=63;i>-1;i-- )
    printf("%c",(value & (((__int64) 1)<<i)) ? '1' : '0');
  printf(" %Li %Lx\n",value,value);
}

#else
void printbinary(long long int value) {
  int i;

  for (i=63;i>-1;i-- )
    printf("%c",(value & (((long long) 1)<<i)) ? '1' : '0');
  printf(" %Li %Lx\n",value,value);
}
#endif  //WIN32

void printbinarylong(long int value)
{
  int i;

  for (i=31;i>-1;i-- )
#ifdef WIN32
    printf("%c",(value & (((__int64) 1)<<i)) ? '1' : '0');
#else
  printf("%c",(value & (((long long) 1)<<i)) ? '1' : '0');
#endif
  printf(" %li %lx\n",value,value);

}

/*
  Change endianess of a long int
 */
long int drokk(long int i)
{
  return ((i & 0xff000000) >> 24) |
    ((i & 0x00ff0000) >> 8) |
    ((i & 0x0000ff00) << 8) |
    ((i & 0x000000ff) << 24);
}


/*
  Change endianess of a short
 */
short tanj(short i)
{
  return ((i & 0xff00) >> 8) |
    ((i & 0x00ff) << 8);
}


/*
  Change endianess of a float
 */
float smeg(float f) 
{
  long int *i;
  long int j;

  i = (long int *)&f;
  
  j = *i;
  
  j = ((j & 0xff000000) >> 24) |
    ((j & 0x00ff0000) >> 8) |
    ((j & 0x0000ff00) << 8) |
    ((j & 0x000000ff) << 24);
  
  i = &j;

  return *((float *)i);
}
