/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  wordMap.c

  Second try, now using hash tables rather than binary trees (which caused
  problems when files were read in alphabetical order, because they got 
  pathologically unbalanced).

*/
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <voxi/util/config.h>

#include <voxi/alwaysInclude.h>

#ifdef _FILE_IO
#include <voxi/util/file.h>
#endif

#include <voxi/util/hash.h>
#include <voxi/util/err.h>
#include <voxi/util/logging.h>
#include <voxi/util/mem.h>
#ifdef WIN32
#include <sys/types.h>
#include <voxi/util/win32_glue.h> /* strsep */
#endif

#include <voxi/util/wordMap.h>

CVSID("$Id$");

#define FILE_FORMAT_VERSION 2
#define DEFAULT_HASHTABLE_SIZE 1024

LOG_MODULE_DECL( "WordMap", LOGLEVEL_NONE );

typedef struct sWordMap
{
  char *name;

  WordMapType type;

  HashTable byName;
  HashTable byNumber;
} sWordMap;

typedef struct sEntry
{
  const char *name;
  unsigned int number;
} sEntry, *Entry;

typedef struct sWordMapCursor
{
  HashTableCursor hashCursor;
} sWordMapCursor;

static int compareEntryNames( Entry e1, Entry pme2 );
static int compareEntryNumbers( Entry e1, Entry pme2 );
static int hashEntryName( Entry e );
static int hashEntryNumber( Entry e );

Error wordMap_create( const char *name, WordMapType type, WordMap *phoneMap )
{
  return wordMap_create2( name, type, phoneMap, DEFAULT_HASHTABLE_SIZE );
}

Error wordMap_create2( const char *name, WordMapType type, WordMap *phoneMap,
                       int hashTableSize )
{
  DEBUG("enter\n");

  *phoneMap = malloc( sizeof( sWordMap ) );

  assert( *phoneMap != NULL );

  (*phoneMap)->name = _strdup( name );
  (*phoneMap)->type = type;

  if( type & WORDMAPMASK_BYNAME )
    (*phoneMap)->byName = HashCreateTable( hashTableSize, 
                                           (HashFuncPtr) hashEntryName, 
                                           (CompFuncPtr) compareEntryNames,
                                           NULL );

  if( type & WORDMAPMASK_BYNUMBER )
    (*phoneMap)->byNumber = HashCreateTable( hashTableSize, 
                                             (HashFuncPtr) hashEntryNumber,
                                             (CompFuncPtr) compareEntryNumbers,
                                             NULL );

  DEBUG("leave\n");

  return NULL;
}

WordMapType wordMap_getType( WordMap map )
{
  return map->type;
}

const char *wordMap_getName( WordMap map )
{
  return map->name;
}

Error wordMap_add( WordMap map, const char *name, int number )
{
  Entry newEntry;

  LOG_TRACE( LOG_TRACE_ARG, "wordMap_add( map %s, '%s', %d )", map->name, 
             name, number );

  newEntry = malloc( sizeof( sEntry ));
  assert( newEntry != NULL );

  newEntry->name = _strdup( name );
  newEntry->number = number;

  if( map->type & WORDMAPMASK_BYNAME )
    HashAdd( map->byName, newEntry );

  if( map->type & WORDMAPMASK_BYNUMBER )
    HashAdd( map->byNumber, newEntry );

  return NULL;
}

int wordMap_findByName( WordMap map, const char *name )
{
  sEntry template;
  Entry entry;

  ErrPushFunc( "wordMap_findByName( map %p, '%s' )", map, name );

  if( !(map->type & WORDMAPMASK_BYNAME) )
  {
    fprintf( stderr, "wordMap_findByName( %p, '%s' ) for map with type %d\n",
             map, name, map->type );
    return -1;
  }

  template.name = name;

  entry = HashFind( map->byName, &template );

  ErrPopFunc();

  if( entry == NULL )
    return -1;
  else
    return entry->number;
}

const char *wordMap_findByNumber( WordMap map, int number )
{
  sEntry template;
  Entry entry;

  assert( map->type & WORDMAPMASK_BYNUMBER );

  template.number = number;
  
  entry = HashFind( map->byNumber, &template );
  
  if( entry == NULL )
    return NULL;
  else
    return entry->name;
}

int wordMap_getNoElements(WordMap map) 
{
  if( map->type & WORDMAPMASK_BYNUMBER ) 
    return HashGetElementCount(map->byNumber);
  else 
    return HashGetElementCount(map->byName);      
}

