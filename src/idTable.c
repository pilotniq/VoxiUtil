/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	idTable.c
*/
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <voxi/alwaysInclude.h>
#include <voxi/types.h>
#include <voxi/util/file.h>

#include <voxi/util/idTable.h>

CVSID("$Id$");

#define IDTABLE_FILE_VERSION 1

#define ELEMENT( idTable, index ) ((TableSlot) (( (char *) (idTable)->table) + (index) * (idTable)->slotSize))

typedef struct sTableSlot
{
	Boolean isFree;
	union
	{
		int nextFree;
		void *element;
	} data;
} sTableSlot, *TableSlot;

typedef struct sIDTable
{
	sTableSlot *table;
	int tableSize;
	int firstFree;
	int elementCount;
  size_t elementSize;
  size_t slotSize;
} sIDTable;

static void idt_enlarge( IDTable idt );

IDTable idt_create( int initialCount )
{
  return idt_create2( initialCount, sizeof( void * ) );
}

IDTable idt_create2( int initialCount, size_t elementSize )
{
	IDTable result;
	int i;
	
  DEBUG("enter\n");

	result = malloc( sizeof( sIDTable ));
	assert( result != NULL );
	
	result->elementSize = elementSize;
  result->slotSize = sizeof( sTableSlot ) + (elementSize >= sizeof( int ) ? elementSize : sizeof( int )) - sizeof( int );
  
	result->table = malloc( result->slotSize * initialCount );
	assert( result->table != NULL );
	
	result->tableSize = initialCount;
	result->elementCount = 0;
  
	for( i = 0; i < initialCount - 1; i++ )
	{
    TableSlot ts = ELEMENT( result, i );
    
		ts->isFree = TRUE;
		ts->data.nextFree = i + 1;
	}
	
	ELEMENT( result, initialCount - 1 )->isFree = TRUE;
	ELEMENT( result, initialCount - 1 )->data.nextFree = -1;
	
	result->firstFree = 0;
	
  DEBUG("leave\n");

	return result;
}

int idt_add( IDTable table, void *element )
{
	int index;
	
  assert( table->elementSize == sizeof( void *) );
  
	if( table->firstFree == -1 )
	{
		idt_enlarge( table );
		assert( table->firstFree != -1 );
	}
	
	index = table->firstFree;
	assert( ELEMENT( table, index )->isFree );
	
	table->firstFree = ELEMENT( table, index )->data.nextFree;
	
	ELEMENT( table, index )->isFree = FALSE;
	ELEMENT( table, index )->data.element = element;
	
	table->elementCount++;
	
	return index;
}

int idt_add2( IDTable table, void *element )
{
	int index;
	
	if( table->firstFree == -1 )
	{
		idt_enlarge( table );
		assert( table->firstFree != -1 );
	}
	
	index = table->firstFree;
	assert( ELEMENT( table, index )->isFree );
	
	table->firstFree = ELEMENT( table, index )->data.nextFree;
	
	ELEMENT( table, index )->isFree = FALSE;
  memcpy( &(ELEMENT( table, index )->data.element), element, table->elementSize );
	
	table->elementCount++;
	
	return index;
}

void *idt_getElementAt( IDTable idt, int index )
{
	assert( index < idt->tableSize );
  /*	assert( !idt->table[ index ].isFree ); this was what stod. is it wrong? -- ara */
  assert( ! ELEMENT( idt, index )->isFree );
	assert( idt->elementSize == sizeof( void *) );
  
	return ELEMENT( idt, index )->data.element;
}

void idt_getElementAt2( IDTable idt, int index, void *data )
{
	assert( index < idt->tableSize );
	assert( !idt->table[ index ].isFree );
  
  memcpy( data, ELEMENT( idt, index )->data.element, idt->elementSize );
}

void idt_remove( IDTable idt, int index )
{
	assert( index < idt->tableSize );
	assert( !idt->table[ index ].isFree );
	
	ELEMENT( idt, index )->isFree = TRUE;
	ELEMENT( idt, index )->data.nextFree = idt->firstFree;
	idt->firstFree = index;
	
	idt->elementCount--;
	assert( idt->elementCount >= 0 );
}

static void idt_enlarge( IDTable idt )
{
	int newSize, oldSize;
	int i;
	
	oldSize = idt->tableSize;
	newSize = oldSize * 2; 
	
	idt->table = realloc( idt->table, newSize * idt->slotSize );
	assert( idt->table != NULL );
	
	idt->tableSize = newSize;
	for( i = oldSize; i < newSize - 1; i++ )
	{
		ELEMENT( idt, i )->isFree = TRUE;
		ELEMENT( idt, i )->data.nextFree = i + 1;
	}
	ELEMENT( idt, newSize - 1 )->isFree = TRUE;
	ELEMENT( idt, newSize - 1 )->data.nextFree = idt->firstFree;
	
	idt->firstFree = oldSize;
}

