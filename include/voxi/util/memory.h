/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  memory.h
  
  Voxi memory utilities
*/

typedef struct sMemoryManager *MemoryManager;

/*
  A memory manager in this implementation manages a fixed amount of memory 
  for fixed size blocks, in order to efficient from a threading perspective 
  and also fairly fast and somewhat error-checking.
*/
MemoryManager memManager_create( size_t elementSize, const char *name, 
                                 int capacity );
/*
  NOTE: the where string will not be duped - the pointer must not be free'd 
  by the caller and must be valid until the block is freed
*/
void *memManager_allocate( MemoryManager manager, const char *where );
void memManager_free( void *block );
void memManager_validate( void *block );



