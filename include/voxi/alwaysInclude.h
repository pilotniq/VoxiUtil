/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
  @file alwaysInclude.h

  Includes some macros that are needed in every file.

  This include file is included by all .c files.
  It should only contain macros etc that really are needed _everywhere_,
  a pretty rare condition.

  Usually you will want to #define DEBUG_MESSAGES to 1 or 0 before including
  this file. This enables/disables debug-message printouts. 
  See voxi/debug.h for more info.

 * @author mst
  
  $Id$
*/

#ifndef ALWAYSINCLUDE_H
#define ALWAYSINCLUDE_H

#include "cvsid.h"
#include "debug.h"

#endif /* ALWAYSINCLUDE_H */