#ifdef _FILE_IO
Error wordMap_save( WordMap map, FILE *file )
{
  HashTable hashTable;
  HashTableCursor cursor;

  fprintf( file, "%d\n", FILE_FORMAT_VERSION );
  fprintf( file, "%s\n", map->name );
  fprintf( file, "%d\n", map->type );

  if( map->type & WORDMAPMASK_BYNAME )
    hashTable = map->byName;
  else if( map->type & WORDMAPMASK_BYNUMBER )
    hashTable = map->byNumber;
  else {
    assert( FALSE );
    hashTable = NULL;
  }
  fprintf( file, "%d\n", HashGetElementCount( hashTable ) );
  
  cursor = HashCursorCreate( hashTable );
  
  while( !HashCursorPastLastElement( cursor ) )
  {
    Entry entry = (Entry) HashCursorGetElement( cursor );
    
    assert( entry != NULL );
    
    fprintf( file, "%d %s\n", entry->number, entry->name );
    
    HashCursorGoNext( cursor );
  }

  HashCursorDestroy( cursor );
  
  return NULL;
}

Error wordMap_saveToStringBuffer( WordMap map, StringBuffer *memBlock )
{
  Error error;
  HashTable hashTable;
  HashTableCursor cursor;
  
  error = strbuf_create( 65536, memBlock );
  assert( error == NULL );
  
  error = strbuf_stringList_append2( *memBlock, "%d", FILE_FORMAT_VERSION );
  assert( error == NULL );
  
  error = strbuf_stringList_append2( *memBlock, "%s", map->name );
  assert( error == NULL );
  error = strbuf_stringList_append2( *memBlock, "%d", map->type );
  assert( error == NULL );
  
  if( map->type & WORDMAPMASK_BYNAME )
    hashTable = map->byName;
  else if( map->type & WORDMAPMASK_BYNUMBER )
    hashTable = map->byNumber;
  else {
    assert( FALSE );
    hashTable = NULL;
  }
  
  error = strbuf_stringList_append2( *memBlock, "%d", 
                                     HashGetElementCount( hashTable ) );
  assert( error == NULL );
  cursor = HashCursorCreate( hashTable );
  
  while( !HashCursorPastLastElement( cursor ) )
  {
    Entry entry = (Entry) HashCursorGetElement( cursor );
    
    assert( entry != NULL );
    
    error = strbuf_stringList_append2( *memBlock, "%d", entry->number );
    assert( error == NULL );
    error = strbuf_stringList_append2( *memBlock, "%s", entry->name );
    assert( error == NULL );
    
    HashCursorGoNext( cursor );
  }

  HashCursorDestroy( cursor );
  
  return NULL;
}

Error wordMap_load( FILE *file, WordMap *map )
{
  int version, count, i;
  char name[256];
  Error error;
  int type;
  
  fscanf( file, "%d\n", &version );
  
  fgets( name, 256, file );
  name[ strlen( name ) - 1 ] = '\0'; /* remove newline */
  
  switch( version )
  {
    case 1:
      fscanf( file, "%d\n", &count );
      /* passing count here will result in a fairly large but fast hash table.
         We might consider using count / 5 or some such thing here instead
         to make a more memory efficient hash table with marginal efficiency 
         loss */
      error = wordMap_create2( name, WORDMAP_BYNAME_NUMBER, map, count );
      break;

  case 2:
    fscanf( file, "%d\n", &type );
    fscanf( file, "%d\n", &count );
    
    error = wordMap_create2( name, type, map, count );
    break;
    
  default:
    return ErrNew( ERR_APP, 0, NULL, "Unsupported version number (%d) in "
                   "WordMap.", version );
  }
  
  assert( error == NULL );
  
  for( i = 0; i < count; i++ )
  {
#define _WORDMAP_LOAD_BUFSIZE 1024
    char buffer[ _WORDMAP_LOAD_BUFSIZE ];
    char *charPtr1, *charPtr2;
    
    /* fscanf here doesn't work if there are spaces in the name */
    file_readLine( file, buffer, _WORDMAP_LOAD_BUFSIZE );
    
    charPtr1 = buffer;
    
    /* charPtr2 = strtok( buffer, " " ); */
    /* charPtr1 = strtok( NULL, "" ); */
    
    charPtr2 = strsep( &charPtr1, " " );
    
    /* fscanf( file, "%d %s\n", &number, name ); */
    
    wordMap_add( *map, charPtr1, atoi( charPtr2 ) );
  }

  return NULL;
}
#endif  /* ifdef _FILE_IO */

Error wordMap_createFromString(const char *str[], WordMap *map) 
{
  Error error = NULL;
  int version, count, i;
  const char *name;
  int type;
  int j = 0;
  
  version = atoi(str[j]);
  j++;
  
  name = str[j];
  j++;
  
  switch( version )
  {
    case 1:
      count = atoi(str[j]);
      j++;
      
      /* passing count here will result in a fairly large but fast hash table.
         We might consider using count / 5 or some such thing here instead
         to make a more memory efficient hash table with marginal efficiency 
         loss */
      error = wordMap_create2( name, WORDMAP_BYNAME_NUMBER, map, count );
      break;
      
    case 2:
      type = atoi(str[j]);
      j++;
      count = atoi(str[j]);
      j++;
    
      error = wordMap_create2( name, type, map, count );
      break;
      
    default:
      return ErrNew( ERR_APP, 0, NULL, "Unsupported version number (%d) in "
                     "WordMap.", version );
  }
  
  assert( error == NULL );
  
  for( i = 0; i < count; i++ )
  {
    char *buffer;
    char *charPtr1, *charPtr2;
    
    /* fscanf here doesn't work if there are spaces in the name */
    buffer = _strdup( str[j] );
    j++;
    
    charPtr1 = buffer;
    
    charPtr2 = strsep( &charPtr1, " " );
    
    wordMap_add( *map, charPtr1, atoi( charPtr2 ) );
    free( buffer );
  }
  
  return NULL;
}

