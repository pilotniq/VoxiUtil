/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	idTable.h
	
	id-tables are collections where each element has an id which will never 
	change.
	
	ids may be reused after an element has been removed from the collection
	
	referencing, deleting are always very fast, and so is adding, unless the 
	array must be enlarged. However, the array will never shrink in size.
*/

#ifndef IDTABLE_H
#define IDTABLE_H

#include <stdio.h>
#include "collection.h"

typedef struct sIDTable *IDTable;

/* this function writes an element to a stream. */
typedef void (*IDTElementWriteFunc)( FILE *stream, int version, void *element,
																		 void *param );
/* this function reads an object from the stream, and adds it to <table> at
	 the index <number>. */
typedef void (*IDTElementReadFunc)( FILE *stream, IDTable table, int version, 
																		int number, void *param );

/* idt_create creates a table of (void *) */
IDTable idt_create( int initialCount );
IDTable idt_create2( int initialCount, size_t elementSize );

int idt_getElementCount( IDTable table );

/* 
   add adds the pointer 'element' to the idTable.
   add2 copies the element pointed to by pointer into a new slot in the idTable
*/
int idt_add( IDTable, void *element );
int idt_add2( IDTable table, void *pointer );

void idt_remove( IDTable, int index );

void *idt_getElementAt( IDTable, int index );
void idt_getElementAt2( IDTable idt, int index, void *data );

void idt_putElementAt( IDTable table, int index, void *element );

int idt_isElementAllocated( IDTable, int index );
int idt_getSize( IDTable table );

/* reading - restoring ID Tables and their elements from streams */

void idt_save( IDTable table, FILE *stream, const char *cookie, int version,
							 IDTElementWriteFunc, void *writeFuncParam );

/* create a new IDTable with the data in the file */
IDTable idt_load( FILE *stream, const char *cookie, 
									IDTElementReadFunc readFunc, void *readFuncParam );

/*
	create the IDTable if *result == NULL, otherwise append to the 
	given table.
*/
void idt_load2( IDTable *table, FILE *stream, const char *cookie, 
									IDTElementReadFunc readFunc, void *readFuncParam );

void idt_forEach( IDTable table, ForEachFunc func, void *funcArgs );

#endif

