/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
  @file cvsid.h

  Defines a versioning macro.

  This file should be included in all .c files. 
  This is usually done by including voxi/alwaysInclude.h.

  The macro can be disabled by defining NDEBUG prior to the inclusion
  of cvsid.h

  $Id$
*/

#ifndef CVSID_H
#define CVSID_H


/**
  @def CVSID   Macro for putting a static CVS (RCS) id symbol in the object file.
*/

/* Note its self-reference, which should lower the likelihood of it
   being optimized away by a too-smart compiler, and also eliminate
   compile-time warnings about an "unreferenced variable". */
#ifndef CVSID
#ifndef NDEBUG
#define CVSID(msg) \
static const char * cvsid[] = {(const char*)cvsid, msg}
#else
#define CVSID(msg)
#endif /* NDEBUG */
#endif /* CVSID */

#endif /* CVSID_H */