Error wordMap_deleteByNumber( WordMap map, int number )
{
  sEntry template;
  Entry entry;
  int tempInt;
  
  assert( map->type & WORDMAPMASK_BYNUMBER );
  
  template.number = number;
  
  entry = HashFind( map->byNumber, &template );
  assert( entry != NULL );
  
  tempInt = HashDelete( map->byNumber, entry );
  assert( tempInt == 1 );
  
  if( map->type & WORDMAPMASK_BYNAME )
  {
    tempInt = HashDelete( map->byName, entry );
    assert( tempInt == 1 );
  }
  
  free( (char *) entry->name );
  free( entry );
  
  return NULL;
}

Error wordMap_deleteByName( WordMap map, const char *name )
{
  sEntry template;
  Entry entry;
  int tempInt;
  
  assert( map->type & WORDMAPMASK_BYNAME );
  
  template.name = name;
  
  entry = HashFind( map->byName, &template );
  assert( entry != NULL );
  
  tempInt = HashDelete( map->byName, entry );
  assert( tempInt == 1 );
  
  if( map->type & WORDMAPMASK_BYNUMBER )
  {
    tempInt = HashDelete( map->byNumber, entry );
    assert( tempInt == 1 );
  }
  
  free( (char *) entry->name );
  free( entry );
  
  return NULL;
}


/*
 *  Cursor functions
 */

/** 
    create a cursor for traversing a word map.
    
    Note that the cursor must either traverse the entries by name or by number;
    if traversing by name, then only one entry for each name will be returned,
    if traversing by number, then only one entry with each number will be 
    returned. This is unfortunately due to the fact that all information in the
    map is stored in hash tables.
    
    The mask argument must be either WORDMAPMASK_BYNAME or 
    WORDMAPMASK_BYNUMBER.
*/
Error wordMap_cursor_create( WordMap map, int mask, WordMapCursor *cursor )
{
  Error error;
  
  if( (map->type & mask) == 0 )
  {
    error = ErrNew( ERR_UNKNOWN, 0, NULL, "Requested a cursor for mask %d, "
                    "but the wordmap is not indexed by that mask." );
    goto CURSOR_CREATE_FAIL_1;
  }
  
  error = emalloc( (void **) cursor, sizeof( sWordMapCursor ) );
  if( error != NULL )
  {
    error = ErrNew( ERR_UNKNOWN, 0, error, "Failed to allocate memory for "
                    "the word map cursor." );
    goto CURSOR_CREATE_FAIL_1;
  }
  
  switch( mask )
  {
    case WORDMAPMASK_BYNUMBER:
      (*cursor)->hashCursor = HashCursorCreate( map->byNumber );
      break;
      
    case WORDMAPMASK_BYNAME:
      (*cursor)->hashCursor = HashCursorCreate( map->byName );
      break;
      
    default:
      assert( FALSE ); /* invalid mask parameter */
      break;
  }
  
  return NULL;
  
 CURSOR_CREATE_FAIL_1:
  assert( error != NULL );
  
  return error;
}

void wordMap_cursor_destroy( WordMapCursor cursor )
{
  HashCursorDestroy( cursor->hashCursor );
}

int wordMap_cursor_getElementNumber( WordMapCursor cursor )
{
  Entry entry;
  
  entry = HashCursorGetElement( cursor->hashCursor );
  
  return entry->number;
}

const char *wordMap_cursor_getElementName( WordMapCursor cursor )
{
  Entry entry;
  
  entry = HashCursorGetElement( cursor->hashCursor );
  
  return entry->name;
}

void wordMap_cursor_goFirst( WordMapCursor cursor )
{
  HashCursorGoFirst( cursor->hashCursor );
}

void wordMap_cursor_goNext( WordMapCursor cursor )
{
  HashCursorGoNext( cursor->hashCursor );
}

Boolean wordMap_cursor_pastLastElement( WordMapCursor cursor )
{
  return HashCursorPastLastElement( cursor->hashCursor );
}

static int compareEntryNames( Entry pme1, Entry pme2 )
{
  return strcmp( pme1->name, pme2->name );
}

static int compareEntryNumbers( Entry pme1, Entry pme2 )
{
  return pme2->number - pme1->number;
}

static int hashEntryName( Entry e )
{
  return HashString( e->name );
}

static int hashEntryNumber( Entry e )
{
  return e->number;
}
