/*
 * <voxi/util/stdint.h>
 *
 * This file attempts to provide the types from <stdint.h> in C99.
 *
 * This is for environments which don't yet support stdint.h, such as 
 * Microsoft's Visual C++ 6.0
 */

#ifndef VOXIUTIL_STDINT_H
#  define VOXIUTIL_STDINT_H

#  ifdef HAVE_STDINT_H
#    include <stdint.h>
#  else
#    include <limits.h>

#    if (SHRT_MAX == 32767)
       typedef short int16_t;
       typedef unsigned short uint16_t;
#    else
#      error Please configure a 16 bit type here.
#    endif
#  endif
#endif

