/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Shared Library functions
*/

#include <voxi/util/err.h>

typedef void *SharedLibrary;

Error shlib_open( const char *filename, SharedLibrary *shlib );
Error shlib_close( SharedLibrary shlib );
Error shlib_findFunc( SharedLibrary shlib, const char *name, void **funcPtr );