void idt_putElementAt( IDTable table, int index, void *element )
{
	int prevFree;
	
  assert( table->elementSize == sizeof( void * ) );
  
	/* first see if the table must be enlarged */
	while( index >= table->tableSize )
		idt_enlarge( table );
	
	/* this is sort of tricky, because we must find the index in the free list
		 in order to add the element */
	
	/* special */
	if( table->firstFree == index )
	{
		assert( ELEMENT( table, table->firstFree )->isFree );
		
		table->firstFree = table->table[ index ].data.nextFree;
		ELEMENT( table, index )->isFree = FALSE;
		ELEMENT( table, index )->data.element = element;
		
		table->elementCount++;
		
		return;
	}
	
	for( prevFree = table->firstFree; 
			 (ELEMENT( table, prevFree )->data.nextFree != index) && 
				 (ELEMENT( table, prevFree )->data.nextFree != -1);
			 prevFree = ELEMENT( table, prevFree )->data.nextFree )
		;
	
#ifndef NDEBUG
	if( ELEMENT( table, prevFree )->data.nextFree == -1 )
	{
		fprintf( stderr, "ERROR: idTable: tried to put element %p at pos %d, "
						 "but it was occupied.\n", element, index );
		return;
	}
#endif
	
	if( prevFree == -1 )
		table->firstFree = -1;
	else
	{
		assert( ELEMENT( table, prevFree )->isFree );
		assert( ELEMENT( table, index )->isFree );
		
		ELEMENT( table, prevFree )->data.nextFree = 
			ELEMENT( table, index )->data.nextFree;
	
		ELEMENT( table, index )->isFree = FALSE;
		ELEMENT( table, index )->data.element = element;
		
		table->elementCount++;
	}
}

#define MAX_LINE_LENGTH 256

void idt_save( IDTable table, FILE *file, const char *cookie, int appVersion,
							 IDTElementWriteFunc writeFunc, void *writeParams )
{
	int actualCount = 0, i;
	
  assert( table->elementSize == sizeof( void * ) );
  
	fputs( "# cookie\n", file );
	fputs( cookie, file );
	fputs( "# application data format version\n", file );
	fprintf( file, "\n%d\n", appVersion );
	fputs( "# ID Table data format version\n", file );
	fprintf( file, "%d\n", IDTABLE_FILE_VERSION );
	fputs( "# Number of elements\n", file );
	fprintf( file, "%d\n", table->elementCount );
	fputs( "#\n# The elements\n#\n", file );
	/*
		write the elements
	*/
	for( i = 0; i < table->tableSize; i++ )
	{
		if( !table->table[ i ].isFree )
		{
			fputs( "# The element number\n", file );
			fprintf( file, "%d\n", i );
			fputs( "# application element data\n", file );
			writeFunc( file, appVersion, ELEMENT( table, i )->data.element, 
								 writeParams);
			
			actualCount++;
		}
	}
	
	fflush( file );
	
	assert( actualCount == table->elementCount );
}

IDTable idt_load( FILE *file, const char *cookie, 
									IDTElementReadFunc readFunc, void *readFuncParam )
{
	IDTable result = NULL;
	
	idt_load2( &result, file, cookie, readFunc, readFuncParam );
	
	return result;
}

/*
	create the IDTable if *result == NULL, otherwise append to the 
	given table.
*/
void idt_load2( IDTable *result, FILE *file, const char *cookie, 
									IDTElementReadFunc readFunc, void *readFuncParam )
{
	char buffer[ MAX_LINE_LENGTH ];
	int count, i, appVersion, idtVersion;
	
	/* first line contains the cookie "Objects" */
	file_readLine( file, buffer, MAX_LINE_LENGTH );
	
	assert( strcmp( buffer, cookie ) == 0 );
	assert( !feof( file ) );
	
	/* read the version of the application data format */
	file_readLine( file, buffer, MAX_LINE_LENGTH );
	appVersion = atoi( buffer );
	
	/* read the ID Table version */
	file_readLine( file, buffer, MAX_LINE_LENGTH );
	idtVersion = atoi( buffer );
	assert( idtVersion == 1 );
	
	/* read the number of objects in the file */
	file_readLine( file, buffer, MAX_LINE_LENGTH );
	count = atoi( buffer );
	
	if( *result == NULL )
		*result = idt_create( 2 * count );
	
	for( i = 0; i < count; i++ )
	{
		int index;
		
		/* read the id number of the object */
		file_readLine( file, buffer, MAX_LINE_LENGTH );
		index = atoi( buffer );
		
		readFunc( file, *result, appVersion, index, readFuncParam );
		/* idt_putElementAt( *result, index, element ); */
	}
}

void idt_forEach( IDTable table, ForEachFunc func, void *funcArgs )
{
	int i;
	
  assert( table->elementSize == sizeof( void * ) );
  
	for( i = 0; i < table->tableSize; i++ )
	{
		if( !ELEMENT( table, i )->isFree )
			func( funcArgs, ELEMENT( table, i )->data.element );
	}
}

Boolean idt_isElementAllocated( IDTable table, int index )
{
	if( index >= table->tableSize )
		return FALSE;
	
	return !ELEMENT( table, index )->isFree;
}

int idt_getSize( IDTable table )
{
	return table->tableSize;
}

int idt_getElementCount( IDTable table )
{
  return table->elementCount;
}
