/**
 * @file circularBuffer.h
 * 
 * @author Erland Lewin <erl@voxi.com>
 * 
 * Semantics:
 * 
 * A circular buffer is a buffer of a limited length. If elements are written 
 * (pushed) to it when it is full, the oldest element is removed from the buffer
 * and destroyed (if a destroyFunc has been specified).
 * 
 * It was initially written for the push-to-talk / speech detection code.
 * 
 * @remarks The circularBuffer is not thread-safe; the application code must ensure
 * that locks are used if the buffer is to be used from different threads.
 */

/**
 * Copyright (C) 2002 Voxi AB. All rights reserved.
 *
 * This software is the proprietary information of Voxi AB, Stockholm, Sweden.
 * Use of this software is subject to license terms.
 *
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <voxi/util/err.h>
#include <voxi/util/collection.h>

typedef struct sCircularBuffer *CircularBuffer;

Error circBuf_create( int size, DestroyElementFunc destroyFunc,
                      CircularBuffer *result );

Error circBuf_destroy( CircularBuffer circBuf );

void circBuf_push( CircularBuffer circBuf, void *element );

/**
 *  Destroys all the elements in the circular buffer. The circular buffer
 *  will be empty after this call.
 */
void circBuf_clear( CircularBuffer circBuf );

#if 0 /* Not Implemented Yet */
Error circBuf_popHead( CircularBuffer circBuf, void **element );
#endif

/* Remove and return the last (tail, oldest) element in the buffer. 
   It is not destroyed. */
void *circBuf_popTail( CircularBuffer circBuf );
int circBuf_getElementCount( CircularBuffer circBuf );

#endif

